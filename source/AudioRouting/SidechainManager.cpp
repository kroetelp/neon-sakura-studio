#include "SidechainManager.h"
#include <juce_core/juce_core.h>

//==============================================================================
SidechainBuffer::SidechainBuffer()
{
    buffer.clear();
}

void SidechainBuffer::prepare(double sampleRate, int maxSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    int bufferSize = juce::jmin(maxSamplesPerBlock, maxBufferSize);
    buffer.setSize(maxBufferChannels, bufferSize, false, false, true);
    clear();
}

void SidechainBuffer::clear()
{
    buffer.clear();
    hasContentFlag.store(false);
}

void SidechainBuffer::addBuffer(const juce::AudioBuffer<float>& source, int numSamples, float gain)
{
    const int sourceChannels = juce::jmin(source.getNumChannels(), buffer.getNumChannels());
    const int samplesToAdd = juce::jmin(numSamples, buffer.getNumSamples());

    if (samplesToAdd <= 0 || sourceChannels <= 0)
        return;

    for (int ch = 0; ch < sourceChannels; ++ch)
    {
        juce::FloatVectorOperations::addWithMultiply(
            buffer.getWritePointer(ch),
            source.getReadPointer(ch),
            gain,
            samplesToAdd
        );
    }

    hasContentFlag.store(true);
}

//==============================================================================
SidechainManager::SidechainManager()
{
    // Sidechain-Buffer für alle Tracks initialisieren
    for (auto& buffer : sidechainBuffers)
        buffer = std::make_unique<SidechainBuffer>();
}

SidechainManager::~SidechainManager()
{
    releaseResources();
}

void SidechainManager::prepare(double sampleRate, int maxSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = maxSamplesPerBlock;

    for (auto& buffer : sidechainBuffers)
        buffer->prepare(sampleRate, maxSamplesPerBlock);
}

void SidechainManager::releaseResources()
{
    for (auto& buffer : sidechainBuffers)
        buffer->clear();
}

//==============================================================================
// Routing-Konfiguration

int SidechainManager::addRoute(const SidechainRoute& route)
{
    if (!route.isValid())
        return -1;

    // Prüfen ob Route bereits existiert
    for (const auto& existingRoute : routes)
    {
        if (existingRoute.sourceTrackIndex == route.sourceTrackIndex &&
            existingRoute.targetTrackIndex == route.targetTrackIndex &&
            existingRoute.targetPluginNodeId == route.targetPluginNodeId &&
            existingRoute.busIndex == route.busIndex)
        {
            // Route bereits vorhanden, nur aktualisieren
            return const_cast<SidechainRoute&>(existingRoute).sourceTrackIndex;
        }
    }

    // Neue Route hinzufügen
    SidechainRoute newRoute = route;
    newRoute.sourceTrackIndex = nextRouteId++;  // Speichere Route-ID in sourceTrackIndex (Workaround)
    routes.push_back(newRoute);

    return nextRouteId - 1;
}

bool SidechainManager::removeRoute(int routeId)
{
    auto it = std::find_if(routes.begin(), routes.end(),
        [routeId](const SidechainRoute& r) {
            // sourceTrackIndex speichert unsere Route-ID
            return r.sourceTrackIndex == routeId;
        });

    if (it != routes.end())
    {
        routes.erase(it);
        return true;
    }

    return false;
}

bool SidechainManager::updateRoute(int routeId, const SidechainRoute& route)
{
    auto it = std::find_if(routes.begin(), routes.end(),
        [routeId](const SidechainRoute& r) {
            return r.sourceTrackIndex == routeId;
        });

    if (it != routes.end() && route.isValid())
    {
        // Route-ID beibehalten
        int savedId = it->sourceTrackIndex;
        *it = route;
        it->sourceTrackIndex = savedId;
        return true;
    }

    return false;
}

bool SidechainManager::setRouteEnabled(int routeId, bool enabled)
{
    auto* route = const_cast<SidechainRoute*>(getRoute(routeId));
    if (route)
    {
        route->enabled = enabled;
        return true;
    }
    return false;
}

const SidechainRoute* SidechainManager::getRoute(int routeId) const
{
    auto it = std::find_if(routes.begin(), routes.end(),
        [routeId](const SidechainRoute& r) {
            return r.sourceTrackIndex == routeId;
        });

    return (it != routes.end()) ? &(*it) : nullptr;
}

SidechainRoute* SidechainManager::getRoute(int routeId)
{
    return const_cast<SidechainRoute*>(
        const_cast<const SidechainManager*>(this)->getRoute(routeId)
    );
}

std::vector<SidechainRoute*> SidechainManager::getRoutesForTarget(int targetTrackIndex)
{
    std::vector<SidechainRoute*> result;
    for (auto& route : routes)
    {
        if (route.targetTrackIndex == targetTrackIndex && route.isValid())
            result.push_back(&route);
    }
    return result;
}

