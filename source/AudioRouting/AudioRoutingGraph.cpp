#include "AudioRoutingGraph.h"
#include "TrackProcessor.h"
#include "MasterOutputProcessor.h"
#include "../ITrackDataProvider.h"
#include "../VSTHost/PluginInstance.h"
#include "SidechainManager.h"
#include "MIDIOutputHandler.h"
#include "PluginParameterAutomation.h"

//==============================================================================
// LoadedPlugin method implementations
//==============================================================================
juce::String LoadedPlugin::getName() const
{
    if (pluginInstanceWrapper)
        return pluginInstanceWrapper->getName();
    return pluginInstance ? pluginInstance->getName() : "Unknown";
}

bool LoadedPlugin::isValid() const
{
    return pluginInstanceWrapper && pluginInstanceWrapper->getAudioPluginInstance();
}

//==============================================================================
AudioRoutingGraph::AudioRoutingGraph()
{
    for (auto& id : trackNodeIDs)
        id = 0;

    // Create sidechain manager
    sidechainManager = std::make_unique<SidechainManager>();

    // Create MIDI output handler
    midiOutputHandler = std::make_unique<MIDIOutputHandler>();

    // Create plugin parameter automation
    pluginParameterAutomation = std::make_unique<PluginParameterAutomation>();
}

//==============================================================================
AudioRoutingGraph::~AudioRoutingGraph()
{
    pluginNodes.clear();
    for (auto& processor : trackProcessors)
        processor.reset();
    masterOutputProcessor.reset();
    internalSynthProcessor.reset();
}

//==============================================================================
void AudioRoutingGraph::initialize(ITrackDataProvider* provider, WavetableEngine* wavetable)
{
    if (initialized)
        return;

    trackProvider = provider;
    wavetableEngine = wavetable;

    // Create Master Output Processor
    masterOutputProcessor = std::make_unique<MasterOutputProcessor>();

    // Create Track Processors
    for (int i = 0; i < maxTracks; ++i)
    {
        trackProcessors[i] = std::make_unique<TrackProcessor>(i, trackProvider);
        trackNodeIDs[i] = static_cast<NodeID>(i + 1);
    }

    // Initialize MIDI output handler
    if (midiOutputHandler)
        midiOutputHandler->initialize(maxTracks);

    initialized = true;
}

//==============================================================================
void AudioRoutingGraph::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    if (!initialized)
        return;

    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Use maximum buffer size to prevent dynamic allocation in audio thread
    const int safeBufferSize = juce::jmax(samplesPerBlock, maxBufferSize);

    // Pre-allocate temp buffer (NO allocation during playback!)
    tempBuffer = std::make_unique<juce::AudioBuffer<float>>(2, safeBufferSize);
    tempMidiBuffer.ensureSize(2048);  // Pre-allocate MIDI buffer

    // Prepare delay lines for latency compensation (max 8192 samples = ~185ms at 44.1kHz)
    for (auto& delayLine : delayLines)
    {
        delayLine.prepare(sampleRate, 8192, 2);
    }

    masterOutputProcessor->prepareToPlay(sampleRate, samplesPerBlock);

    if (internalSynthProcessor)
        internalSynthProcessor->prepareToPlay(sampleRate, samplesPerBlock);

    for (auto& trackProc : trackProcessors)
    {
        if (trackProc)
            trackProc->prepareToPlay(sampleRate, samplesPerBlock);
    }

    for (auto& [id, plugin] : pluginNodes)
    {
        if (plugin)
            plugin->prepareToPlay(sampleRate, samplesPerBlock);
    }

    // Update latency compensation after all plugins are prepared
    updateLatencyCompensation();

    // Prepare sidechain manager
    if (sidechainManager)
        sidechainManager->prepare(sampleRate, samplesPerBlock);
}

//==============================================================================
void AudioRoutingGraph::releaseResources()
{
    if (!initialized)
        return;

    // Release pre-allocated buffers
    tempBuffer.reset();
    tempMidiBuffer.clear();

    // Reset delay lines
    for (auto& delayLine : delayLines)
    {
        delayLine.reset();
    }

    masterOutputProcessor->releaseResources();

    if (internalSynthProcessor)
        internalSynthProcessor->releaseResources();

    for (auto& trackProc : trackProcessors)
    {
        if (trackProc)
            trackProc->releaseResources();
    }

    for (auto& [id, plugin] : pluginNodes)
    {
        if (plugin)
            plugin->releaseResources();
    }

    // Release sidechain manager resources
    if (sidechainManager)
        sidechainManager->releaseResources();

    // Clear MIDI output queues
    if (midiOutputHandler)
        midiOutputHandler->clearAllQueues();
}

