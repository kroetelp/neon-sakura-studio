#pragma once

/**
 * AudioRoutingGraph - Central AudioProcessorGraph for Neon Sakura Studio DAW
 *
 * This is the heart of the DAW's audio routing system. It manages:
 *   - Track processors (each track is a processor)
 *   - Internal synthesizer (InternalSynthProcessor)
 *   - External VST3/AU plugins
 *   - Master output with effects
 *
 * Architecture:
 *
 *   [TrackProcessor 0] ──┐
 *   [TrackProcessor 1] ──┼──► [MasterOutput] ──► Output
 *   ...                  │
 *   [InternalSynth] ─────┘
 *
 * Real-Time Safety:
 *   - All buffers are pre-allocated in prepareToPlay()
 *   - No heap allocations in processBlock()
 *   - Atomic NodeID counter for thread-safe plugin insertion
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <array>
#include <unordered_map>
#include <atomic>
#include <algorithm>
#include <functional>
#include "DelayLine.h"

class TrackProcessor;
class MasterOutputProcessor;
class ITrackDataProvider;
class WavetableEngine;
class TimelinePlayHead;
class PluginInstance;
class SidechainManager;
class MIDIOutputHandler;
class PluginParameterAutomation;

// Forward declarations for SidechainManager types
struct SidechainRoute;
struct MIDIRoute;
struct PluginParameterMapping;

class InternalSynthProcessor
{
public:
    InternalSynthProcessor() = default;
    virtual ~InternalSynthProcessor() = default;

    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void releaseResources() = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) = 0;

    virtual void noteOn(int channel, int midiNote, float velocity) = 0;
    virtual void noteOff(int channel, int midiNote) = 0;
    virtual void setBPM(float bpm) = 0;
    virtual void setMasterVolume(float volume) = 0;

    virtual WavetableEngine* getWavetableEngine() = 0;
};

// Forward declare NodeID for TrackPluginChain
using NodeID = uint32_t;

//==============================================================================
// Forward declaration for shared_ptr
class PluginInstance;

/**
 * LoadedPlugin - Tracks a plugin that has been loaded into the graph
 *
 * This structure keeps track of where a plugin is loaded (track, position),
 * its NodeID for graph operations, and provides access to the underlying
 * AudioPluginInstance for UI purposes.
 *
 * IMPORTANT: The pluginInstanceWrapper keeps the PluginInstance alive.
 * Do NOT store raw pointers to AudioPluginInstance outside this struct.
 */
struct LoadedPlugin
{
    NodeID nodeId = 0;
    int trackIndex = 0;
    int positionInChain = 0;
    std::shared_ptr<PluginInstance> pluginInstanceWrapper;  // Owning - keeps plugin alive
    juce::AudioPluginInstance* pluginInstance = nullptr;    // Non-owning convenience pointer

    /** Get the plugin name. Implementation in .cpp file due to incomplete type. */
    juce::String getName() const;

    /** Check if this plugin is still valid. Implementation in .cpp file. */
    bool isValid() const;
};

/**
 * TrackPluginChain - Verwaltet die Plugin-Kette für einen einzelnen Track
 *
 * Jeder Track hat seine eigene Plugin-Kette, die in der richtigen Reihenfolge
 * verarbeitet wird. Dies ermöglicht es, verschiedene Plugins auf verschiedenen
 * Tracks zu laden (z.B. Serum2 auf Track 1, Spitfish auf Track 2).
 */
struct TrackPluginChain
{
    std::vector<NodeID> pluginNodeIds;  // Geordnete Liste von Plugin-NodeIDs

    void clear() { pluginNodeIds.clear(); }
    bool empty() const { return pluginNodeIds.empty(); }
    size_t size() const { return pluginNodeIds.size(); }

    // Plugin an Position einfügen (-1 = am Ende)
    void insertPlugin(NodeID nodeId, int position = -1)
    {
        if (position < 0 || position >= static_cast<int>(pluginNodeIds.size()))
            pluginNodeIds.push_back(nodeId);
        else
            pluginNodeIds.insert(pluginNodeIds.begin() + position, nodeId);
    }

    // Plugin entfernen
    bool removePlugin(NodeID nodeId)
    {
        auto it = std::find(pluginNodeIds.begin(), pluginNodeIds.end(), nodeId);
        if (it != pluginNodeIds.end())
        {
            pluginNodeIds.erase(it);
            return true;
        }
        return false;
    }
};

class AudioRoutingGraph
{
public:
    static constexpr int maxTracks = 16;

    AudioRoutingGraph();
    ~AudioRoutingGraph();

    // ============================================================
    // Initialization
    // ============================================================

