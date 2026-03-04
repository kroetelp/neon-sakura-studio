#include "MIDIOutputHandler.h"
#include <juce_core/juce_core.h>

//==============================================================================
// MIDIMessageQueue Implementation
//==============================================================================
MIDIMessageQueue::MIDIMessageQueue()
    : readIndex(0), writeIndex(0)
{
    for (int i = 0; i < maxMessagesPerBlock; ++i)
        messageBuffer[i] = juce::MidiMessage();
}

MIDIMessageQueue::~MIDIMessageQueue()
{
}

void MIDIMessageQueue::addMessage(const juce::MidiMessage& message)
{
    int currentCount = messageCount.load();
    if (currentCount >= maxMessagesPerBlock)
        return;  // Queue full - drop message (real-time safety)

    // Add message to buffer (lock-free write)
    messageBuffer[writeIndex] = message;
    writeIndex = (writeIndex + 1) % maxMessagesPerBlock;
    messageCount.fetch_add(1);
}

void MIDIMessageQueue::getMessages(juce::MidiBuffer& destination)
{
    int count = messageCount.exchange(0);  // Atomic exchange - clear and get

    for (int i = 0; i < count; ++i)
    {
        destination.addEvent(messageBuffer[readIndex], 0);
        readIndex = (readIndex + 1) % maxMessagesPerBlock;
    }
}

void MIDIMessageQueue::clear()
{
    messageCount.store(0);
    readIndex = 0;
    writeIndex = 0;
}

//==============================================================================
// MIDIOutputHandler Implementation
//==============================================================================
MIDIOutputHandler::MIDIOutputHandler()
{
}

MIDIOutputHandler::~MIDIOutputHandler()
{
    clearAllQueues();
}

void MIDIOutputHandler::initialize(int numTracks_)
{
    numTracks = juce::jmin(numTracks_, maxTracks);

    // Allocate MIDI queues for each track
    midiQueues.clear();
    for (int i = 0; i < numTracks; ++i)
    {
        midiQueues.push_back(std::make_unique<MIDIMessageQueue>());
    }

    // Allocate original MIDI buffers for MIDI-Thru
    originalMidiBuffers.resize(numTracks);
    for (auto& buffer : originalMidiBuffers)
    {
        buffer.clear();
    }
}

void MIDIOutputHandler::addMIDIOutput(int sourceTrackIndex, const juce::MidiBuffer& midiBuffer)
{
    if (sourceTrackIndex < 0 || sourceTrackIndex >= numTracks)
        return;

    // Add all messages from the plugin's MIDI output to the source queue
    for (const auto& message : midiBuffer)
    {
        midiQueues[sourceTrackIndex]->addMessage(message.getMessage());
    }
}

void MIDIOutputHandler::getMIDIForTrack(int targetTrackIndex, juce::MidiBuffer& destination)
{
    if (targetTrackIndex < 0 || targetTrackIndex >= numTracks)
        return;

    destination.clear();

    // Collect MIDI from all source tracks that have routes to this target
    for (const auto& route : routes)
    {
        if (!route.isValid() || route.targetTrackIndex != targetTrackIndex)
            continue;

        int sourceTrackIndex = route.sourceTrackIndex;
        if (sourceTrackIndex < 0 || sourceTrackIndex >= numTracks)
            continue;

        // Get MIDI from source queue
        juce::MidiBuffer tempBuffer;
        midiQueues[sourceTrackIndex]->getMessages(tempBuffer);

        // Filter by channel if specified
        if (route.channelFilter >= 0 && route.channelFilter <= 15)
        {
            for (const auto metadata : tempBuffer)
            {
                if (metadata.getMessage().getChannel() == route.channelFilter)
                    destination.addEvent(metadata.getMessage(), metadata.samplePosition);
            }
        }
        else
        {
            // No channel filter - add all messages
            destination.addEvents(tempBuffer, 0, -1, 0);
        }
    }

    // MIDI-Thru: Add original MIDI from sequencer (if enabled)
    if (midiThruEnabled.load())
    {
        int channelFilter = midiThruChannelFilter.load();

        if (channelFilter >= 0 && channelFilter <= 15)
        {
            // Filter by channel
            for (const auto metadata : originalMidiBuffers[targetTrackIndex])
            {
                if (metadata.getMessage().getChannel() == channelFilter)
                    destination.addEvent(metadata.getMessage(), metadata.samplePosition);
            }
        }
        else
        {
            // No channel filter - add all messages
            destination.addEvents(originalMidiBuffers[targetTrackIndex], 0, -1, 0);
        }
    }
}

void MIDIOutputHandler::addOriginalMidi(int trackIndex, const juce::MidiBuffer& midiBuffer)
{
    if (trackIndex < 0 || trackIndex >= numTracks)
        return;

    originalMidiBuffers[trackIndex] = midiBuffer;
}

void MIDIOutputHandler::clearAllQueues()
{
    for (auto& queue : midiQueues)
    {
        if (queue)
            queue->clear();
    }

    for (auto& buffer : originalMidiBuffers)
    {
        buffer.clear();
    }
}

//==============================================================================
// Routing-Konfiguration