//==============================================================================
void AudioRoutingGraph::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& midiMessages)
{
    if (!initialized)
    {
        buffer.clear();
        return;
    }

    buffer.clear();

    // Safety check: ensure pre-allocated buffer is large enough
    if (!tempBuffer || tempBuffer->getNumSamples() < buffer.getNumSamples())
    {
        jassertfalse;  // This should never happen - buffer was pre-allocated
        return;
    }

    // Clear pre-allocated buffers (no heap allocation!)
    tempBuffer->clear();
    tempMidiBuffer.clear();

    // Clear all sidechain buffers at the start of each block
    if (sidechainManager)
        sidechainManager->clearAllBuffers();

    // Clear all MIDI output queues at the start of each block
    if (midiOutputHandler)
        midiOutputHandler->clearAllQueues();

    // Process each track and mix into output
    for (int i = 0; i < maxTracks; ++i)
    {
        if (!trackProcessors[i])
            continue;

        // Check mute/solo state
        bool anySolo = false;
        for (int j = 0; j < maxTracks; ++j)
        {
            if (trackProvider && trackProvider->getSolo(j))
            {
                anySolo = true;
                break;
            }
        }

        if (trackProvider)
        {
            if (trackProvider->getMuted(i) || (anySolo && !trackProvider->getSolo(i)))
                continue;
        }

        // Use pre-allocated temp buffer
        tempBuffer->clear();
        tempMidiBuffer.clear();

        // Store original MIDI for MIDI-Thru (before processing)
        juce::MidiBuffer originalMidi = tempMidiBuffer;

        trackProcessors[i]->processBlock(*tempBuffer, tempMidiBuffer);

        // Add this track's audio to sidechain buffers (if this track is a sidechain source)
        if (sidechainManager)
            sidechainManager->addSourceToBuffer(i, *tempBuffer, buffer.getNumSamples());

        // Add original MIDI to MIDI-Thru handler (for MIDI-Thru functionality)
        if (midiOutputHandler && !originalMidi.isEmpty())
        {
            midiOutputHandler->addOriginalMidi(i, originalMidi);
        }

        // Get MIDI output from other tracks (MIDI routing)
        if (midiOutputHandler)
        {
            juce::MidiBuffer routedMidi;
            midiOutputHandler->getMIDIForTrack(i, routedMidi);
            if (!routedMidi.isEmpty())
            {
                tempMidiBuffer.addEvents(routedMidi, 0, -1, 0);
            }
        }

        // Process plugins in this track's chain (in order)
        auto& chain = trackPluginChains[i];

        // Apply plugin parameter automation (before processing plugins)
        std::unordered_map<uint32_t, std::unordered_map<int, float>> automationValues{};
        if (pluginParameterAutomation && pluginParameterAutomation->isEnabled())
        {
            // Get current playhead position (in beats) - JUCE 8.0 API
            double currentBeat = 0.0;
            if (currentPlayHead)
            {
                if (auto position = currentPlayHead->getPosition())
                {
                    if (auto bpm = position->getBpm())
                        currentBeat = *bpm * 4.0;  // Approximate beats per quarter note
                }
            }

            // Process automation for all plugins in this track
            for (NodeID pluginId : chain.pluginNodeIds)
            {
                auto trackAutomationValues = pluginParameterAutomation->processAutomation(currentBeat);

                // Check if this track has automation for this plugin
                auto trackValues = trackAutomationValues.find(pluginId);
                if (trackValues != trackAutomationValues.end())
                {
                    automationValues[pluginId] = trackValues->second;
                }
            }
        }

        for (NodeID pluginId : chain.pluginNodeIds)
        {
            // First check if this is a PluginInstance wrapper
            auto wrapperIt = pluginInstanceWrappers.find(pluginId);
            if (wrapperIt != pluginInstanceWrappers.end() && wrapperIt->second)
            {
                auto* audioPlugin = wrapperIt->second->getAudioPluginInstance();
                if (audioPlugin && !wrapperIt->second->isBypassed())
                {
                    // Apply plugin parameter automation (before processing)
                    if (!automationValues.empty())
                    {
                        auto paramValues = automationValues.find(pluginId);
                        if (paramValues != automationValues.end())
                        {
                            for (const auto& [paramIndex, value] : paramValues->second)
                            {
                                // Apply automation value to plugin parameter
                                audioPlugin->getParameters()[paramIndex]->setValueNotifyingHost(value);
                            }
                        }
                    }

                    audioPlugin->processBlock(*tempBuffer, tempMidiBuffer);
                }
                continue;
            }

            // Check if this is a non-owning plugin reference
            auto refIt = pluginReferences.find(pluginId);
            if (refIt != pluginReferences.end() && refIt->second)
            {
                // Apply plugin parameter automation (before processing)
                if (!automationValues.empty())
                {
                    auto paramValues = automationValues.find(pluginId);
                    if (paramValues != automationValues.end())
                    {
                        for (const auto& [paramIndex, value] : paramValues->second)
                        {
                            refIt->second->getParameters()[paramIndex]->setValueNotifyingHost(value);
                        }
                    }
                }

                refIt->second->processBlock(*tempBuffer, tempMidiBuffer);
                continue;
            }

            // Fallback to raw plugin nodes (legacy support)
            auto it = pluginNodes.find(pluginId);
            if (it != pluginNodes.end() && it->second)
            {
                // Apply plugin parameter automation (before processing)
                if (!automationValues.empty())
                {
                    auto paramValues = automationValues.find(pluginId);
                    if (paramValues != automationValues.end())
                    {
                        for (const auto& [paramIndex, value] : paramValues->second)
                        {
                            it->second->getParameters()[paramIndex]->setValueNotifyingHost(value);
                        }
                    }
                }

                it->second->processBlock(*tempBuffer, tempMidiBuffer);
            }
        }

        // Capture MIDI output from plugins in this track
        if (midiOutputHandler && !tempMidiBuffer.isEmpty())
        {
            midiOutputHandler->addMIDIOutput(i, tempMidiBuffer);
        }

        // Apply latency compensation delay to this track
        // This ensures all tracks are aligned, even if some have plugins with latency
        if (maxLatency > 0 && trackLatencies[i] < maxLatency)
        {
            delayLines[i].process(*tempBuffer, buffer.getNumSamples());
        }

        // Mix track into master buffer with track volume
        float trackVolume = trackProvider ? trackProvider->getVolume(i) : 0.8f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.addFrom(ch, 0, *tempBuffer, ch, 0, buffer.getNumSamples(), trackVolume);
        }
    }

    // Process internal synth (if enabled)
    if (internalSynthProcessor)
    {
        tempBuffer->clear();
        tempMidiBuffer.clear();

        internalSynthProcessor->processBlock(*tempBuffer, tempMidiBuffer);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.addFrom(ch, 0, *tempBuffer, ch, 0, buffer.getNumSamples());
        }
    }

    // Process master output (volume, reverb, limiting)
    masterOutputProcessor->processBlock(buffer, midiMessages);
}

