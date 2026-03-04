# Roadmap: Timeline als Herzstück & Universelle Plugin-Integration

## Übersicht

Diese Roadmap transformiert Neon Sakura Studio von einem dual-mode System (StepSequencer/Timeline) zu einer unified Timeline-Architektur, bei der alle Elemente (StepSequencer, Internal Synths, VST Plugins) als Timeline-Tracks fungieren.

---

## Phase 1: Foundation - Unified Audio Architecture

### 1.1 IAudioSource Interface
**Ziel:** Einheitliches Interface für alle Audio-Quellen

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/IAudioSource.h` (neu) ✅
- `source/AudioRouting/TrackProcessor.h` (anpassen) ✅

**Implementierte Features:**
- [x] `IAudioSource` Interface definiert
- [x] `getLatencySamples()` für Delay-Kompensation
- [x] `getTailSeconds()` für Reverb-Tails
- [x] `supportsDoublePrecision()` für High-Quality Plugins
- [x] `isBypassed()` / `setBypassed()` für Bypass-Support
- [x] `saveState()` / `loadState()` für Persistenz

**Implementierung:**
```cpp
class IAudioSource {
public:
    virtual ~IAudioSource() = default;
    virtual void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) = 0;
    virtual void prepareToPlay(double sampleRate, int maxBlockSize) = 0;
    virtual void releaseResources() = 0;
    virtual int getLatencySamples() const { return 0; }
    virtual double getTailSeconds() const { return 0.0; }
    virtual bool supportsDoublePrecision() const { return false; }
    virtual void processBlockDouble(AudioBuffer<double>& buffer, MidiBuffer& midi);
    virtual juce::String getName() const = 0;
    virtual bool isBypassed() const { return false; }
    virtual void setBypassed(bool bypass) { }
};
```

### 1.2 TrackProcessor Refactoring
**Ziel:** TrackProcessor als IAudioSource-Implementierung

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/TrackProcessor.h` ✅
- `source/AudioRouting/TrackProcessor.cpp` ✅

**Implementierte Features:**
- [x] TrackProcessor implementiert IAudioSource (Mehrfachvererbung)
- [x] Latency-Propagation durch Plugin-Chain (`pluginChainLatency`)
- [x] Sidechain-Input Support vorbereitet (Interface erweiterbar)
- [x] Bypass-Support implementiert

### 1.3 AudioPlayHead Implementation
**Ziel:** Plugins erhalten Tempo/Position-Information

**Dateien:**
- `source/Timeline/TimelinePlayHead.h` (neu)
- `source/Timeline/TimelinePlayHead.cpp` (neu)

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Implementierte Features:**
- [x] `juce::AudioPlayHead` ableiten
- [x] `getPosition()` mit TimelineData verknüpfen
- [x] TimeSignature, Loop-Points, Bar/Beat-Position
- [x] Integration in AudioEngine
- [x] Verbindung mit AudioRoutingGraph (alle Plugins erhalten PlayHead)
- [x] Sample-Rate für accurate time-in-samples Berechnung

**Implementierung:**
```cpp
class TimelinePlayHead : public juce::AudioPlayHead {
public:
    TimelinePlayHead(TimelineData& data, TimelineTransport& transport);

    juce::Optional<PositionInfo> getPosition() const override;
    bool canControlTransport() override { return false; }

    void setSampleRate(double sampleRate);

private:
    TimelineData& timelineData;
    TimelineTransport& timelineTransport;
    double currentSampleRate = 44100.0;
};
```

---

## Phase 2: Timeline als Primary Mode

### 2.1 Engine Mode Consolidation
**Ziel:** Entfernung der dual-mode Architektur

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioEngine.h` ✅
- `source/AudioEngine.cpp` ✅

**Implementierte Features:**
- [x] `EngineMode` Enum entfernt
- [x] Timeline immer aktiv (unified architecture)
- [x] StepSequencer-Logik in `generateStepSequencerMidi()` extrahiert
- [x] BPM-Sync zwischen AudioEngine und TimelineData
- [x] Unified `getNextAudioBlock()` mit 6-stufiger Pipeline:
  1. StepSequencer MIDI Generation
  2. Timeline Clip Rendering
  3. Track Mixing
  4. VST Plugin Graph
  5. Master Effects
  6. Playhead Update

**Implementierung:**
```cpp
void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // === Unified Timeline Architecture ===
    // Timeline is always active and handles:
    // - Audio/MIDI clips on tracks
    // - VST plugins via AudioRoutingGraph
    // - Step sequencer patterns as track data

    // Step 1: Generate MIDI from Step Sequencer Patterns
    if (localIsPlaying)
        generateStepSequencerMidi(bufferToFill.numSamples);

    // Step 2: Render Timeline Clips
    if (localIsPlaying && timelineTransport->isPlaying())
        timelineRenderer->processBlock(*bufferToFill.buffer, wavetableMidiBuffer);

    // Step 3-6: Mix, VST, Effects, Playhead
    ...
}
```

### 2.2 StepSequencer als Timeline Clip
**Ziel:** StepSequencer-Patterns werden zu Timeline-Clips

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/StepSequencer/StepSequencerClip.h` (neu) ✅
- `source/StepSequencer/StepSequencerClip.cpp` (neu) ✅
- `source/Timeline/TimelineClip.h` (angepasst - virtueller Destruktor) ✅
- `source/Timeline/TimelineRenderer.h` (erweitert) ✅
- `source/Timeline/TimelineRenderer.cpp` (StepSequencer Rendering) ✅