    /** Initialize the graph with track provider. */
    void initialize(ITrackDataProvider* provider, WavetableEngine* wavetable);

    /** Check if initialized. */
    bool isInitialized() const { return initialized; }

    // ============================================================
    // Audio Processing
    // ============================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // ============================================================
    // Track Access
    // ============================================================

    TrackProcessor* getTrackProcessor(int trackIndex);
    const TrackProcessor* getTrackProcessor(int trackIndex) const;

    // ============================================================
    // Internal Synth Access
    // ============================================================

    InternalSynthProcessor* getInternalSynthProcessor() { return internalSynthProcessor.get(); }
    const InternalSynthProcessor* getInternalSynthProcessor() const { return internalSynthProcessor.get(); }

    // ============================================================
    // Master Output Access
    // ============================================================

    MasterOutputProcessor* getMasterOutputProcessor() { return masterOutputProcessor.get(); }
    const MasterOutputProcessor* getMasterOutputProcessor() const { return masterOutputProcessor.get(); }

    // ============================================================
    // Connection Management
    // ============================================================

    bool connectTrackToMaster(int trackIndex);
    bool disconnectTrackFromMaster(int trackIndex);
    bool connectInternalSynthToMaster();
    bool disconnectInternalSynthFromMaster();

    // ============================================================
    // Plugin Management
    // ============================================================

    // NodeID is now defined at namespace level

    /** Insert a plugin into a specific track's plugin chain at the given position.
        position = -1 appends to the end of the chain. */
    NodeID insertPluginInTrack(int trackIndex, int position,
                               std::unique_ptr<juce::AudioPluginInstance> plugin);

    /** Insert a PluginInstance wrapper into a track's plugin chain.
        This method extracts the AudioPluginInstance and stores the wrapper for later access.
        The Graph takes ownership of the PluginInstance.
        Returns the NodeID for this plugin, or 0 on failure. */
    NodeID insertPluginInstanceInTrack(int trackIndex, int position,
                                       std::shared_ptr<PluginInstance> pluginInstance);

    /** Insert a plugin reference into a track's plugin chain (non-owning).
        The caller retains ownership of the AudioPluginInstance and must ensure it remains valid
        for the lifetime of this graph or until the plugin is removed.
        Returns the NodeID for this plugin, or 0 on failure. */
    NodeID insertPluginReference(int trackIndex, int position,
                                 juce::AudioPluginInstance* pluginInstance);

    /** Legacy method - inserts plugin into track 0 for backwards compatibility. */
    NodeID insertPluginAfterTrack(int trackIndex,
                                   std::unique_ptr<juce::AudioPluginInstance> plugin);

    /** Remove a plugin from any track. Returns true if found and removed. */
    bool removePluginNode(NodeID nodeId);

    /** Remove a plugin from a specific track. Returns true if found and removed. */
    bool removePluginFromTrack(int trackIndex, NodeID nodeId);

    /** Get the plugin chain for a specific track. */
    TrackPluginChain* getTrackPluginChain(int trackIndex);
    const TrackPluginChain* getTrackPluginChain(int trackIndex) const;

    /** Get the track index that owns a specific plugin. Returns -1 if not found. */
    int getPluginTrackIndex(NodeID nodeId) const;

    /** Get a plugin by its NodeID. */
    juce::AudioPluginInstance* getPlugin(NodeID nodeId);
    const juce::AudioPluginInstance* getPlugin(NodeID nodeId) const;

    /** Get a PluginInstance wrapper by its NodeID. Returns nullptr if not found. */
    std::shared_ptr<PluginInstance> getPluginInstance(NodeID nodeId);
    std::shared_ptr<const PluginInstance> getPluginInstance(NodeID nodeId) const;

    /** Get all PluginInstances for a specific track. */
    std::vector<std::shared_ptr<PluginInstance>> getPluginInstancesForTrack(int trackIndex) const;

    // ============================================================
    // State Management
    // ============================================================

    std::unique_ptr<juce::XmlElement> getState() const;

    /** Callback type for plugin loading during state restoration.
        Returns nullptr if the plugin could not be loaded.
        The callback receives:
        - fileOrIdentifier: Plugin path or identifier
        - formatName: Plugin format name (e.g., "VST3")
        Returns: AudioPluginInstance (unique_ptr ownership) */
    using PluginLoadCallback = std::function<std::unique_ptr<juce::AudioPluginInstance>(
        const juce::String& fileOrIdentifier,
        const juce::String& formatName)>;