//==============================================================================
bool AudioRoutingGraph::connectTrackToMaster(int trackIndex)
{
    return trackIndex >= 0 && trackIndex < maxTracks && trackProcessors[trackIndex] != nullptr;
}

//==============================================================================
bool AudioRoutingGraph::disconnectTrackFromMaster(int trackIndex)
{
    return trackIndex >= 0 && trackIndex < maxTracks;
}

//==============================================================================
bool AudioRoutingGraph::connectInternalSynthToMaster()
{
    return internalSynthProcessor != nullptr;
}

//==============================================================================
bool AudioRoutingGraph::disconnectInternalSynthFromMaster()
{
    return true;
}

//==============================================================================
NodeID AudioRoutingGraph::insertPluginInTrack(int trackIndex, int position,
                                                  std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    if (!plugin || trackIndex < 0 || trackIndex >= maxTracks)
        return 0;

    plugin->prepareToPlay(currentSampleRate, currentBlockSize);

    // Set the playhead so the plugin can sync with tempo/position
    if (currentPlayHead)
    {
        plugin->setPlayHead(currentPlayHead);
    }

    // Thread-safe NodeID generation using atomic
    NodeID nodeId = nextNodeId.fetch_add(1);

    pluginNodes[nodeId] = std::move(plugin);
    trackPluginChains[trackIndex].insertPlugin(nodeId, position);

    return nodeId;
}

//==============================================================================
NodeID AudioRoutingGraph::insertPluginAfterTrack(int trackIndex,
                                                  std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    // Legacy method: inserts at the end of the track's plugin chain
    return insertPluginInTrack(trackIndex, -1, std::move(plugin));
}

//==============================================================================
NodeID AudioRoutingGraph::insertPluginInstanceInTrack(int trackIndex, int position,
                                                       std::shared_ptr<PluginInstance> pluginInstance)
{
    if (!pluginInstance || trackIndex < 0 || trackIndex >= maxTracks)
        return 0;

    auto* audioPlugin = pluginInstance->getAudioPluginInstance();
    if (!audioPlugin)
        return 0;

    // Prepare the plugin for playback
    audioPlugin->prepareToPlay(currentSampleRate, currentBlockSize);

    // Set the playhead so the plugin can sync with tempo/position
    if (currentPlayHead)
    {
        audioPlugin->setPlayHead(currentPlayHead);
    }

    // Thread-safe NodeID generation using atomic
    NodeID nodeId = nextNodeId.fetch_add(1);

    // Store the plugin instance wrapper (shared_ptr keeps it alive)
    pluginInstanceWrappers[nodeId] = pluginInstance;

    // Extract and store the raw AudioPluginInstance for processing
    // Note: We don't take ownership here - PluginInstance still owns it
    // We store a non-owning reference via the wrapper
    trackPluginChains[trackIndex].insertPlugin(nodeId, position);

    // Scan plugin parameters for automation (Phase 4.3)
    scanPluginParameters();

    // Scan plugin sidechain capabilities (Phase 4.1)
    scanPluginSidechains();

    return nodeId;
}

//==============================================================================
bool AudioRoutingGraph::removePluginNode(NodeID nodeId)
{
    // Find which track owns this plugin and remove it from the chain
    for (int i = 0; i < maxTracks; ++i)
    {
        if (trackPluginChains[i].removePlugin(nodeId))
        {
            pluginNodes.erase(nodeId);
            pluginInstanceWrappers.erase(nodeId);
            pluginReferences.erase(nodeId);

            // Scan plugin parameters for automation after removal (Phase 4.3)
            scanPluginParameters();

            // Scan plugin sidechain capabilities after removal (Phase 4.1)
            scanPluginSidechains();

            return true;
        }
    }

    // Plugin not found in any track chain, but might still be in storage
    pluginNodes.erase(nodeId);
    pluginInstanceWrappers.erase(nodeId);
    pluginReferences.erase(nodeId);

    // Scan plugin parameters for automation after removal (Phase 4.3)
    scanPluginParameters();

    return false;
}

//==============================================================================
bool AudioRoutingGraph::removePluginFromTrack(int trackIndex, NodeID nodeId)
{
    if (trackIndex < 0 || trackIndex >= maxTracks)
        return false;

    if (trackPluginChains[trackIndex].removePlugin(nodeId))
    {
        pluginNodes.erase(nodeId);
        pluginInstanceWrappers.erase(nodeId);
        pluginReferences.erase(nodeId);
        return true;
    }

    return false;
}

//==============================================================================
NodeID AudioRoutingGraph::insertPluginReference(int trackIndex, int position,
                                                 juce::AudioPluginInstance* pluginInstance)
{
    if (!pluginInstance || trackIndex < 0 || trackIndex >= maxTracks)
        return 0;

    // Prepare the plugin for playback if not already prepared
    pluginInstance->prepareToPlay(currentSampleRate, currentBlockSize);

    // Set the playhead so the plugin can sync with tempo/position
    if (currentPlayHead)
    {
        pluginInstance->setPlayHead(currentPlayHead);
    }

    // Thread-safe NodeID generation using atomic
    NodeID nodeId = nextNodeId.fetch_add(1);

    // Store non-owning reference
    pluginReferences[nodeId] = pluginInstance;
    trackPluginChains[trackIndex].insertPlugin(nodeId, position);

    return nodeId;
}