**Implementierte Features:**
- [x] Neue Klasse `StepSequencerClip : public TimelineClip`
- [x] Pattern-Daten im Clip speichern (8 Tracks x 64 Steps)
- [x] Clip kann auf Timeline positioniert/geloopt werden
- [x] Mehrere StepSequencer-Clips pro Track möglich
- [x] StepModifier-Unterstützung (TidalCycles-style)
- [x] P-Locks (Elektron-style) pro Step
- [x] Per-Track Base-Note und MIDI-Channel
- [x] Runtime MIDI-Generierung (renderToMidiBuffer)
- [x] TimelineClip::Type um StepSequencer erweitert
- [x] TimelineRenderer unterstützt StepSequencerClip-Rendering

**Code-Snippet:**
```cpp
class StepSequencerClip : public TimelineClip {
public:
    // Pattern data
    int numSteps = 16;
    int loopLength = 16;
    std::array<std::array<bool, 16>, 8> stepData;  // [track][step]
    std::array<float, 16> velocities;

    // Modifiers
    std::array<std::array<StepModifier, 16>, 8> modifiers;
    std::array<std::array<std::map<int, float>, 16>, 8> pLocks;  // Parameter Locks

    // Playback state (per instance)
    mutable int currentStep = 0;
    mutable double stepAccumulator = 0.0;

    void render(AudioBuffer<float>& buffer, MidiBuffer& midi,
                double playheadBeat, double bpm, double sampleRate) override;
};
```

### 2.3 Wavetable Synth als Timeline Track
**Ziel:** WavetableEngine vollständig in TrackProcessor integriert

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/TrackProcessor.cpp` ✅
- `source/AudioRouting/TrackProcessor.h` ✅
- `source/Timeline/AutomationClip.h` (neu) ✅
- `source/Timeline/AutomationClip.cpp` (neu) ✅
- `source/Timeline/AutomationLane.h` (neu) ✅
- `source/Timeline/AutomationLane.cpp` (neu) ✅
- `source/Timeline/TimelineTrack.h` (erweitert) ✅
- `source/TrackManager.h` (erweitert) ✅
- `source/TrackManager.cpp` (erweitert) ✅
- `source/AudioEngine.cpp` (erweitert) ✅
- `source/ITrackDataProvider.h` (erweitert) ✅

**Implementierte Features:**
- [x] WavetableEngine als TrackProcessor-Modus
- [x] Automation von Wavetable-Parametern über Timeline
  - AutomationClip: Speichert Automation-Points mit Beat-Position
  - AutomationLane: Verwaltet Automation-Clips pro Parameter
  - AutomationManager: Koordiniert alle Automations für einen Track
  - Unterstützte Interpolation: Step, Linear, Bezier
- [x] Multi-Timbral Support (verschiedene WT pro Track) - vorbereitet
- [x] Playhead-Sync zwischen AudioEngine und TrackManager
- [x] Parameter-Mapping für Wavetable-Params (oscMorph, filterCutoff, etc.)

**Implementierung:**
```cpp
// AutomationClip - Ein Clip mit Automation-Points
class AutomationClip {
public:
    void addPoint(double positionBeat, float value, float curve = 0.0f);
    float getValueAtBeat(double beatPosition) const;

    enum Interpolation { Step, Linear, Bezier };

private:
    std::vector<AutomationPoint> points;
};

// AutomationManager - Koordiniert alle Automations
class AutomationManager {
public:
    AutomationLane* createLane(const String& parameterId);
    std::unordered_map<String, float> processAutomation(double currentBeat);

private:
    std::unordered_map<String, std::unique_ptr<AutomationLane>> lanes;
};

// TrackProcessor - Wendet Automation an
void TrackProcessor::processBlock(...) {
    if (timelineTrack) {
        auto automationValues = timelineTrack->processAutomation(currentBeat);
        if (!automationValues.empty()) {
            applyWavetableAutomation(automationValues);
        }
    }
}
```

---

## Phase 3: Plugin Integration Completion

### 3.1 Plugin-to-Track Routing
**Ziel:** VST Plugins werden in Track-Chains eingefügt

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/MainComponent.cpp` (loadPluginToSelectedTrack) ✅
- `source/AudioRouting/AudioRoutingGraph.h` (LoadedPlugin struct erweitert) ✅
- `source/AudioRouting/AudioRoutingGraph.cpp` (insertPluginInstanceInTrack, Latency Compensation) ✅

**Implementierte Features:**
- [x] `AudioRoutingGraph::insertPluginInstanceInTrack()` mit shared_ptr Ownership
- [x] Plugin wird nach TrackProcessor in Chain eingefügt
- [x] `LoadedPlugin` struct erweitert um `shared_ptr<PluginInstance>` für korrektes Ownership
- [x] GUI-Callback für erfolgreiche Insertion (PluginWindow öffnet sich)
- [x] Latency Compensation implementiert:
  - `calculateTrackLatency()` - Summe aller Plugin-Latenzen pro Track
  - `updateLatencyCompensation()` - Delay Lines für Track-Synchronisation
  - Delay Lines in `processBlock()` angewendet

**Implementierung:**
```cpp
void MainComponent::loadPluginToSelectedTrack(std::unique_ptr<PluginInstance> instance)
{
    // Convert to shared_ptr for shared ownership
    auto sharedInstance = std::shared_ptr<PluginInstance>(instance.release());

    // Insert into track's plugin chain
    NodeID nodeId = graph->insertPluginInstanceInTrack(targetTrack, -1, sharedInstance);

    // Store in loadedPlugins for window management
    LoadedPlugin loadedPlugin;
    loadedPlugin.nodeId = nodeId;
    loadedPlugin.pluginInstanceWrapper = sharedInstance;
    loadedPlugins.push_back(loadedPlugin);

    // Open plugin window
    pluginWindowManager->openWindowForPlugin(*sharedInstance);

    // Update latency compensation
    graph->updateLatencyCompensation();
}
```