    /** Restore state from XML. The pluginLoadCallback is used to recreate plugins.
        @param xml The XML element containing the saved state
        @param pluginLoadCallback Callback to load plugins by description
    */
    void setState(const juce::XmlElement& xml, PluginLoadCallback pluginLoadCallback = nullptr);

    /** Legacy method for backwards compatibility (does not restore plugins). */
    void setState(const juce::XmlElement& xml);

    // ============================================================
    // PlayHead Support (for plugin tempo/position sync)
    // ============================================================

    /** Set the playhead for all plugins to sync with. */
    void setPlayHead(juce::AudioPlayHead* playHead);

    /** Get the current playhead. */
    juce::AudioPlayHead* getPlayHead() { return currentPlayHead; }
    const juce::AudioPlayHead* getPlayHead() const { return currentPlayHead; }

    // ============================================================
    // Latency Compensation
    // ============================================================

    /** Calculate and update latency compensation for all tracks.
        This should be called after adding/removing plugins.
        Tracks with less latency will be delayed to match the track with most latency. */
    void updateLatencyCompensation();

    /** Get the current maximum latency across all tracks (in samples). */
    int getMaxLatency() const { return maxLatency.load(); }

    /** Get the latency for a specific track (in samples). */
    int getTrackLatency(int trackIndex) const;

    // ============================================================
    // Sidechain Routing Support
    // ============================================================

    /** Get the sidechain manager for this graph. */
    SidechainManager* getSidechainManager() { return sidechainManager.get(); }
    const SidechainManager* getSidechainManager() const { return sidechainManager.get(); }

    /** Scan all loaded plugins for sidechain input buses.
        This should be called after plugin insert/remove/load. */
    void scanPluginSidechains();

    /** Add a sidechain route from source track to target track.
        @return Route ID or -1 on failure */
    int addSidechainRoute(int sourceTrackIndex, int targetTrackIndex,
                         uint32_t targetPluginNodeId = 0, int busIndex = 0, float gain = 1.0f);

    /** Remove a sidechain route by ID. */
    bool removeSidechainRoute(int routeId);

    /** Update a sidechain route. */
    bool updateSidechainRoute(int routeId, int sourceTrackIndex, int targetTrackIndex,
                             uint32_t targetPluginNodeId = 0, int busIndex = 0, float gain = 1.0f);

    /** Enable/disable a sidechain route. */
    bool setSidechainRouteEnabled(int routeId, bool enabled);

    /** Get all sidechain routes (for UI visualization). */
    const std::vector<SidechainRoute>& getAllSidechainRoutes() const;

    // ============================================================
    // MIDI Output Support
    // ============================================================

    /** Get the MIDI output handler for this graph. */
    MIDIOutputHandler* getMIDIOutputHandler() { return midiOutputHandler.get(); }
    const MIDIOutputHandler* getMIDIOutputHandler() const { return midiOutputHandler.get(); }

    /** Add a MIDI route from source track to target track.
        @return Route ID or -1 on failure */
    int addMIDIRoute(int sourceTrackIndex, int targetTrackIndex, int channelFilter = -1);

    /** Remove a MIDI route by ID. */
    bool removeMIDIRoute(int routeId);

    /** Update a MIDI route. */
    bool updateMIDIRoute(int routeId, int sourceTrackIndex, int targetTrackIndex, int channelFilter = -1);

    /** Enable/disable a MIDI route. */
    bool setMIDIRouteEnabled(int routeId, bool enabled);

    /** Get all MIDI routes (for UI visualization). */
    const std::vector<MIDIRoute>& getAllMIDIRoutes() const;

    /** Set MIDI-Thru enabled state. */
    void setMidiThruEnabled(bool enabled);

    /** Get MIDI-Thru enabled state. */
    bool isMidiThruEnabled() const;

    /** Set MIDI-Thru channel filter (-1 = all channels). */
    void setMidiThruChannelFilter(int channel);

    /** Get MIDI-Thru channel filter. */
    int getMidiThruChannelFilter() const;

    // ============================================================
    // Plugin Parameter Automation
    // ============================================================

    /** Get the plugin parameter automation handler. */
    PluginParameterAutomation* getPluginParameterAutomation() { return pluginParameterAutomation.get(); }
    const PluginParameterAutomation* getPluginParameterAutomation() const { return pluginParameterAutomation.get(); }

    /** Scan all loaded plugins for automatable parameters. */
    void scanPluginParameters();

    /** Add a new plugin parameter mapping.
        @return Mapping ID or -1 on failure */
    int addPluginParameterMapping(const PluginParameterMapping& mapping);

    /** Remove a plugin parameter mapping by ID. */
    bool removePluginParameterMapping(int mappingId);

