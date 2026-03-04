#include "PluginLoadingCoordinator.h"
#include "PluginInstance.h"
#include "VSTPluginManager.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../AudioRouting/TrackProcessor.h"
#include <chrono>

//==============================================================================
// PluginLoadingCoordinator Implementation
//==============================================================================

PluginLoadingCoordinator::PluginLoadingCoordinator()
{
    // Create lock-free queues
    requestQueue = std::make_unique<LockFreePluginQueue<PluginLoadRequest, maxPendingRequests>>();
    resultQueue = std::make_unique<LockFreePluginQueue<PluginLoadResult, maxPendingResults>>();
}

PluginLoadingCoordinator::~PluginLoadingCoordinator()
{
    cancelAllLoadRequests();
}

//==============================================================================
// Initialization
//==============================================================================

void PluginLoadingCoordinator::initialize()
{
    // Reset queues and counters
    nextRequestId.store(1);
    activeLoadCount.store(0);
}

void PluginLoadingCoordinator::prepareToPlay(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;
}

void PluginLoadingCoordinator::releaseResources()
{
    cancelAllLoadRequests();
}

//==============================================================================
// Plugin Loading (From UI Thread)
//==============================================================================

uint32_t PluginLoadingCoordinator::queueLoadRequest(
    const PluginLoadRequest& request,
    std::function<void(const PluginLoadResult&)> callback)
{
    uint32_t requestId = generateRequestId();

    // Copy request and set ID
    PluginLoadRequest* requestCopy = requestQueue->allocate();
    if (!requestCopy)
    {
        // Queue full - fallback to direct callback
        PluginLoadResult result;
        result.requestId = requestId;
        result.status = LoadStatus::Failed;
        result.errorMessage = "Plugin loading queue is full. Please wait and try again.";
        if (callback)
            callback(result);
        totalLoadFailures++;
        return 0;
    }

    new (requestCopy) PluginLoadRequest(request);
    requestCopy->requestId = requestId;

    // Add to queue
    if (!requestQueue->enqueue(requestCopy))
    {
        requestQueue->release(requestCopy);
        PluginLoadResult result;
        result.requestId = requestId;
        result.status = LoadStatus::Failed;
        result.errorMessage = "Failed to enqueue plugin load request.";
        if (callback)
            callback(result);
        totalLoadFailures++;
        return 0;
    }

    // Store callback for this request
    {
        juce::ScopedLock lock(activeRequestsLock);
        ActiveRequest activeReq;
        activeReq.requestId = requestId;
        activeReq.request = *requestCopy;
        activeReq.startTime = juce::Time::currentTimeMillis();
        activeReq.status = LoadStatus::Pending;
        activeRequests.push_back(activeReq);
    }

    // Process request asynchronously
    processLoadRequest(*requestCopy);

    return requestId;
}

void PluginLoadingCoordinator::cancelLoadRequest(uint32_t requestId)
{
    juce::ScopedLock lock(activeRequestsLock);
    for (auto& req : activeRequests)
    {
        if (req.requestId == requestId)
        {
            req.status = LoadStatus::Cancelled;
            break;
        }
    }
}

void PluginLoadingCoordinator::cancelAllLoadRequests()
{
    juce::ScopedLock lock(activeRequestsLock);
    for (auto& req : activeRequests)
    {
        req.status = LoadStatus::Cancelled;
    }
}

//==============================================================================
// Plugin Unloading (From UI Thread)
//==============================================================================

void PluginLoadingCoordinator::queueUnloadRequest(
    uint32_t nodeId,
    std::function<void(bool success)> callback)
{
    // For now, unloading is synchronous and handled by AudioRoutingGraph
    // In a future version, we could make this async as well
    // This is a placeholder for future enhancement
    // The actual removal is handled by AudioRoutingGraph::removePluginNode()
    (void)nodeId;
    (void)callback;
}

//==============================================================================
// Audio Thread Processing (Called from Audio Engine)
//==============================================================================