//==============================================================================
TrackPluginChain* AudioRoutingGraph::getTrackPluginChain(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
        return &trackPluginChains[trackIndex];
    return nullptr;
}

//==============================================================================
const TrackPluginChain* AudioRoutingGraph::getTrackPluginChain(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
        return &trackPluginChains[trackIndex];
    return nullptr;
}

//==============================================================================
int AudioRoutingGraph::getPluginTrackIndex(NodeID nodeId) const
{
    for (int i = 0; i < maxTracks; ++i)
    {
        const auto& chain = trackPluginChains[i];
        if (std::find(chain.pluginNodeIds.begin(), chain.pluginNodeIds.end(), nodeId)
            != chain.pluginNodeIds.end())
        {
            return i;
        }
    }
    return -1;
}

//==============================================================================
juce::AudioPluginInstance* AudioRoutingGraph::getPlugin(NodeID nodeId)
{
    auto it = pluginNodes.find(nodeId);
    return it != pluginNodes.end() ? it->second.get() : nullptr;
}

//==============================================================================
const juce::AudioPluginInstance* AudioRoutingGraph::getPlugin(NodeID nodeId) const
{
    auto it = pluginNodes.find(nodeId);
    return it != pluginNodes.end() ? it->second.get() : nullptr;
}

//==============================================================================
TrackProcessor* AudioRoutingGraph::getTrackProcessor(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
        return trackProcessors[trackIndex].get();
    return nullptr;
}

//==============================================================================
const TrackProcessor* AudioRoutingGraph::getTrackProcessor(int trackIndex) const
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
        return trackProcessors[trackIndex].get();
    return nullptr;
}

//==============================================================================
std::unique_ptr<juce::XmlElement> AudioRoutingGraph::getState() const
{
    auto xml = std::make_unique<juce::XmlElement>("AUDIO_ROUTING_GRAPH");
    xml->setAttribute("sampleRate", currentSampleRate);
    xml->setAttribute("blockSize", currentBlockSize);

    if (masterOutputProcessor)
    {
        juce::MemoryBlock state;
        masterOutputProcessor->getStateInformation(state);
        auto* child = xml->createNewChildElement("MASTER");
        child->setAttribute("state", state.toBase64Encoding());
    }

    // Save per-track plugin chains with their states
    for (int trackIdx = 0; trackIdx < maxTracks; ++trackIdx)
    {
        const auto& chain = trackPluginChains[trackIdx];
        if (chain.empty())
            continue;

        auto* trackElement = xml->createNewChildElement("TRACK_PLUGINS");
        trackElement->setAttribute("trackIndex", trackIdx);

        for (size_t pos = 0; pos < chain.size(); ++pos)
        {
            NodeID pluginId = chain.pluginNodeIds[pos];

            // First check PluginInstance wrappers (primary storage)
            auto wrapperIt = pluginInstanceWrappers.find(pluginId);
            if (wrapperIt != pluginInstanceWrappers.end() && wrapperIt->second)
            {
                auto* pluginElement = trackElement->createNewChildElement("PLUGIN");
                pluginElement->setAttribute("id", static_cast<int>(pluginId));
                pluginElement->setAttribute("position", static_cast<int>(pos));
                pluginElement->setAttribute("name", wrapperIt->second->getName());
                pluginElement->setAttribute("bypassed", wrapperIt->second->isBypassed());
                pluginElement->setAttribute("type", "PluginInstance");

                // Save plugin state from PluginInstance wrapper
                juce::MemoryBlock pluginState;
                wrapperIt->second->getState(pluginState);
                pluginElement->setAttribute("state", pluginState.toBase64Encoding());

                // Save plugin description for reloading
                juce::PluginDescription desc = wrapperIt->second->getDescription();
                auto* descElement = pluginElement->createNewChildElement("DESCRIPTION");
                descElement->setAttribute("fileOrIdentifier", desc.fileOrIdentifier);
                descElement->setAttribute("name", desc.name);
                descElement->setAttribute("pluginFormatName", desc.pluginFormatName);
                descElement->setAttribute("manufacturerName", desc.manufacturerName);
                descElement->setAttribute("version", desc.version);
                descElement->setAttribute("isInstrument", desc.isInstrument);
                descElement->setAttribute("numInputChannels", desc.numInputChannels);
                descElement->setAttribute("numOutputChannels", desc.numOutputChannels);
                descElement->setAttribute("uniqueId", desc.uniqueId);
                continue;
            }

            // Fallback to non-owning references
            auto refIt = pluginReferences.find(pluginId);
            if (refIt != pluginReferences.end() && refIt->second)
            {
                auto* pluginElement = trackElement->createNewChildElement("PLUGIN");
                pluginElement->setAttribute("id", static_cast<int>(pluginId));
                pluginElement->setAttribute("position", static_cast<int>(pos));
                pluginElement->setAttribute("name", refIt->second->getName());
                pluginElement->setAttribute("type", "Reference");

                // Save plugin state
                juce::MemoryBlock pluginState;
                refIt->second->getStateInformation(pluginState);
                pluginElement->setAttribute("state", pluginState.toBase64Encoding());

                // Save plugin description for reloading
                juce::PluginDescription desc;
                refIt->second->fillInPluginDescription(desc);
                auto* descElement = pluginElement->createNewChildElement("DESCRIPTION");
                descElement->setAttribute("fileOrIdentifier", desc.fileOrIdentifier);
                descElement->setAttribute("name", desc.name);
                descElement->setAttribute("pluginFormatName", desc.pluginFormatName);
                descElement->setAttribute("manufacturerName", desc.manufacturerName);
                descElement->setAttribute("version", desc.version);
                descElement->setAttribute("isInstrument", desc.isInstrument);
                descElement->setAttribute("numInputChannels", desc.numInputChannels);
                descElement->setAttribute("numOutputChannels", desc.numOutputChannels);
                descElement->setAttribute("uniqueId", desc.uniqueId);
                continue;
            }

            // Fallback to raw plugin nodes (legacy support)
            auto it = pluginNodes.find(pluginId);
            if (it != pluginNodes.end() && it->second)
            {
                auto* pluginElement = trackElement->createNewChildElement("PLUGIN");
                pluginElement->setAttribute("id", static_cast<int>(pluginId));
                pluginElement->setAttribute("position", static_cast<int>(pos));
                pluginElement->setAttribute("name", it->second->getName());
                pluginElement->setAttribute("type", "Raw");

                // Save plugin state
                juce::MemoryBlock pluginState;
                it->second->getStateInformation(pluginState);
                pluginElement->setAttribute("state", pluginState.toBase64Encoding());

                // Save plugin description for reloading
                juce::PluginDescription desc;
                it->second->fillInPluginDescription(desc);
                auto* descElement = pluginElement->createNewChildElement("DESCRIPTION");
                descElement->setAttribute("fileOrIdentifier", desc.fileOrIdentifier);
                descElement->setAttribute("name", desc.name);
                descElement->setAttribute("pluginFormatName", desc.pluginFormatName);
                descElement->setAttribute("manufacturerName", desc.manufacturerName);
                descElement->setAttribute("version", desc.version);
                descElement->setAttribute("isInstrument", desc.isInstrument);
                descElement->setAttribute("numInputChannels", desc.numInputChannels);
                descElement->setAttribute("numOutputChannels", desc.numOutputChannels);
                descElement->setAttribute("uniqueId", desc.uniqueId);
            }
        }
    }

    return xml;
}