### 3.2 Plugin State Persistence
**Ziel:** Plugin-Zustände werden mit Projekt gespeichert

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/AudioRoutingGraph.cpp` ✅ (erweitert - pluginInstanceWrappers Serialisierung)
- `source/AudioRouting/AudioRoutingGraph.h` ✅ (erweitert - PluginLoadCallback Dokumentation)
- `source/VSTHost/VSTPluginManager.h` ✅ (neu - loadPluginForState Methode)
- `source/VSTHost/VSTPluginManager.cpp` ✅ (erweitert - loadPluginForState Implementierung)
- `source/Theme/WorkspacePreset.h` ✅ (erweitert - audioRoutingState Feld)
- `source/Theme/WorkspacePreset.cpp` ✅ (erweitert - saveState/restoreState)
- `source/Theme/WorkspaceManager.h` ✅ (erweitert - setAudioRoutingGraph, AudioRoutingGraph Referenz)
- `source/Theme/WorkspaceManager.cpp` ✅ (erweitert - captureCurrentState/applyPreset mit Plugin State)
- `source/MainComponent.cpp` ✅ (erweitert - AudioRoutingGraph Referenz übergeben)

**Implementierte Features:**
- [x] AudioRoutingGraph.getState() serialisiert pluginInstanceWrappers mit Bypass State
- [x] AudioRoutingGraph.setState() lädt PluginInstanceWrapper und stellt State wieder her
- [x] VSTPluginManager.loadPluginForState() für Plugin-Laden beim Projekt-Load
- [x] WorkspacePreset speichert AudioRoutingGraph State als Base64-kodiertes XML
- [x] WorkspaceManager captureCurrentState() erfasst AudioRoutingGraph State
- [x] WorkspaceManager applyPreset() stellt AudioRoutingGraph State mit PluginLoadCallback wieder her
- [x] AudioRoutingGraph Referenz wird an WorkspaceManager übergeben
- [x] Plugin Description wird für korrektes Reload gespeichert

**Code-Snippet:**
```cpp
// Project Save
void ProjectSerializer::savePluginChain(XmlElement& parent, const TrackPluginChain& chain)
{
    for (auto& pluginId : chain.pluginNodeIds)
    {
        auto* pluginXml = parent.createNewChildElement("PLUGIN");
        pluginXml->setAttribute("pluginPath", plugin->getPluginDescription().fileOrIdentifier);
        pluginXml->setAttribute("name", plugin->getName());

        // Plugin state
        MemoryBlock state;
        plugin->getStateInformation(state);
        pluginXml->setAttribute("state", state.toBase64Encoding());
    }
}

// Project Load
void ProjectSerializer::loadPluginChain(const XmlElement& parent, TrackPluginChain& chain)
{
    for (auto* pluginXml : parent.getChildIterator())
    {
        String path = pluginXml->getStringAttribute("pluginPath");
        String stateBase64 = pluginXml->getStringAttribute("state");

        // Async load with state restoration callback
        pluginManager->loadPlugin(path, [stateBase64, &chain](auto instance) {
            MemoryBlock state;
            state.fromBase64Encoding(stateBase64);
            instance->setStateInformation(state.getData(), state.getSize());
            // Insert into chain
        });
    }
}
```

### 3.3 Latency Compensation
**Ziel:** Automatische Delay-Kompensation durch Plugin-Chain

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/AudioRoutingGraph.cpp` ✅
- `source/AudioRouting/DelayLine.h` ✅
- `source/AudioRouting/AudioRoutingGraph.h` ✅

**Implementierte Features:**
- [x] `DelayLine` Klasse für Sample-Verzögerung (DelayLine.h)
- [x] Latency pro Track berechnen (Summe aller Plugin-Latenzen) - `calculateTrackLatency()`
- [x] Delay-Line auf Tracks ohne Plugin anwenden - `updateLatencyCompensation()`
- [x] Latency wird nach jedem Plugin hinzufügen/entfernen aktualisiert
- [x] Threadsichere Delay-Werte mit atomic<int>

**Code-Snippet:**
```cpp
class DelayLine {
public:
    void prepare(double sr, int maxDelaySamples) {
        buffer.setSize(2, maxDelaySamples);
        buffer.clear();
        writePos = 0;
    }

    void process(AudioBuffer<float>& audio, int delaySamples) {
        if (delaySamples == 0) return;

        for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        {
            auto* channelData = audio.getWritePointer(ch);
            auto* delayData = buffer.getWritePointer(ch);

            for (int i = 0; i < audio.getNumSamples(); ++i)
            {
                // Read from delay line
                float delayed = delayData[writePos];

                // Write to delay line
                delayData[writePos] = channelData[i];

                // Output delayed sample
                channelData[i] = delayed;

                writePos = (writePos + 1) % buffer.getNumSamples();
            }
        }
    }

private:
    AudioBuffer<float> buffer;
    int writePos = 0;
};

// In AudioRoutingGraph
void AudioRoutingGraph::updateLatencyCompensation() {
    maxLatency = 0;

    for (int i = 0; i < numTracks; ++i) {
        int trackLatency = calculateTrackLatency(i);
        maxLatency = std::max(maxLatency, trackLatency);
    }

    // Apply delay to tracks with less latency
    for (int i = 0; i < numTracks; ++i) {
        int trackLatency = calculateTrackLatency(i);
        int delayNeeded = maxLatency - trackLatency;
        delayLines[i].setDelay(delayNeeded);
    }
}
```

