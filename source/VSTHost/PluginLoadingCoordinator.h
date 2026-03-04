#pragma once

/**
 * PluginLoadingCoordinator - Lock-Free Plugin Loading Coordinator
 *
 * Phase 6.1: Lock-Free Plugin Loading
 *
 * This class provides thread-safe, lock-free plugin loading and transfer to the audio thread.
 * It coordinates plugin loading between the UI/background thread and the audio thread using
 * lock-free queues and atomic operations.
 *
 * Architecture:
 *
 *   UI/Background Thread                          Audio Thread
 *   ─────────────────────                          ────────────
 *   PluginLoadRequest
 *   ─────────────────► Queue 1 ─────────────────► Process Loads
 *
 *   PluginLoadResult
 *   ◄──────────────── Queue 2 ◄───────────────── Return Results
 *
 * Benefits:
 *   - No audio glitches during plugin loading
 *   - Real-time safe plugin insertion/removal
 *   - Graceful fallback on plugin load failures
 *   - Progress callbacks for UI updates
 *
 * Usage:
 *   1. Request plugin load from UI (queueLoadRequest)
 *   2. Background thread processes requests (loadPluginAsync)
 *   3. Plugin prepared in background thread
 *   4. Results transferred via lock-free queue
 *   5. Audio thread safely inserts plugin
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>
#include <functional>
#include <vector>

class PluginInstance;

//==============================================================================
// Plugin Load Request - Sent from UI to loading system
//==============================================================================

struct PluginLoadRequest
{
    uint32_t requestId = 0;               // Unique ID for tracking
    juce::String pluginPath;              // Path to plugin file
    juce::String pluginFormatName;         // e.g., "VST3", "AU"
    juce::PluginDescription description;   // Plugin description (optional)
    int targetTrackIndex = 0;             // Target track for insertion
    int positionInChain = -1;             // Position in plugin chain (-1 = end)
    double sampleRate = 44100.0;          // Target sample rate
    int blockSize = 512;                  // Target block size

    // Load options
    bool scanForParameters = true;         // Scan parameters after load
    bool enableAutomation = true;          // Enable parameter automation
};

//==============================================================================
// Plugin Load Result - Returned to UI after loading completes
//==============================================================================

enum class LoadStatus
{
    Pending,
    Loading,
    Preparing,
    Success,
    Failed,
    Timeout,
    Cancelled
};

struct PluginLoadResult
{
    uint32_t requestId = 0;               // Matching request ID
    LoadStatus status = LoadStatus::Pending;
    juce::String errorMessage;            // Error message if failed

    // Plugin data
    uint32_t nodeId = 0;                // Node ID assigned by graph
    std::shared_ptr<PluginInstance> pluginInstance;  // Loaded plugin instance
    juce::AudioPluginInstance* audioPluginInstance = nullptr;  // Convenience pointer

    // Metadata
    juce::String pluginName;             // Plugin name
    juce::String manufacturerName;        // Manufacturer
    juce::String category;               // Plugin category
    int numInputs = 0;                   // Number of audio inputs
    int numOutputs = 0;                  // Number of audio outputs
    int numParameters = 0;                // Number of parameters

    // Timing info
    juce::int64 loadStartTime = 0;       // When load started (milliseconds)
    juce::int64 loadEndTime = 0;         // When load completed (milliseconds)
    juce::int64 loadDuration = 0;               // Duration in milliseconds

    // Plugin state (for state restoration)
    juce::MemoryBlock pluginState;       // Plugin state blob

    /** Check if this result represents a successful load. */
    bool isSuccess() const { return status == LoadStatus::Success; }

    /** Check if this result represents a failed load. */
    bool isFailed() const { return status == LoadStatus::Failed || status == LoadStatus::Timeout; }
};

//==============================================================================
// Lock-Free Queue for Plugin Loading (Single Producer, Single Consumer)
//==============================================================================

template<typename T, int Capacity>
class LockFreePluginQueue
{
public:
    LockFreePluginQueue() : head(0), tail(0)
    {
        // Pre-allocate memory for all elements
        buffer = std::make_unique<std::atomic<T*>[]>(Capacity);
        for (int i = 0; i < Capacity; ++i)
            buffer[i].store(nullptr, std::memory_order_relaxed);

        // Pre-allocate element pool
        pool = std::make_unique<T[]>(Capacity);
        poolIndex = 0;
    }