//==============================================================================
void AudioRoutingGraph::setState(const juce::XmlElement& xml, PluginLoadCallback pluginLoadCallback)
{
    // Clear existing plugins from all tracks
    for (auto& chain : trackPluginChains)
        chain.clear();
    pluginNodes.clear();
    pluginInstanceWrappers.clear();
    pluginReferences.clear();

    // Restore master state
    if (auto* masterElement = xml.getChildByName("MASTER"))
    {
        if (masterOutputProcessor)
        {
            auto stateString = masterElement->getStringAttribute("state");
            if (stateString.isNotEmpty())
            {
                juce::MemoryBlock state;
                state.fromBase64Encoding(stateString);
                masterOutputProcessor->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            }
        }
    }

    // Restore per-track plugin chains
    if (pluginLoadCallback)
    {
        for (auto* trackElement : xml.getChildWithTagNameIterator("TRACK_PLUGINS"))
        {
            int trackIndex = trackElement->getIntAttribute("trackIndex", -1);
            if (trackIndex < 0 || trackIndex >= maxTracks)
                continue;

            for (auto* pluginElement : trackElement->getChildIterator())
            {
                if (pluginElement->getTagName() != "PLUGIN")
                    continue;

                auto* descElement = pluginElement->getChildByName("DESCRIPTION");
                if (!descElement)
                    continue;

                juce::String type = pluginElement->getStringAttribute("type", "PluginInstance");
                juce::String fileOrIdentifier = descElement->getStringAttribute("fileOrIdentifier");
                juce::String formatName = descElement->getStringAttribute("pluginFormatName");

                // Try to load the plugin
                auto plugin = pluginLoadCallback(fileOrIdentifier, formatName);
                if (!plugin)
                {
                    DBG("Failed to restore plugin: " + fileOrIdentifier);
                    continue;
                }

                // Prepare plugin
                plugin->prepareToPlay(currentSampleRate, currentBlockSize);

                // Set playhead
                if (currentPlayHead)
                {
                    plugin->setPlayHead(currentPlayHead);
                }

                int position = pluginElement->getIntAttribute("position", -1);
                NodeID nodeId = nextNodeId.fetch_add(1);

                // Store based on plugin type
                if (type == "PluginInstance")
                {
                    // Create PluginInstance wrapper and store
                    auto pluginInstanceWrapper = std::make_shared<PluginInstance>(std::move(plugin));
                    pluginInstanceWrappers[nodeId] = pluginInstanceWrapper;
                }
                else if (type == "Raw")
                {
                    // Raw AudioPluginInstance (legacy)
                    pluginNodes[nodeId] = std::move(plugin);
                }
                else
                {
                    // Reference type - store non-owning pointer
                    pluginReferences[nodeId] = plugin.get();
                    pluginNodes[nodeId] = std::move(plugin);
                }

                // Add to track chain
                trackPluginChains[trackIndex].insertPlugin(nodeId, position);

                // Restore plugin state
                auto stateString = pluginElement->getStringAttribute("state");
                if (stateString.isNotEmpty())
                {
                    juce::MemoryBlock state;
                    state.fromBase64Encoding(stateString);

                    // Restore based on plugin type
                    auto wrapperIt = pluginInstanceWrappers.find(nodeId);
                    if (wrapperIt != pluginInstanceWrappers.end())
                    {
                        wrapperIt->second->setState(state.getData(), static_cast<int>(state.getSize()));
                    }
                    else
                    {
                        auto it = pluginNodes.find(nodeId);
                        if (it != pluginNodes.end())
                        {
                            it->second->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
                        }
                    }
                }

                // Restore bypass state
                bool bypassed = pluginElement->getBoolAttribute("bypassed", false);
                auto wrapperIt = pluginInstanceWrappers.find(nodeId);
                if (wrapperIt != pluginInstanceWrappers.end())
                {
                    wrapperIt->second->setBypassed(bypassed);
                }
            }
        }

        // Update latency compensation after all plugins are restored
        updateLatencyCompensation();

        // Scan plugin parameters for automation after loading (Phase 4.3)
        scanPluginParameters();

        // Scan plugin sidechain capabilities after loading (Phase 4.1)
        scanPluginSidechains();
    }
}