    /** Update a plugin parameter mapping. */
    bool updatePluginParameterMapping(int mappingId, const PluginParameterMapping& mapping);

    /** Enable/disable a plugin parameter mapping. */
    bool setPluginParameterMappingEnabled(int mappingId, bool enabled);

    /** Get all plugin parameter mappings (for UI visualization). */
    const std::vector<PluginParameterMapping>& getAllPluginParameterMappings() const;

    /** Get mappings for a specific plugin. */
    std::vector<PluginParameterMapping*> getPluginParameterMappingsForPlugin(uint32_t pluginNodeId);

    /** Enable/disable all plugin parameter automation. */
    void setPluginAutomationEnabled(bool enabled);

    /** Check if plugin parameter automation is enabled. */
    bool isPluginAutomationEnabled() const;

    // ============================================================
    // UI Integration: MasterBusPanel Callbacks
    // ============================================================

    // Callback-Typ für Level-Updates (L/R-Kanäle)
    using LevelUpdateCallback = std::function<void(float, float)>;

    // Callback-Typ für Loudness-Updates (LUFS, Peaks)
    using LoudnessUpdateCallback = std::function<void(float)>;

    void setLevelUpdateCallback(LevelUpdateCallback callback);
    void setLoudnessUpdateCallback(LoudnessUpdateCallback callback);

    // Master Control Access
    void setMasterVolume(float volume);
    void setMasterPan(float pan);
    void setMasterMute(bool muted);

    // Level-Update für UI
    void notifyLevelUpdate(float leftLevel, float rightLevel);

    // Loudness-Update für UI
    void notifyLoudnessUpdate(float loudness);

private:
    bool initialized = false;

    std::unique_ptr<InternalSynthProcessor> internalSynthProcessor;
    std::unique_ptr<MasterOutputProcessor> masterOutputProcessor;
    std::array<std::unique_ptr<TrackProcessor>, maxTracks> trackProcessors;

    std::unordered_map<NodeID, std::unique_ptr<juce::AudioPluginInstance>> pluginNodes;
    std::array<NodeID, maxTracks> trackNodeIDs{};

    // Per-Track Plugin Chains - Jeder Track hat seine eigene Plugin-Kette
    std::array<TrackPluginChain, maxTracks> trackPluginChains;

    ITrackDataProvider* trackProvider = nullptr;
    WavetableEngine* wavetableEngine = nullptr;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    // ============================================================
    // Pre-allocated buffers for Real-Time Safety
    // ============================================================
    static constexpr int maxBufferSize = 4096;
    std::unique_ptr<juce::AudioBuffer<float>> tempBuffer;      // Pre-allocated temp buffer for mixing
    juce::MidiBuffer tempMidiBuffer;                           // Pre-allocated MIDI buffer

    // Thread-safe NodeID counter
    std::atomic<uint32_t> nextNodeId{1000};

    // PlayHead for plugin tempo/position sync (not owned by this class)
    juce::AudioPlayHead* currentPlayHead = nullptr;

    // PluginInstance wrappers (for UI access, state management, window management)
    // Maps NodeID to the PluginInstance wrapper that owns the plugin
    std::unordered_map<NodeID, std::shared_ptr<PluginInstance>> pluginInstanceWrappers;

    // Non-owning plugin references (for plugins owned externally)
    std::unordered_map<NodeID, juce::AudioPluginInstance*> pluginReferences;

    // ============================================================
    // Latency Compensation
    // ============================================================
    std::array<DelayLine, maxTracks> delayLines;       // Delay lines for each track
    std::array<int, maxTracks> trackLatencies{};       // Cached latency per track
    std::atomic<int> maxLatency{0};                    // Maximum latency across all tracks

    // ============================================================
    // Sidechain Routing
    // ============================================================
    std::unique_ptr<SidechainManager> sidechainManager;  // Manages all sidechain routings

    // ============================================================
    // MIDI Output Routing
    // ============================================================
    std::unique_ptr<MIDIOutputHandler> midiOutputHandler;  // Manages all MIDI output routings

    // ============================================================
    // UI Callbacks (thread-safe)
    // ============================================================
    LevelUpdateCallback levelUpdateCallback;       // Callback for L/R level updates
    LoudnessUpdateCallback loudnessUpdateCallback;  // Callback for LUFS updates

    // ============================================================
    // Plugin Parameter Automation
    // ============================================================
    std::unique_ptr<PluginParameterAutomation> pluginParameterAutomation;  // Manages plugin parameter automation

    /** Calculate latency for a single track (sum of all plugin latencies in chain). */
    int calculateTrackLatency(int trackIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioRoutingGraph)
};