---

## Phase 4: Advanced Plugin Features

### 4.1 Sidechain Routing
**Ziel:** Audio von einem Track zu Plugin-Input eines anderen

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/SidechainManager.h` (neu) ✅
- `source/AudioRouting/SidechainManager.cpp` (neu) ✅
- `source/AudioRouting/TrackProcessor.h` (erweitert) ✅
- `source/AudioRouting/TrackProcessor.cpp` (erweitert) ✅
- `source/AudioRouting/AudioRoutingGraph.h` (erweitert) ✅
- `source/AudioRouting/AudioRoutingGraph.cpp` (erweitert) ✅
- `source/UI/SidechainRoutingPanel.h` (neu) ✅
- `source/UI/SidechainRoutingPanel.cpp` (neu) ✅

**Implementierte Features:**
- [x] SidechainBuffer-Klasse für Sidechain-Signal pro Track
- [x] SidechainManager-Klasse für zentrale Verwaltung aller Routings
- [x] TrackProcessor mit SidechainBuffer-Input erweitert
- [x] AudioRoutingGraph mit Sidechain-Routing-Methoden erweitert
  - `addSidechainRoute()` - Neue Route erstellen
  - `removeSidechainRoute()` - Route entfernen
  - `updateSidechainRoute()` - Route aktualisieren
  - `setSidechainRouteEnabled()` - Route aktivieren/deaktivieren
  - `getAllSidechainRoutes()` - Alle Routings abrufen
- [x] Integration in processBlock() (Sidechain-Buffer clearen, Track-Output hinzufügen)
- [x] CMakeLists.txt aktualisiert
- [x] UI für Sidechain-Routing (SidechainRoutingPanel)
- [x] Sidechain-enabled Plugins erkennen (scanPluginSidechains())

**Implementierung:**
```cpp
// SidechainRoute - Definiert eine Sidechain-Verbindung
struct SidechainRoute
{
    int sourceTrackIndex = -1;       // Welcher Track liefert das Signal
    int targetTrackIndex = -1;       // Welcher Track empfängt das Signal
    uint32_t targetPluginNodeId = 0;  // Welches Plugin empfängt das Signal (0 = alle)
    int busIndex = 0;                // Sidechain-Bus-Index
    float gain = 1.0f;              // Verstärkung
    bool enabled = true;               // Route aktiv/inaktiv
};

// SidechainManager - Verwaltet alle Sidechain-Routings
class SidechainManager
{
public:
    int addRoute(const SidechainRoute& route);
    bool removeRoute(int routeId);
    void clearAllBuffers();  // Am Block-Anfang aufrufen
    void addSourceToBuffer(int sourceTrackIndex, const AudioBuffer<float>& audio, int numSamples);
    SidechainBuffer* getSidechainBuffer(int trackIndex);
};
```

**Verwendung in AudioRoutingGraph::processBlock():**
```cpp
// Am Block-Anfang: Alle Sidechain-Buffer clearen
sidechainManager->clearAllBuffers();

// Nach Track-Verarbeitung: Als Sidechain-Source hinzufügen
for (int i = 0; i < maxTracks; ++i)
{
    trackProcessors[i]->processBlock(*tempBuffer, tempMidiBuffer);
    sidechainManager->addSourceToBuffer(i, *tempBuffer, buffer.getNumSamples());
    // ... Plugin-Verarbeitung ...
}
```

### 4.2 MIDI Output from Plugins
**Ziel:** Plugins können MIDI generieren (Arpeggiators, etc.)

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/MIDIOutputHandler.h` (neu) ✅
- `source/AudioRouting/MIDIOutputHandler.cpp` (neu) ✅
- `source/AudioRouting/AudioRoutingGraph.h` (erweitert) ✅
- `source/AudioRouting/AudioRoutingGraph.cpp` (erweitert) ✅

**Implementierte Features:**
- [x] MIDIOutputHandler-Klasse für zentrale MIDI-Output-Verwaltung
  - MIDIMessageQueue für Thread-sichere MIDI-Message-Verwaltung
  - MIDIRoute für Routing-Konfiguration
  - PluginMIDIOutputInfo für Plugin-MIDI-Fähigkeiten
- [x] AudioRoutingGraph mit MIDI Output erweitert
  - `addMIDIRoute()` - Neue MIDI-Route erstellen
  - `removeMIDIRoute()` - Route entfernen
  - `updateMIDIRoute()` - Route aktualisieren
  - `setMIDIRouteEnabled()` - Route aktivieren/deaktivieren
  - `getAllMIDIRoutes()` - Alle Routings abrufen
  - `setMidiThruEnabled()` - MIDI-Thru aktivieren
  - `setMidiThruChannelFilter()` - MIDI-Thru Kanal-Filter
- [x] MIDI-Output von Plugins abfangen und in Queue speichern
- [x] MIDI-Routing zu anderen Tracks implementiert
- [x] MIDI-Thru (MIDI-Durchleitung) mit Kanal-Filter implementiert
- [x] CMakeLists.txt aktualisiert