//==============================================================================
void AudioRoutingGraph::setState(const juce::XmlElement& xml)
{
    // Legacy overload - only restores master state, not plugins
    if (auto* masterElement = xml.getChildByName("MASTER"))
    {
        if (masterOutputProcessor)
        {
            auto stateString = masterElement->getStringAttribute("state");
            if (stateString.isNotEmpty())
            {
                juce::MemoryBlock state;
                state.fromBase64Encoding(stateString);
                masterOutputProcessor->setStateInformation(state.getData(), static_cast<int>(state.getSize()));
            }
        }
    }
}

//==============================================================================
void AudioRoutingGraph::setPlayHead(juce::AudioPlayHead* playHead)
{
    currentPlayHead = playHead;

    // Update all existing raw plugin nodes with the new playhead
    for (auto& [id, plugin] : pluginNodes)
    {
        if (plugin)
        {
            plugin->setPlayHead(playHead);
        }
    }

    // Update all PluginInstance wrappers with the new playhead
    for (auto& [id, wrapper] : pluginInstanceWrappers)
    {
        if (wrapper)
        {
            auto* audioPlugin = wrapper->getAudioPluginInstance();
            if (audioPlugin)
            {
                audioPlugin->setPlayHead(playHead);
            }
        }
    }

    // Update all non-owning plugin references with the new playhead
    for (auto& [id, pluginRef] : pluginReferences)
    {
        if (pluginRef)
        {
            pluginRef->setPlayHead(playHead);
        }
    }
}

//==============================================================================
std::shared_ptr<PluginInstance> AudioRoutingGraph::getPluginInstance(NodeID nodeId)
{
    auto it = pluginInstanceWrappers.find(nodeId);
    return it != pluginInstanceWrappers.end() ? it->second : nullptr;
}

//==============================================================================
std::shared_ptr<const PluginInstance> AudioRoutingGraph::getPluginInstance(NodeID nodeId) const
{
    auto it = pluginInstanceWrappers.find(nodeId);
    return it != pluginInstanceWrappers.end() ? it->second : nullptr;
}

//==============================================================================
std::vector<std::shared_ptr<PluginInstance>> AudioRoutingGraph::getPluginInstancesForTrack(int trackIndex) const
{
    std::vector<std::shared_ptr<PluginInstance>> result;

    if (trackIndex < 0 || trackIndex >= maxTracks)
        return result;

    const auto& chain = trackPluginChains[trackIndex];
    for (NodeID pluginId : chain.pluginNodeIds)
    {
        auto it = pluginInstanceWrappers.find(pluginId);
        if (it != pluginInstanceWrappers.end() && it->second)
        {
            result.push_back(it->second);
        }
    }

    return result;
}

//==============================================================================
int AudioRoutingGraph::calculateTrackLatency(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= maxTracks)
        return 0;

    int totalLatency = 0;
    const auto& chain = trackPluginChains[trackIndex];

    for (NodeID pluginId : chain.pluginNodeIds)
    {
        // Check PluginInstance wrappers
        auto wrapperIt = pluginInstanceWrappers.find(pluginId);
        if (wrapperIt != pluginInstanceWrappers.end() && wrapperIt->second)
        {
            auto* audioPlugin = wrapperIt->second->getAudioPluginInstance();
            if (audioPlugin)
            {
                totalLatency += audioPlugin->getLatencySamples();
            }
            continue;
        }

        // Check non-owning references
        auto refIt = pluginReferences.find(pluginId);
        if (refIt != pluginReferences.end() && refIt->second)
        {
            totalLatency += refIt->second->getLatencySamples();
            continue;
        }

        // Check raw plugin nodes
        auto it = pluginNodes.find(pluginId);
        if (it != pluginNodes.end() && it->second)
        {
            totalLatency += it->second->getLatencySamples();
        }
    }

    return totalLatency;
}

//==============================================================================
int AudioRoutingGraph::getTrackLatency(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= maxTracks)
        return 0;
    return trackLatencies[trackIndex];
}

//==============================================================================
void AudioRoutingGraph::updateLatencyCompensation()
{
    // Calculate latency for each track
    maxLatency = 0;
    for (int i = 0; i < maxTracks; ++i)
    {
        trackLatencies[i] = calculateTrackLatency(i);
        maxLatency = std::max(maxLatency.load(), trackLatencies[i]);
    }

    // Update delay lines to compensate
    // Tracks with less latency need to be delayed to match the track with most latency
    for (int i = 0; i < maxTracks; ++i)
    {
        int delayNeeded = maxLatency - trackLatencies[i];
        delayLines[i].setDelay(delayNeeded);
    }

    if (maxLatency > 0)
    {
        DBG("Latency compensation updated: max latency = " + juce::String(maxLatency) + " samples");
    }
}

//==============================================================================
// Sidechain Routing Support
//==============================================================================