    ~LockFreePluginQueue()
    {
        // Clean up any remaining elements
        while (head.load(std::memory_order_relaxed) != tail.load(std::memory_order_relaxed))
        {
            T* elem = dequeue();
            if (elem)
                elem->~T();
        }
    }

    /** Enqueue an element (single producer). Returns true if successful. */
    bool enqueue(T* element)
    {
        if (!element)
            return false;

        int currentTail = tail.load(std::memory_order_relaxed);
        int nextTail = (currentTail + 1) % Capacity;

        // Check if queue is full
        int currentHead = head.load(std::memory_order_acquire);
        if (nextTail == currentHead)
            return false;  // Queue is full

        buffer[currentTail].store(element, std::memory_order_release);
        tail.store(nextTail, std::memory_order_release);
        return true;
    }

    /** Dequeue an element (single consumer). Returns nullptr if empty. */
    T* dequeue()
    {
        int currentHead = head.load(std::memory_order_relaxed);
        int currentTail = tail.load(std::memory_order_acquire);

        if (currentHead == currentTail)
            return nullptr;  // Queue is empty

        T* element = buffer[currentHead].load(std::memory_order_acquire);
        head.store((currentHead + 1) % Capacity, std::memory_order_release);
        return element;
    }

    /** Check if queue is empty. */
    bool isEmpty() const
    {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    /** Get the number of elements in the queue. */
    int getNumElements() const
    {
        int h = head.load(std::memory_order_acquire);
        int t = tail.load(std::memory_order_acquire);
        return (t >= h) ? (t - h) : (Capacity - h + t);
    }

    /** Allocate a new element from the pool. */
    T* allocate()
    {
        if (poolIndex >= Capacity)
            return new T();  // Fallback to heap allocation

        return &pool[poolIndex++];
    }

    /** Release an element back to the pool. */
    void release(T* element)
    {
        if (!element)
            return;

        // Check if element is from pool
        if (element >= pool.get() && element < (pool.get() + Capacity))
        {
            element->~T();
            // Pool elements are automatically reused
        }
        else
        {
            delete element;
        }
    }

private:
    std::unique_ptr<std::atomic<T*>[]> buffer;
    std::unique_ptr<T[]> pool;
    std::atomic<int> head;
    std::atomic<int> tail;
    int poolIndex = 0;
};

//==============================================================================
// Plugin Loading Coordinator
//==============================================================================

class PluginLoadingCoordinator
{
public:
    PluginLoadingCoordinator();
    ~PluginLoadingCoordinator();

    // ============================================================
    // Initialization
    // ============================================================

    /** Initialize the coordinator. */
    void initialize();

    /** Prepare for playback with given audio settings. */
    void prepareToPlay(double sampleRate, int blockSize);

    /** Clean up resources. */
    void releaseResources();

    // ============================================================
    // Plugin Loading (From UI Thread)
    // ============================================================

    /**
     * Request a plugin to be loaded asynchronously.
     * @param request The load request details
     * @param callback Called when load completes (with LoadResult)
     * @return Request ID for tracking
     */
    uint32_t queueLoadRequest(const PluginLoadRequest& request,
                             std::function<void(const PluginLoadResult&)> callback);

    /**
     * Cancel a pending load request.
     * @param requestId The request ID to cancel
     */
    void cancelLoadRequest(uint32_t requestId);

    /**
     * Cancel all pending load requests.
     */
    void cancelAllLoadRequests();

    // ============================================================
    // Plugin Unloading (From UI Thread)
    // ============================================================

    /**
     * Request a plugin to be unloaded.
     * @param nodeId The node ID of the plugin to unload
     * @param callback Called when unload completes
     */
    void queueUnloadRequest(uint32_t nodeId,
                          std::function<void(bool success)> callback);

    // ============================================================
    // Audio Thread Processing (Called from Audio Engine)
    // ============================================================

    /**
     * Process pending plugin load/unload requests.
     * This MUST be called from the audio thread at regular intervals.
     * Returns the number of plugins loaded/unloaded.
     */
    int processPendingLoads();

    // ============================================================
    // Status Queries
    // ============================================================

    /** Check if any loads are pending. */
    bool hasPendingLoads() const;

    /** Get the number of pending load requests. */
    int getNumPendingLoads() const;

    /** Get the number of pending unload requests. */
    int getNumPendingUnloads() const;