**Implementierung:**
```cpp
// MIDIMessageQueue - Thread-sichere Queue für MIDI-Messages
class MIDIMessageQueue
{
public:
    static constexpr int maxMessagesPerBlock = 256;

    void addMessage(const juce::MidiMessage& message);
    void getMessages(juce::MidiBuffer& destination);
    void clear();
    bool hasMessages() const;
};

// MIDIRoute - Definiert eine MIDI-Routing-Verbindung
struct MIDIRoute
{
    int sourceTrackIndex = -1;
    int targetTrackIndex = -1;
    int channelFilter = -1;  // -1 = alle Kanäle, 0-15 = spezifischer Kanal
    bool enabled = true;
};

// MIDIOutputHandler - Verwaltet alle MIDI-Output-Routings
class MIDIOutputHandler
{
public:
    void addMIDIOutput(int sourceTrackIndex, const juce::MidiBuffer& midiBuffer);
    void getMIDIForTrack(int targetTrackIndex, juce::MidiBuffer& destination);
    void setMidiThruEnabled(bool enabled);
    void setMidiThruChannelFilter(int channel);
};
```

**Verwendung in AudioRoutingGraph::processBlock():**
```cpp
// Am Block-Anfang: Alle MIDI-Queues clearen
midiOutputHandler->clearAllQueues();

// Nach Plugin-Verarbeitung: MIDI-Output abfangen
if (midiOutputHandler && !tempMidiBuffer.isEmpty())
{
    midiOutputHandler->addMIDIOutput(i, tempMidiBuffer);
}

// Vor Track-Verarbeitung: MIDI von anderen Tracks empfangen
if (midiOutputHandler)
{
    juce::MidiBuffer routedMidi;
    midiOutputHandler->getMIDIForTrack(i, routedMidi);
    if (!routedMidi.isEmpty())
        tempMidiBuffer.addEvents(routedMidi, 0, -1, 0);
}
```

### 4.3 Plugin Automation
**Ziel:** Timeline-basierte Parameter-Automation

**Status: ✅ ABGESCHLOSSEN (2026-03-04) - Backend implementiert**

**Dateien:**
- `source/AudioRouting/PluginParameterAutomation.h` (neu) ✅
- `source/AudioRouting/PluginParameterAutomation.cpp` (neu) ✅
- `source/AudioRouting/AudioRoutingGraph.h` (erweitert) ✅
- `source/AudioRouting/AudioRoutingGraph.cpp` (erweitert) ✅
- `source/Timeline/AutomationLane.h` (existiert bereits - Phase 2.3) ✅
- `source/Timeline/AutomationClip.h` (existiert bereits - Phase 2.3) ✅

**Implementierte Features:**
- [x] PluginParameterAutomation-Klasse für Plugin-Parameter Automation
  - PluginParameterMapping struct mit Gain/Min/Max Override
  - PluginParameterInfo struct für Plugin-Parameter Discovery
  - PluginParameterLane mit Linear, Bezier, Step Interpolation
  - Plugin-Parameter-Discovery via `scanPlugins()`
  - Thread-sichere Automation-Verarbeitung
  - `setEnabled()` / `isEnabled()` für globale Automation-Kontrolle
- [x] AudioRoutingGraph mit Plugin-Automation erweitert
  - `addPluginParameterMapping()` - Neue Mapping erstellen
  - `removePluginParameterMapping()` - Mapping entfernen
  - `updatePluginParameterMapping()` - Mapping aktualisieren
  - `setPluginParameterMappingEnabled()` - Mapping aktivieren/deaktivieren
  - `getAllPluginParameterMappings()` - Alle Mappings abrufen
  - `getPluginParameterMappingsForPlugin()` - Mappings pro Plugin abrufen
  - `setPluginAutomationEnabled()` - Alle Automation aktivieren
  - `scanPluginParameters()` - Plugins nach automatisierbaren Parametern scannen
- [x] CMakeLists.txt aktualisiert
- [x] Plugin-Parameter Discovery in `initialize()` integriert (wird automatisch aufgerufen bei Plugin-Insert/Remove/State-Load)
- [ ] UI für Plugin-Parameter Automation (TODO - Phase 5)
- [ ] Parameter-Recording (TODO - Phase 5)

**Implementierung:**
```cpp
// PluginParameterMapping - Verknüpft Automation-Parameter mit Plugin-Parameter
struct PluginParameterMapping
{
    juce::String automationParameterId;  // z.B. "plugin_1001_cutoff"
    uint32_t pluginNodeId = 0;           // NodeID des Plugins
    int pluginParameterIndex = -1;          // Parameter-Index (0-1023)
    float gain = 1.0f;                   // Verstärkungsfaktor
    float minOverride = 0.0f;             // Min-Wert-Override
    float maxOverride = 1.0f;             // Max-Wert-Override
};

// PluginParameterAutomation - Verwaltet alle Plugin-Parameter-Automationen
class PluginParameterAutomation
{
public:
    void scanPlugins(const std::unordered_map<uint32_t, juce::AudioPluginInstance*>& plugins);
    std::unordered_map<uint32_t, std::unordered_map<int, float>> processAutomation(double currentBeat);
};
```

**Noch offen:**
- [ ] UI für Plugin-Parameter Automation
- [ ] Parameter-Recording
- [ ] Parameter-Browse-Dialog

### 4.4 Additional Plugin Formats

| Format | Priorität | Aufwand | Status |
|--------|-----------|---------|--------|
| LV2 | Mittel | JUCE hat LV2PluginFormat seit 7.0 | ✅ ABGESCHLOSSEN (2026-03-04) |
| CLAP | Niedrig | Eigenständige Integration nötig | 🚧 TODO |
| VST2 | Nur Legacy | Steinberg hat Support beendet | ❌ Wird nicht implementiert |