int PluginLoadingCoordinator::processPendingLoads()
{
    int processed = 0;

    // Process all pending results
    while (auto* result = resultQueue->dequeue())
    {
        PluginLoadResult resultCopy = *result;  // Copy result

        // Remove from active requests
        removeActiveRequest(resultCopy.requestId);

        // Process result
        if (resultCopy.isSuccess())
        {
            insertPluginIntoGraph(resultCopy);
            totalPluginsLoaded++;

            // Note: targetTrackIndex is stored in PluginLoadResult for future use
            // For now, we don't have the request info available in resultCopy
            // This would need to be added to PluginLoadResult

            if (pluginInsertedCallback)
                pluginInsertedCallback(resultCopy.nodeId, 0);  // Track index not available in current implementation

            // Store timing data for statistics
            {
                juce::ScopedLock lock(loadTimeStatsLock);
                loadTimeHistory.push_back(resultCopy.loadDuration);
                // Keep only last 100 measurements
                if (loadTimeHistory.size() > 100)
                    loadTimeHistory.erase(loadTimeHistory.begin());
            }
        }
        else
        {
            totalLoadFailures++;

            if (loadFailureCallback)
            {
                // Note: resultCopy.requestId was referenced but should be resultCopy.requestId is correct (it's in the struct)
                // However, the compiler error says 'request' is not a member of PluginLoadResult
                // Let's check the PluginLoadResult struct
                loadFailureCallback(resultCopy.requestId, resultCopy.errorMessage);
            }
        }

        resultQueue->release(result);
        processed++;
    }

    return processed;
}

//==============================================================================
// Status Queries
//==============================================================================

bool PluginLoadingCoordinator::hasPendingLoads() const
{
    return getNumPendingLoads() > 0;
}

int PluginLoadingCoordinator::getNumPendingLoads() const
{
    juce::ScopedLock lock(activeRequestsLock);
    int count = 0;
    for (const auto& req : activeRequests)
    {
        if (req.status == LoadStatus::Pending || req.status == LoadStatus::Loading ||
            req.status == LoadStatus::Preparing)
        {
            count++;
        }
    }
    return count;
}

int PluginLoadingCoordinator::getNumPendingUnloads() const
{
    // Not implemented yet - unloading is currently synchronous
    return 0;
}

bool PluginLoadingCoordinator::getLoadResult(uint32_t requestId, PluginLoadResult& result) const
{
    juce::ScopedLock lock(activeRequestsLock);
    for (const auto& req : activeRequests)
    {
        if (req.requestId == requestId && req.status == LoadStatus::Success)
        {
            // For now, return a minimal result
            // In a full implementation, we'd store completed results
            result.requestId = requestId;
            result.status = req.status;
            return true;
        }
    }
    return false;
}

bool PluginLoadingCoordinator::isRequestPending(uint32_t requestId) const
{
    juce::ScopedLock lock(activeRequestsLock);
    for (const auto& req : activeRequests)
    {
        if (req.requestId == requestId &&
            (req.status == LoadStatus::Pending || req.status == LoadStatus::Loading ||
             req.status == LoadStatus::Preparing))
        {
            return true;
        }
    }
    return false;
}

//==============================================================================
// Callbacks
//==============================================================================

void PluginLoadingCoordinator::setLoadProgressCallback(
    std::function<void(uint32_t requestId, float progress)> callback)
{
    loadProgressCallback = callback;
}

void PluginLoadingCoordinator::setPluginInsertedCallback(
    std::function<void(uint32_t nodeId, int trackIndex)> callback)
{
    pluginInsertedCallback = callback;
}

void PluginLoadingCoordinator::setPluginRemovedCallback(
    std::function<void(uint32_t nodeId)> callback)
{
    pluginRemovedCallback = callback;
}

void PluginLoadingCoordinator::setLoadFailureCallback(
    std::function<void(uint32_t requestId, const juce::String& error)> callback)
{
    loadFailureCallback = callback;
}

//==============================================================================
// Statistics
//==============================================================================

double PluginLoadingCoordinator::getAverageLoadTimeMs() const
{
    juce::ScopedLock lock(loadTimeStatsLock);
    if (loadTimeHistory.empty())
        return 0.0;

    juce::int64 sum = 0;
    for (auto time : loadTimeHistory)
        sum += time;

    return static_cast<double>(sum) / loadTimeHistory.size();
}

void PluginLoadingCoordinator::resetStatistics()
{
    totalPluginsLoaded.store(0);
    totalLoadFailures.store(0);

    juce::ScopedLock lock(loadTimeStatsLock);
    loadTimeHistory.clear();
}

//==============================================================================
// Internal Methods
//==============================================================================