    /** Get a load result by request ID. */
    bool getLoadResult(uint32_t requestId, PluginLoadResult& result) const;

    /** Check if a request is still pending. */
    bool isRequestPending(uint32_t requestId) const;

    // ============================================================
    // Callbacks
    // ============================================================

    /** Set callback for load progress updates. */
    void setLoadProgressCallback(std::function<void(uint32_t requestId, float progress)> callback);

    /** Set callback for plugin inserted notification. */
    void setPluginInsertedCallback(std::function<void(uint32_t nodeId, int trackIndex)> callback);

    /** Set callback for plugin removed notification. */
    void setPluginRemovedCallback(std::function<void(uint32_t nodeId)> callback);

    /** Set callback for load failure notification. */
    void setLoadFailureCallback(std::function<void(uint32_t requestId, const juce::String& error)> callback);

    // ============================================================
    // Configuration
    // ============================================================

    /** Set the maximum number of concurrent loads. */
    void setMaxConcurrentLoads(int maxLoads) { maxConcurrentLoads = maxLoads; }

    /** Get the maximum number of concurrent loads. */
    int getMaxConcurrentLoads() const { return maxConcurrentLoads; }

    /** Set the load timeout in milliseconds. */
    void setLoadTimeoutMs(int timeoutMs) { loadTimeoutMs = timeoutMs; }

    /** Get the load timeout in milliseconds. */
    int getLoadTimeoutMs() const { return loadTimeoutMs; }

    // ============================================================
    // Statistics
    // ============================================================

    /** Get the total number of plugins loaded successfully. */
    int getTotalPluginsLoaded() const { return totalPluginsLoaded.load(); }

    /** Get the total number of plugin load failures. */
    int getTotalLoadFailures() const { return totalLoadFailures.load(); }

    /** Get the average load time in milliseconds. */
    double getAverageLoadTimeMs() const;

    /** Reset statistics. */
    void resetStatistics();

private:
    // Queue capacities
    static constexpr int maxPendingRequests = 32;
    static constexpr int maxPendingResults = 32;

    // Lock-free queues
    std::unique_ptr<LockFreePluginQueue<PluginLoadRequest, maxPendingRequests>> requestQueue;
    std::unique_ptr<LockFreePluginQueue<PluginLoadResult, maxPendingResults>> resultQueue;

    // Atomic counters
    std::atomic<uint32_t> nextRequestId{1};
    std::atomic<int> activeLoadCount{0};
    std::atomic<int> totalPluginsLoaded{0};
    std::atomic<int> totalLoadFailures{0};

    // Timing data for statistics
    juce::CriticalSection loadTimeStatsLock;
    std::vector<juce::int64> loadTimeHistory;

    // Configuration
    int maxConcurrentLoads = 4;
    int loadTimeoutMs = 10000;  // 10 seconds default

    // Audio settings
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // Callbacks
    std::function<void(uint32_t requestId, float progress)> loadProgressCallback;
    std::function<void(uint32_t nodeId, int trackIndex)> pluginInsertedCallback;
    std::function<void(uint32_t nodeId)> pluginRemovedCallback;
    std::function<void(uint32_t requestId, const juce::String& error)> loadFailureCallback;

    // Active requests tracking
    struct ActiveRequest
    {
        uint32_t requestId;
        PluginLoadRequest request;
        juce::int64 startTime;
        LoadStatus status;
    };

    std::vector<ActiveRequest> activeRequests;
    juce::CriticalSection activeRequestsLock;

    // ============================================================
    // Internal Methods
    // ============================================================

    /** Process a single load request (called from background thread). */
    void processLoadRequest(const PluginLoadRequest& request);

    /** Prepare a loaded plugin for use in the audio thread. */
    void preparePlugin(PluginLoadResult& result);

    /** Insert plugin into the audio graph (called from audio thread). */
    void insertPluginIntoGraph(const PluginLoadResult& result);

    /** Remove plugin from the audio graph (called from audio thread). */
    void removePluginFromGraph(uint32_t nodeId);

    /** Find an active request by ID. */
    ActiveRequest* findActiveRequest(uint32_t requestId);

    /** Remove an active request by ID. */
    void removeActiveRequest(uint32_t requestId);

    /** Generate a unique request ID. */
    uint32_t generateRequestId();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginLoadingCoordinator)
};