**LV2-Implementierung (ABGESCHLOSSEN):**
- [x] `formatManager.addFormat(new LV2PluginFormat())` in VSTPluginManager::initialize()
- [x] `juce_lv2` Modul zu CMakeLists.txt hinzugefügt
- [x] LV2-Scan Paths konfiguriert (Windows/macOS/Linux)
- [x] Platform-spezifische LV2-Pfade:
  - Windows: `%PROGRAMFILES%\LV2`, `%LOCALAPPDATA%\lv2`
  - macOS: `/Library/Audio/Plug-Ins/LV2`, `~/Library/Audio/Plug-Ins/LV2`
  - Linux: `~/.lv2`, `/usr/local/lib/lv2`, `/usr/lib/lv2`, `/opt/lv2`

**Implementierung:**
```cpp
// In VSTPluginManager::initialize()
#if JUCE_PLUGINHOST_LV2 && !JUCE_DISABLE_FEATURE_ALL
    if (auto* lv2Format = new juce::LV2PluginFormat())
    {
        formatManager.addFormat(lv2Format);
        configureLV2SearchPaths(lv2Format);
    }
#endif
```

**CLAP-Implementierung (TODO):**
- [ ] CLAP-Format manuell integrieren (nicht in JUCE enthalten)
- [ ] CLAP-Scan Paths konfigurieren
- [ ] CLAP-State mit Worker-Support

---

## Phase 5: UI/UX Integration

### 5.1 Unified Track Header
**Ziel:** Konsistente Track-Controls für alle Typen

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/UI/TrackHeader.h` (neu) ✅
- `source/UI/TrackHeader.cpp` (neu) ✅
- `source/TrackComponent.h`

**Implementierte Features:**
- [x] Track-Typ Switcher (Dropdown)
- [x] Plugin-Slot Indicators (visuelle Darstellung)
- [x] Routing-Visualisierung (Sidechain/MIDI)
- [x] Volume/Mute/Solo Controls
- [x] Expand/Collapse Button
- [x] Plugin Button zum Öffnen der Plugin Chain

### 5.2 Plugin Chain Editor
**Ziel:** Visuelle Darstellung der Plugin-Kette

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/UI/PluginChainEditor.h` (neu) ✅
- `source/UI/PluginChainEditor.cpp` (neu) ✅

**Implementierte Features:**
- [x] Visuelle Liste aller Plugins im Track
- [x] Bypass-Buttons pro Plugin
- [x] Remove-Buttons pro Plugin
- [x] Add Plugin Button
- [x] Scrollbare Ansicht für lange Plugin-Ketten
- [x] Empty State anzeigen
- [ ] Drag & Drop Plugin-Reihenfolge (TODO)
- [ ] Quick-Access Parameter (TODO)

### 5.3 Timeline Zoom & Navigation
**Ziel:** Professionelle Timeline-Navigation

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/Timeline/TimelineViewport.h` (neu) ✅
- `source/Timeline/TimelineViewport.cpp` (neu) ✅

**Implementierte Features:**
- [x] Mouse-Wheel Zoom (horizontal & vertikal)
- [x] Drag-to-Scroll (Mittlere Maustaste oder Drag)
- [x] Zoom-In/Out Buttons (+/-)
- [x] Reset Zoom Button (1:1)
- [x] Zoom-Level-Anzeige
- [x] Playhead-Scroll-Tracking (setPlayheadTracking())
- [x] Minimap (kleine Übersicht der Timeline oben rechts)
- [x] Viewport Indicator im Minimap

---

## Phase 6: Performance & Stability

### 6.1 Lock-Free Plugin Loading
**Ziel:** Keine Audio-Glitches beim Laden

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/VSTHost/PluginLoadingCoordinator.h` (neu) ✅
- `source/VSTHost/PluginLoadingCoordinator.cpp` (neu) ✅
- `source/AudioEngine.h` (erweitert) ✅
- `source/AudioEngine.cpp` (erweitert) ✅
- `source/MainComponent.h` (erweitert) ✅
- `source/MainComponent.cpp` (erweitert) ✅
- `CMakeLists.txt` (erweitert) ✅

**Implementierte Features:**
- [x] `PluginLoadingCoordinator` Klasse für lock-free Plugin-Loading
- [x] `LockFreePluginQueue` Template-Klasse für lock-free Queue-Operationen
- [x] `PluginLoadRequest` struct für Load-Requests
- [x] `PluginLoadResult` struct für Load-Results mit Status-Tracking
- [x] `processPendingLoads()` Methode für Audio-Thread-Verarbeitung
- [x] Integration in AudioEngine getNextAudioBlock()
- [x] Initialisierung in MainComponent initializeVSTHosting()
- [x] Callbacks für Plugin-Insert, -Remove und Load-Failure
- [x] Statistik-Methoden (getAverageLoadTimeMs, getTotalPluginsLoaded, etc.)

**Implementierung:**
```cpp
// Lock-Free Queue (SPSC - Single Producer, Single Consumer)
template<typename T, int Capacity>
class LockFreePluginQueue
{
    bool enqueue(T* element);
    T* dequeue();
    bool isEmpty() const;
};

// Plugin Loading Coordinator
class PluginLoadingCoordinator
{
    // UI-Thread: Queue load requests
    uint32_t queueLoadRequest(const PluginLoadRequest& request, callback);

    // Audio-Thread: Process pending loads
    int processPendingLoads();

    // Status queries
    bool hasPendingLoads() const;
    int getNumPendingLoads() const;
};
```

### 6.2 Plugin Sandbox (Optional)
**Ziel:** Abstürze isolieren
**Ziel:** Performance-Metriken sichtbar machen

**Aufgaben:**
- [ ] Per-Plugin CPU-Usage
- [ ] Per-Track CPU-Usage
- [ ] Warning bei hoher Auslastung

---