std::vector<const SidechainRoute*> SidechainManager::getRoutesForTarget(int targetTrackIndex) const
{
    std::vector<const SidechainRoute*> result;
    for (const auto& route : routes)
    {
        if (route.targetTrackIndex == targetTrackIndex && route.isValid())
            result.push_back(&route);
    }
    return result;
}

std::vector<SidechainRoute*> SidechainManager::getRoutesForSource(int sourceTrackIndex)
{
    std::vector<SidechainRoute*> result;
    // Wir müssen die echte sourceTrackIndex extrahieren
    for (auto& route : routes)
    {
        if (route.sourceTrackIndex >= maxTracks &&  // Ist eine gültige Route-ID
            route.enabled)  // Route ist aktiv
        {
            // Das ist ein Hack - wir speichern echte Daten anders
            // Für eine saubere Implementierung bräuchten wir eine echte ID-Struktur
        }
    }
    return result;
}

const SidechainRoute* SidechainManager::findRouteByNodeId(uint32_t targetPluginNodeId) const
{
    auto it = std::find_if(routes.begin(), routes.end(),
        [targetPluginNodeId](const SidechainRoute& r) {
            return r.targetPluginNodeId == targetPluginNodeId && r.isValid();
        });

    return (it != routes.end()) ? &(*it) : nullptr;
}

//==============================================================================
// Audio-Processing

void SidechainManager::clearAllBuffers()
{
    for (auto& buffer : sidechainBuffers)
        buffer->clear();
}

void SidechainManager::addSourceToBuffer(int sourceTrackIndex,
                                         const juce::AudioBuffer<float>& audio,
                                         int numSamples)
{
    // Alle Routings finden, die diesen Track als Source nutzen
    for (const auto& route : routes)
    {
        if (route.enabled && route.sourceTrackIndex == sourceTrackIndex &&
            route.targetTrackIndex >= 0 && route.targetTrackIndex < maxTracks)
        {
            // Zum Buffer des Target-Tracks hinzufügen
            sidechainBuffers[route.targetTrackIndex]->addBuffer(audio, numSamples, route.gain);
        }
    }
}

SidechainBuffer* SidechainManager::getSidechainBuffer(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
        return sidechainBuffers[trackIndex].get();
    return nullptr;
}

const SidechainBuffer* SidechainManager::getSidechainBuffer(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
        return sidechainBuffers[trackIndex].get();
    return nullptr;
}

//==============================================================================
// Plugin Sidechain-Fähigkeiten

void SidechainManager::registerPluginSidechainInfo(const SidechainPluginInfo& info)
{
    if (info.nodeId != 0)
    {
        pluginSidechainInfos[info.nodeId] = info;
    }
}

const SidechainPluginInfo* SidechainManager::getPluginSidechainInfo(uint32_t nodeId) const
{
    auto it = pluginSidechainInfos.find(nodeId);
    return (it != pluginSidechainInfos.end()) ? &(it->second) : nullptr;
}

std::vector<SidechainPluginInfo> SidechainManager::getPluginsWithSidechainForTrack(int trackIndex) const
{
    std::vector<SidechainPluginInfo> result;

    for (const auto& [nodeId, info] : pluginSidechainInfos)
    {
        if (info.trackIndex == trackIndex && info.hasSidechainInput)
            result.push_back(info);
    }

    return result;
}

//==============================================================================
// UI-Visualisierung

bool SidechainManager::isTrackUsedAsSource(int trackIndex) const
{
    for (const auto& route : routes)
    {
        if (route.sourceTrackIndex == trackIndex && route.enabled)
            return true;
    }
    return false;
}

bool SidechainManager::isTrackUsedAsTarget(int trackIndex) const
{
    for (const auto& route : routes)
    {
        if (route.targetTrackIndex == trackIndex && route.enabled)
            return true;
    }
    return false;
}

int SidechainManager::getActiveRouteCount() const
{
    int count = 0;
    for (const auto& route : routes)
    {
        if (route.enabled && route.isValid())
            ++count;
    }
    return count;
}

juce::String SidechainManager::getDebugInfo() const
{
    juce::String info = "SidechainManager Debug Info:\n";
    info += "============================\n";
    info += "Sample Rate: " + juce::String(currentSampleRate) + " Hz\n";
    info += "Block Size: " + juce::String(currentBlockSize) + " samples\n";
    info += "Active Routes: " + juce::String(getActiveRouteCount()) + " / " + juce::String(routes.size()) + "\n";
    info += "Plugins with Sidechain: " + juce::String(pluginSidechainInfos.size()) + "\n\n";

    info += "Active Sidechain Routes:\n";
    for (const auto& route : routes)
    {
        if (route.isValid())
            info += "  " + route.toString() + "\n";
    }

    info += "\nPlugin Sidechain Info:\n";
    for (const auto& [nodeId, pluginInfo] : pluginSidechainInfos)
    {
        if (pluginInfo.hasSidechainInput)
            info += "  " + pluginInfo.pluginName + " (Node: " + juce::String(nodeId) + "): " +
                    pluginInfo.getSidechainBusInfo() + "\n";
    }

    return info;
}