int AudioRoutingGraph::addSidechainRoute(int sourceTrackIndex, int targetTrackIndex,
                                       uint32_t targetPluginNodeId, int busIndex, float gain)
{
    if (!sidechainManager)
        return -1;

    SidechainRoute route;
    route.sourceTrackIndex = sourceTrackIndex;
    route.targetTrackIndex = targetTrackIndex;
    route.targetPluginNodeId = targetPluginNodeId;
    route.busIndex = busIndex;
    route.gain = gain;
    route.enabled = true;

    int routeId = sidechainManager->addRoute(route);
    if (routeId >= 0)
    {
        // Update sidechain buffer connection for target track
        if (auto* targetBuffer = sidechainManager->getSidechainBuffer(targetTrackIndex))
        {
            if (auto* targetProcessor = getTrackProcessor(targetTrackIndex))
                targetProcessor->setSidechainBuffer(targetBuffer);
        }

        DBG("Sidechain route added: Track " + juce::String(sourceTrackIndex) + " -> Track " +
            juce::String(targetTrackIndex) + " (ID: " + juce::String(routeId) + ")");
    }

    return routeId;
}

bool AudioRoutingGraph::removeSidechainRoute(int routeId)
{
    if (!sidechainManager)
        return false;

    return sidechainManager->removeRoute(routeId);
}

bool AudioRoutingGraph::updateSidechainRoute(int routeId, int sourceTrackIndex, int targetTrackIndex,
                                         uint32_t targetPluginNodeId, int busIndex, float gain)
{
    if (!sidechainManager)
        return false;

    SidechainRoute route;
    route.sourceTrackIndex = sourceTrackIndex;
    route.targetTrackIndex = targetTrackIndex;
    route.targetPluginNodeId = targetPluginNodeId;
    route.busIndex = busIndex;
    route.gain = gain;

    return sidechainManager->updateRoute(routeId, route);
}

bool AudioRoutingGraph::setSidechainRouteEnabled(int routeId, bool enabled)
{
    if (!sidechainManager)
        return false;

    return sidechainManager->setRouteEnabled(routeId, enabled);
}

const std::vector<SidechainRoute>& AudioRoutingGraph::getAllSidechainRoutes() const
{
    static std::vector<SidechainRoute> emptyRoutes;
    if (!sidechainManager)
        return emptyRoutes;

    return sidechainManager->getAllRoutes();
}

//==============================================================================
void AudioRoutingGraph::scanPluginSidechains()
{
    if (!sidechainManager)
        return;

    // Clear existing sidechain info
    sidechainManager->clearPluginSidechainInfo();

    // Scan all plugin wrappers for sidechain input buses
    for (const auto& [nodeId, wrapper] : pluginInstanceWrappers)
    {
        if (!wrapper || wrapper->isBypassed())
            continue;

        auto* audioPlugin = wrapper->getAudioPluginInstance();
        if (!audioPlugin)
            continue;

        SidechainPluginInfo info;
        info.nodeId = nodeId;
        info.pluginName = audioPlugin->getName();
        info.trackIndex = getPluginTrackIndex(nodeId);

        // Check for sidechain input buses
        int numBuses = audioPlugin->getBusCount(true);  // input buses
        info.hasSidechainInput = false;
        info.numSidechainBuses = 0;
        info.sidechainBusChannels.clear();

        // Sidechain buses are typically buses beyond the main audio input
        // For most plugins: bus 0 = main audio, bus 1+ = sidechain
        for (int busIdx = 0; busIdx < numBuses; ++busIdx)
        {
            auto* bus = audioPlugin->getBus(true, busIdx);  // isInput=true

            if (bus)
            {
                // Check if this is a sidechain bus (not the main input)
                juce::String busName = bus->getName().toLowerCase();
                bool isSidechain = busName.contains("sidechain") ||
                                  busName.contains("key input") ||
                                  busName.contains("sc") ||
                                  busIdx > 0;  // Assume secondary buses are sidechain

                if (isSidechain && bus->getNumberOfChannels() > 0)
                {
                    info.hasSidechainInput = true;
                    info.numSidechainBuses++;
                    info.sidechainBusChannels.push_back(bus->getNumberOfChannels());
                }
            }
        }

        // Register plugin info if it has sidechain capability
        if (info.hasSidechainInput)
        {
            sidechainManager->registerPluginSidechainInfo(info);
            DBG("Sidechain-capable plugin found: " + info.pluginName +
                 " (Track: " + juce::String(info.trackIndex) +
                 ", Buses: " + juce::String(info.numSidechainBuses) + ")");
        }
    }

    // Also scan non-owning references
    for (const auto& [nodeId, ref] : pluginReferences)
    {
        if (!ref)
            continue;

        SidechainPluginInfo info;
        info.nodeId = nodeId;
        info.pluginName = ref->getName();
        info.trackIndex = getPluginTrackIndex(nodeId);

        // Check for sidechain input buses
        int numBuses = ref->getBusCount(true);  // input buses
        info.hasSidechainInput = false;
        info.numSidechainBuses = 0;
        info.sidechainBusChannels.clear();

        for (int busIdx = 0; busIdx < numBuses; ++busIdx)
        {
            auto* bus = ref->getBus(true, busIdx);  // isInput=true

            if (bus)
            {
                juce::String busName = bus->getName().toLowerCase();
                bool isSidechain = busName.contains("sidechain") ||
                                  busName.contains("key input") ||
                                  busName.contains("sc") ||
                                  busIdx > 0;

                if (isSidechain && bus->getNumberOfChannels() > 0)
                {
                    info.hasSidechainInput = true;
                    info.numSidechainBuses++;
                    info.sidechainBusChannels.push_back(bus->getNumberOfChannels());
                }
            }
        }

        if (info.hasSidechainInput)
        {
            sidechainManager->registerPluginSidechainInfo(info);
            DBG("Sidechain-capable plugin found: " + info.pluginName +
                 " (Track: " + juce::String(info.trackIndex) +
                 ", Buses: " + juce::String(info.numSidechainBuses) + ")");
        }
    }
}