## Implementierungs-Reihenfolge

```
Woche 1-2:  Phase 1.1 + 1.3 (Interfaces + PlayHead)
Woche 3-4:  Phase 2.1 (Engine Consolidation)
Woche 5-6:  Phase 2.2 (StepSeq als Clip)
Woche 7-8:  Phase 3.1 + 3.2 (Plugin Routing + State)
Woche 9-10: Phase 3.3 (Latency Compensation)
Woche 11+:  Phase 4 & 5 (Advanced Features + UI)
```

---

## Erfolgsmetriken

- [ ] Alle aktuellen StepSequencer-Features in Timeline funktionieren
- [ ] VST3 Plugins laden und spielen ohne Glitches
- [ ] Plugin-State wird korrekt gespeichert/geladen
- [ ] Latency Compensation funktioniert (Phase-Alignment Test)
- [ ] MIDI-Sync von Plugins korrekt (Arpeggiator Test)
- [ ] Projekt mit 10+ Tracks + 20+ Plugins stabil

---

## Plugin-Kompatibilitäts-Checkliste

Test mit diesen Plugins:

| Plugin | Typ | Test-Case |
|--------|-----|-----------|
| Serum | VST3 | Wavetable-Synth, GUI, Automation |
| Vital | VST3 | Open-Source WT-Synth |
| Spitfire BBC SO | VST3 | Large Library Loading |
| FabFilter Pro-Q 3 | VST3 | Parameter-Automation |
| Valhalla VintageVerb | VST3 | Tail Handling |
|OTT | VST3 | Multiband Compression |
| TAL-Noisemaker | VST3 | Einfacher Synth |
| Dexed | VST3 | FM-Synth, MIDI-Out |
| Reaper ReaPlugs | VST3 | Basic Effects Suite |

---

## Risiko-Matrix

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| Plugin-Absturz | Mittel | Hoch | Sandbox (Phase 6.2) |
| Latency-Bugs | Mittel | Mittel | Extensive Tests |
| State-Loss | Niedrig | Hoch | Backup-Mechanismus |
| Performance | Niedrig | Mittel | Profiling-Tools |

### 6.2 Plugin Sandbox (Optional)
**Ziel:** Abstürze isolieren

**Status: ⚠️ DEAKTIVIERT (2026-03-04)**

**Dateien:**
- `source/VSTHost/PluginSandbox.h` (neu) ✅
- `source/VSTHost/PluginSandbox.cpp` (neu) ✅
- `source/MainComponent.h` (erweitert) ✅
- `source/MainComponent.cpp` (erweitert) ✅
- `CMakeLists.txt` (auskommentiert) ⚠️

**Implementierte Features:**
- [x] `PluginSandboxManager` - Host-seitige Verwaltung aller Sandboxes
- [x] `PluginSandboxWorker` - Sandbox-seitige Prozess-Isolation
- [x] Shared Memory für Audio/MIDI-Exchange (Windows/macOS/Linux)
- [x] IPC Channels (Shared Memory, Named Pipes, Event Signals)
- [x] Auto-Recovery bei Plugin-Abstürzen
- [x] CPU-Usage Monitoring pro Sandbox
- [x] Crash Detection & Reporting
- [x] Graceful Shutdown Mechanism

**Hinweis:** Diese Phase wurde erstellt, aber aufgrund von plattform-spezifischen Compilerfehlern (SharedMemoryHandle, sigaction, etc.) vorerst deaktiviert. Die Dateien sind vorhanden und können später reaktiviert werden, wenn:
- Die plattform-spezifischen Header und APIs korrekt inkludiert werden
- Oder eine vereinfachte Version ohne komplexe IPC-Mechanismen implementiert wird

**Fehler:**
- `SharedMemoryHandle` existiert nicht in juce Namespace (plattform-spezifisch)
- `sigaction` ist nicht auf Windows verfügbar
- Komplexe Typumwandlungen und Template-Fehler

**Implementierung:**
```cpp
// Shared Memory Header
struct SharedMemoryHeader
{
    uint32_t magic = 0x53414E44;  // "SAND"
    std::atomic<bool> audioReady;
    std::atomic<bool> midiReady;
    std::atomic<uint64_t> processingTimeNs;
};

// Sandbox Manager (Host Process)
class PluginSandboxManager
{
    uint32_t launchSandbox(const SandboxConfig& config);
    void terminateSandbox(uint32_t sandboxId, bool graceful);
    bool restartSandbox(uint32_t sandboxId);
    void sendAudioToSandbox(...);
    bool receiveAudioFromSandbox(...);
};
```

**Sicherheitsfeatures:**
- Prozess-Isolation pro Plugin
- Crash Detection via Signal Handlers
- Auto-Recovery mit konfigurierbaren Max-Restart-Attempts
- CPU-Monitoring mit Warnungen
- Graceful Shutdown via Status Flags

**Plattform-Spezifika:**
- Windows: Named Shared Memory (`CreateFileMapping`)
- macOS: POSIX Shared Memory (`shm_open`, `mmap`)
- Linux: POSIX Shared Memory (`shm_open`, `mmap`)

### 6.3 CPU Profiling

**Dateien:**
- `source/VSTHost/PluginSandbox.h` (neu) ✅
- `source/VSTHost/PluginSandbox.cpp` (neu) ✅
- `source/MainComponent.h` (erweitert) ✅
- `source/MainComponent.cpp` (erweitert) ✅
- `CMakeLists.txt` (erweitert) ✅