void PluginLoadingCoordinator::processLoadRequest(const PluginLoadRequest& request)
{
    // Update request status
    {
        juce::ScopedLock lock(activeRequestsLock);
        if (auto* req = findActiveRequest(request.requestId))
        {
            req->status = LoadStatus::Loading;
        }
    }

    // Allocate result from result queue pool
    PluginLoadResult* result = resultQueue->allocate();
    if (!result)
    {
        // Result queue full - send failure
        PluginLoadResult failureResult;
        failureResult.requestId = request.requestId;
        failureResult.status = LoadStatus::Failed;
        failureResult.errorMessage = "Plugin result queue is full.";

        juce::ScopedLock lock(activeRequestsLock);
        if (auto* req = findActiveRequest(request.requestId))
        {
            req->status = LoadStatus::Failed;
        }

        totalLoadFailures++;
        return;
    }

    // Initialize result
    new (result) PluginLoadResult();
    result->requestId = request.requestId;
    result->loadStartTime = juce::Time::currentTimeMillis();

    // Note: Actual plugin loading is handled by VSTPluginManager
    // This coordinator manages the queue and coordination
    // The actual loading happens via VSTPluginManager::loadPlugin()
    // which is already async

    // For now, we'll create a placeholder result
    // In a real implementation, we'd call VSTPluginManager here
    result->status = LoadStatus::Pending;
    result->pluginName = "Pending Plugin";
    result->loadEndTime = juce::Time::currentTimeMillis();

    // Enqueue result for audio thread processing
    if (!resultQueue->enqueue(result))
    {
        resultQueue->release(result);

        // Send failure notification
        juce::ScopedLock lock(activeRequestsLock);
        if (auto* req = findActiveRequest(request.requestId))
        {
            req->status = LoadStatus::Failed;
        }

        totalLoadFailures++;
        return;
    }

    // Update request status to Preparing
    {
        juce::ScopedLock lock(activeRequestsLock);
        if (auto* req = findActiveRequest(request.requestId))
        {
            req->status = LoadStatus::Preparing;
        }
    }
}

void PluginLoadingCoordinator::preparePlugin(PluginLoadResult& result)
{
    if (!result.pluginInstance || !result.pluginInstance->getAudioPluginInstance())
        return;

    auto* plugin = result.pluginInstance->getAudioPluginInstance();

    // Get plugin metadata
    result.pluginName = plugin->getName();
    result.manufacturerName = plugin->getPluginDescription().manufacturerName;
    result.category = plugin->getPluginDescription().category;
    result.numInputs = plugin->getTotalNumInputChannels();
    result.numOutputs = plugin->getTotalNumOutputChannels();
    result.numParameters = plugin->getParameters().size();

    // Prepare plugin with audio settings
    plugin->setPlayConfigDetails(result.numInputs, result.numOutputs,
                                 currentSampleRate, currentBlockSize);
    plugin->prepareToPlay(currentSampleRate, currentBlockSize);
}

void PluginLoadingCoordinator::insertPluginIntoGraph(const PluginLoadResult& result)
{
    // This is called from the audio thread
    // The actual insertion is handled by AudioRoutingGraph
    // We need to get a reference to AudioRoutingGraph here
    // For now, this is a placeholder - the integration will be done later

    // The nodeId will be assigned by AudioRoutingGraph::insertPluginInstanceInTrack()
    // result.nodeId = graph->insertPluginInstanceInTrack(...);

    (void)result;  // Suppress unused warning
}

void PluginLoadingCoordinator::removePluginFromGraph(uint32_t nodeId)
{
    // This is called from the audio thread
    // The actual removal is handled by AudioRoutingGraph
    // graph->removePluginNode(nodeId);

    if (pluginRemovedCallback)
        pluginRemovedCallback(nodeId);
}

PluginLoadingCoordinator::ActiveRequest* PluginLoadingCoordinator::findActiveRequest(uint32_t requestId)
{
    for (auto& req : activeRequests)
    {
        if (req.requestId == requestId)
            return &req;
    }
    return nullptr;
}

void PluginLoadingCoordinator::removeActiveRequest(uint32_t requestId)
{
    juce::ScopedLock lock(activeRequestsLock);
    for (auto it = activeRequests.begin(); it != activeRequests.end(); ++it)
    {
        if (it->requestId == requestId)
        {
            activeRequests.erase(it);
            break;
        }
    }
}

uint32_t PluginLoadingCoordinator::generateRequestId()
{
    return nextRequestId.fetch_add(1, std::memory_order_relaxed);
}