//==============================================================================
// MIDI Output Support
//==============================================================================

int AudioRoutingGraph::addMIDIRoute(int sourceTrackIndex, int targetTrackIndex, int channelFilter)
{
    if (!midiOutputHandler)
        return -1;

    MIDIRoute route;
    route.sourceTrackIndex = sourceTrackIndex;
    route.targetTrackIndex = targetTrackIndex;
    route.channelFilter = channelFilter;
    route.enabled = true;

    int routeId = midiOutputHandler->addRoute(route);
    if (routeId >= 0)
    {
        DBG("MIDI route added: Track " + juce::String(sourceTrackIndex) + " -> Track " +
            juce::String(targetTrackIndex) + " (ID: " + juce::String(routeId) + ")");
    }

    return routeId;
}

bool AudioRoutingGraph::removeMIDIRoute(int routeId)
{
    if (!midiOutputHandler)
        return false;

    return midiOutputHandler->removeRoute(routeId);
}

bool AudioRoutingGraph::updateMIDIRoute(int routeId, int sourceTrackIndex, int targetTrackIndex, int channelFilter)
{
    if (!midiOutputHandler)
        return false;

    MIDIRoute route;
    route.sourceTrackIndex = sourceTrackIndex;
    route.targetTrackIndex = targetTrackIndex;
    route.channelFilter = channelFilter;

    return midiOutputHandler->setRouteEnabled(routeId, true);
}

bool AudioRoutingGraph::setMIDIRouteEnabled(int routeId, bool enabled)
{
    if (!midiOutputHandler)
        return false;

    return midiOutputHandler->setRouteEnabled(routeId, enabled);
}

const std::vector<MIDIRoute>& AudioRoutingGraph::getAllMIDIRoutes() const
{
    static std::vector<MIDIRoute> emptyRoutes;
    if (!midiOutputHandler)
        return emptyRoutes;

    return midiOutputHandler->getAllRoutes();
}

void AudioRoutingGraph::setMidiThruEnabled(bool enabled)
{
    if (midiOutputHandler)
        midiOutputHandler->setMidiThruEnabled(enabled);
}

bool AudioRoutingGraph::isMidiThruEnabled() const
{
    return midiOutputHandler ? midiOutputHandler->isMidiThruEnabled() : false;
}

void AudioRoutingGraph::setMidiThruChannelFilter(int channel)
{
    if (midiOutputHandler)
        midiOutputHandler->setMidiThruChannelFilter(channel);
}

int AudioRoutingGraph::getMidiThruChannelFilter() const
{
    return midiOutputHandler ? midiOutputHandler->getMidiThruChannelFilter() : -1;
}

//==============================================================================
// Plugin Parameter Automation
//==============================================================================

void AudioRoutingGraph::scanPluginParameters()
{
    if (!pluginParameterAutomation)
        return;

    // Collect all loaded plugins
    std::unordered_map<uint32_t, juce::AudioPluginInstance*> loadedPlugins;

    // Scan all wrappers
    for (const auto& [nodeId, wrapper] : pluginInstanceWrappers)
    {
        if (wrapper && !wrapper->isBypassed())
        {
            auto* plugin = wrapper->getAudioPluginInstance();
            if (plugin)
                loadedPlugins[nodeId] = plugin;
        }
    }

    // Scan all references
    for (const auto& [nodeId, ref] : pluginReferences)
    {
        if (ref)
        {
            loadedPlugins[nodeId] = ref;
        }
    }

    // Scan raw plugin nodes (legacy)
    for (const auto& [nodeId, plugin] : pluginNodes)
    {
        if (plugin)
            loadedPlugins[nodeId] = plugin.get();
    }

    pluginParameterAutomation->scanPlugins(loadedPlugins);
}

int AudioRoutingGraph::addPluginParameterMapping(const PluginParameterMapping& mapping)
{
    if (!pluginParameterAutomation)
        return -1;

    return pluginParameterAutomation->addMapping(mapping);
}

bool AudioRoutingGraph::removePluginParameterMapping(int mappingId)
{
    if (!pluginParameterAutomation)
        return false;

    return pluginParameterAutomation->removeMapping(mappingId);
}

bool AudioRoutingGraph::updatePluginParameterMapping(int mappingId, const PluginParameterMapping& mapping)
{
    if (!pluginParameterAutomation)
        return false;

    return pluginParameterAutomation->updateMapping(mappingId, mapping);
}

bool AudioRoutingGraph::setPluginParameterMappingEnabled(int mappingId, bool enabled)
{
    if (!pluginParameterAutomation)
        return false;

    return pluginParameterAutomation->setMappingEnabled(mappingId, enabled);
}

const std::vector<PluginParameterMapping>& AudioRoutingGraph::getAllPluginParameterMappings() const
{
    static std::vector<PluginParameterMapping> emptyMappings;
    if (!pluginParameterAutomation)
        return emptyMappings;

    return pluginParameterAutomation->getAllMappings();
}

std::vector<PluginParameterMapping*> AudioRoutingGraph::getPluginParameterMappingsForPlugin(uint32_t pluginNodeId)
{
    if (!pluginParameterAutomation)
        return {};

    return pluginParameterAutomation->getMappingsForPlugin(pluginNodeId);
}

void AudioRoutingGraph::setPluginAutomationEnabled(bool enabled)
{
    if (pluginParameterAutomation)
        pluginParameterAutomation->setEnabled(enabled);
}

bool AudioRoutingGraph::isPluginAutomationEnabled() const
{
    return pluginParameterAutomation ? pluginParameterAutomation->isEnabled() : false;
}