int MIDIOutputHandler::addRoute(const MIDIRoute& route)
{
    if (!route.isValid())
        return -1;

    // Check if route already exists
    for (const auto& existingRoute : routes)
    {
        if (existingRoute.sourceTrackIndex == route.sourceTrackIndex &&
            existingRoute.targetTrackIndex == route.targetTrackIndex &&
            existingRoute.channelFilter == route.channelFilter)
        {
            // Route bereits vorhanden, nur aktualisieren
            return existingRoute.sourceTrackIndex;  // Use as ID
        }
    }

    // Neue Route hinzufügen
    MIDIRoute newRoute = route;
    newRoute.sourceTrackIndex = nextRouteId++;  // Speichere Route-ID in sourceTrackIndex (Workaround)
    routes.push_back(newRoute);

    DBG("MIDI route added: " + newRoute.toString());

    return nextRouteId - 1;
}

bool MIDIOutputHandler::removeRoute(int routeId)
{
    auto it = std::find_if(routes.begin(), routes.end(),
        [routeId](const MIDIRoute& r) {
            return r.sourceTrackIndex == routeId;
        });

    if (it != routes.end())
    {
        DBG("MIDI route removed: " + it->toString());
        routes.erase(it);
        return true;
    }

    return false;
}

bool MIDIOutputHandler::setRouteEnabled(int routeId, bool enabled)
{
    auto* route = const_cast<MIDIRoute*>(getRoute(routeId));
    if (route)
    {
        route->enabled = enabled;
        DBG("MIDI route " + juce::String(routeId) + " " + (enabled ? "enabled" : "disabled"));
        return true;
    }
    return false;
}

const MIDIRoute* MIDIOutputHandler::getRoute(int routeId) const
{
    auto it = std::find_if(routes.begin(), routes.end(),
        [routeId](const MIDIRoute& r) {
            return r.sourceTrackIndex == routeId;
        });

    return (it != routes.end()) ? &(*it) : nullptr;
}

std::vector<MIDIRoute*> MIDIOutputHandler::getRoutesForTarget(int targetTrackIndex)
{
    std::vector<MIDIRoute*> result;
    for (auto& route : routes)
    {
        if (route.targetTrackIndex == targetTrackIndex && route.isValid())
            result.push_back(&route);
    }
    return result;
}

std::vector<const MIDIRoute*> MIDIOutputHandler::getRoutesForTarget(int targetTrackIndex) const
{
    std::vector<const MIDIRoute*> result;
    for (const auto& route : routes)
    {
        if (route.targetTrackIndex == targetTrackIndex && route.isValid())
            result.push_back(&route);
    }
    return result;
}

//==============================================================================
// Plugin MIDI-Output-Fähigkeiten

void MIDIOutputHandler::registerPluginMIDIInfo(const PluginMIDIOutputInfo& info)
{
    if (info.nodeId != 0)
    {
        pluginMidiInfos[info.nodeId] = info;
        DBG("Plugin MIDI info registered: " + info.pluginName + " (Node: " +
            juce::String(info.nodeId) + ")");
    }
}

const PluginMIDIOutputInfo* MIDIOutputHandler::getPluginMIDIInfo(uint32_t nodeId) const
{
    auto it = pluginMidiInfos.find(nodeId);
    return (it != pluginMidiInfos.end()) ? &(it->second) : nullptr;
}

std::vector<PluginMIDIOutputInfo> MIDIOutputHandler::getPluginsWithMIDIOutput() const
{
    std::vector<PluginMIDIOutputInfo> result;

    for (const auto& [nodeId, info] : pluginMidiInfos)
    {
        if (info.producesMidi)
            result.push_back(info);
    }

    return result;
}

//==============================================================================
// UI-Visualisierung

bool MIDIOutputHandler::isTrackUsedAsMidiSource(int trackIndex) const
{
    for (const auto& route : routes)
    {
        if (route.sourceTrackIndex == trackIndex && route.enabled)
            return true;
    }
    return false;
}

bool MIDIOutputHandler::isTrackUsedAsMidiTarget(int trackIndex) const
{
    for (const auto& route : routes)
    {
        if (route.targetTrackIndex == trackIndex && route.enabled)
            return true;
    }
    return false;
}

int MIDIOutputHandler::getActiveRouteCount() const
{
    int count = 0;
    for (const auto& route : routes)
    {
        if (route.enabled && route.isValid())
            ++count;
    }
    return count;
}

juce::String MIDIOutputHandler::getDebugInfo() const
{
    juce::String info = "MIDIOutputHandler Debug Info:\n";
    info += "=============================\n";
    info += "Num Tracks: " + juce::String(numTracks) + "\n";
    info += "MIDI-Thru: " + juce::String(midiThruEnabled.load() ? "Enabled" : "Disabled") + "\n";
    info += "MIDI-Thru Channel: " + juce::String(midiThruChannelFilter.load() + 1) + "\n";
    info += "Active Routes: " + juce::String(getActiveRouteCount()) + " / " + juce::String(routes.size()) + "\n";
    info += "Plugins with MIDI: " + juce::String(pluginMidiInfos.size()) + "\n\n";

    info += "Active MIDI Routes:\n";
    for (const auto& route : routes)
    {
        if (route.isValid())
            info += "  " + route.toString() + "\n";
    }

    info += "\nPlugin MIDI Info:\n";
    for (const auto& [nodeId, pluginInfo] : pluginMidiInfos)
    {
        if (pluginInfo.producesMidi)
            info += "  " + pluginInfo.pluginName + " (Node: " + juce::String(nodeId) + "): " +
                    pluginInfo.getDescription() + "\n";
    }

    return info;
}