**Implementierte Features:**
- [x] `PluginSandboxManager` - Host-seitige Verwaltung aller Sandboxes
- [x] `PluginSandboxWorker` - Sandbox-seitige Prozess-Isolation
- [x] Shared Memory für Audio/MIDI-Exchange (Windows/macOS/Linux)
- [x] IPC Channels (Shared Memory, Named Pipes, Event Signals)
- [x] Auto-Recovery bei Plugin-Abstürzen
- [x] CPU-Usage Monitoring pro Sandbox
- [x] Crash Detection & Reporting
- [x] Graceful Shutdown Mechanism
- [x] Konfigurierbare Timeouts und Max-Restart-Attempts

**Implementierung:**
```cpp
// Shared Memory Header
struct SharedMemoryHeader
{
    uint32_t magic = 0x53414E44;  // "SAND"
    std::atomic<bool> audioReady;
    std::atomic<bool> midiReady;
    std::atomic<uint64_t> processingTimeNs;
    // ...
};

// Sandbox Manager (Host Process)
class PluginSandboxManager
{
    uint32_t launchSandbox(const SandboxConfig& config);
    void terminateSandbox(uint32_t sandboxId, bool graceful);
    bool restartSandbox(uint32_t sandboxId);
    void sendAudioToSandbox(...);
    bool receiveAudioFromSandbox(...);
    // ...
};

// Sandbox Worker (Sandbox Process)
class PluginSandboxWorker
{
    int run(int argc, char* argv[]);
    bool attachToSharedMemory(...);
    void processAudioBlock();
    void setupSignalHandlers();
    // ...
};
```

**Plattform-Spezifika:**
- Windows: Named Shared Memory (`CreateFileMapping`)
- macOS: POSIX Shared Memory (`shm_open`, `mmap`)
- Linux: POSIX Shared Memory (`shm_open`, `mmap`)

**Sicherheitsfeatures:**
- Prozess-Isolation pro Plugin
- Crash Detection via Signal Handlers
- Auto-Recovery mit konfigurierbaren Versuchen
- CPU-Monitoring mit Warnungen
- Graceful Shutdown via Status Flags

**Hinweis:** Die Sandbox-Worker-Exe muss separat kompiliert werden
(ein separates CMake-Target) um in einem isolierten Prozess zu laufen.

### 6.3 CPU Profiling
**Ziel:** Performance-Metriken sichtbar machen

**Status: ✅ ABGESCHLOSSEN (2026-03-04)**

**Dateien:**
- `source/AudioRouting/CPUProfiler.h` (neu) ✅
- `source/AudioRouting/CPUProfiler.cpp` (neu) ✅
- `source/AudioEngine.h` (erweitert) ✅
- `source/AudioEngine.cpp` (erweitert) ✅
- `source/MainComponent.h` (erweitert) ✅
- `source/MainComponent.cpp` (erweitert) ✅
- `CMakeLists.txt` (erweitert) ✅

**Implementierte Features:**
- [x] `CPUProfiler` Klasse für Performance-Metriken
- [x] `CPUStats` struct für Statistiken pro Komponente
- [x] `CPUMeasurement` struct für nanosekundengenaue Messungen
- [x] `ComponentID` für eindeutige Komponenten-Identifikation
- [x] `ProfilingComponent` Enum für Komponenten-Typen (Track, Plugin, Effect, etc.)
- [x] Per-Track CPU Usage Messung
- [x] Per-Plugin CPU Usage Messung
- [x] Globaler CPU Durchschnitt & Peak
- [x] Warnungen bei hoher Auslastung (configurierbar)
- [x] WarningLevel Enum (None, Low, Medium, High, Critical)
- [x] `ScopedCPUMeasure` RAII-Helper für automatische Messungen
- [x] Historische Daten (gleitender Durchschnitt über 100 Messungen)
- [x] Thread-sichere Statistiken mit atomic
- [x] Callbacks für Warnungen an UI
- [x] Reset-Methoden für Statistiken

**Implementierung:**
```cpp
// CPU Stats mit Historie
struct CPUStats
{
    float currentCPU;       // Aktuelle CPU (0.0 - 1.0)
    float peakCPU;           // Maximaler CPU-Wert
    float averageCPU;        // Gleitender Durchschnitt
    int warningCount;        // Anzahl der Warnungen
    std::deque<float> history;  // Ring-Buffer für Durchschnitt
};

// CPU Profiler
class CPUProfiler
{
    void startMeasure(ProfilingComponent type, int index);
    void endMeasure(ProfilingComponent type, int index);
    float getTrackCPU(int trackIndex);
    float getPluginCPU(uint32_t pluginNodeId);
    float getGlobalAverageCPU();
    // ...
};

// RAII Helper
class ScopedCPUMeasure
{
    ScopedCPUMeasure(CPUProfiler& profiler, ProfilingComponent type, int index = 0);
    ~ScopedCPUMeasure();  // Automatisch endMeasure()
};
```

**Warnungsschwellwerte (konfigurierbar):**
- Low: 70% - 85%
- Medium: 85% - 95%
- High: 95% - 98%
- Critical: > 98%

**Komponenten-Typen:**
- GlobalEngine - Gesamte Audio-Engine
- Track - Einzelner Track (0-15)
- Plugin - Einzelnes Plugin (NodeID)
- Effect - Effekt (z.B. Reverb)
- Sandbox - Plugin Sandbox
- InternalSynth - Interne Synthesizer
- Timeline - Timeline Renderer
- Automation - Automation Processing
- MasterOutput - Master Output

**UI-Integration:**
- CPU-Warnungen als AlertWindow bei kritischer Last
- Debug-Logging für Warnungen
- Vorbereitete Integration für Track Header / Plugin Chain Editor
