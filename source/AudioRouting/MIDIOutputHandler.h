#pragma once

/**
 * MIDIOutputHandler - Verwaltet MIDI-Output von Plugins
 *
 * Diese Klasse ermöglicht Plugins (z.B. Arpeggiators, Step-Sequencer)
 * MIDI-Messages zu generieren, die an andere Tracks weitergeleitet werden.
 *
 * Features:
 *   - MIDI-Output von Plugins abfangen
 *   - MIDI-Routing zu anderen Tracks
 *   - MIDI-Thru (MIDI-Durchleitung)
 *   - MIDI-Filter-Kanal für selektives Routing
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <functional>
#include <memory>

//==============================================================================
/**
 * MIDIRoute - Definiert eine MIDI-Routing-Verbindung
 *
 * Eine Route definiert:
 *   - sourceTrackIndex: Welcher Track generiert das MIDI
 *   - targetTrackIndex: Welcher Track empfängt das MIDI
 *   - channelFilter: Welcher MIDI-Kanal wird geroutet (-1 = alle Kanäle)
 *   - enabled: Route aktiv/inaktiv
 */
struct MIDIRoute
{
    int sourceTrackIndex = -1;    // -1 = keine Route
    int targetTrackIndex = -1;    // -1 = keine Route
    int channelFilter = -1;       // -1 = alle Kanäle, 0-15 = spezifischer Kanal
    bool enabled = true;           // Route aktiv/inaktiv

    bool isValid() const
    {
        return sourceTrackIndex >= 0 && targetTrackIndex >= 0 &&
               sourceTrackIndex != targetTrackIndex && enabled;
    }

    juce::String toString() const
    {
        juce::String channelStr = (channelFilter >= 0 && channelFilter <= 15)
            ? " Ch " + juce::String(channelFilter + 1)
            : " All Ch";

        return juce::String::formatted(
            "Track %d MIDI -> Track %d%s",
            sourceTrackIndex + 1, targetTrackIndex + 1, channelStr.toRawUTF8()
        );
    }
};

//==============================================================================
/**
 * PluginMIDIOutputInfo - Informationen über MIDI-Output-Fähigkeiten eines Plugins
 *
 * Ermöglicht Erkennung von Plugins mit MIDI-Output für UI-Integration.
 */
struct PluginMIDIOutputInfo
{
    uint32_t nodeId = 0;
    juce::String pluginName;
    int trackIndex = -1;
    bool producesMidi = false;
    bool isInstrument = false;
    juce::String description;  // z.B. "Arpeggiator", "Sequencer", "Chord Generator"

    juce::String getDescription() const
    {
        if (!producesMidi)
            return "No MIDI output";

        juce::String info = isInstrument ? "Instrument" : "Effect";
        if (description.isNotEmpty())
            info += " (" + description + ")";
        return info;
    }
};

//==============================================================================
/**
 * MIDIMessageQueue - Thread-sichere Queue für MIDI-Messages
 *
 * Da Plugins MIDI im Audio-Thread generieren und Tracks diese
 * im selben Thread empfangen müssen, ist eine lock-free Queue
 * für die Kommunikation wichtig.
 */
class MIDIMessageQueue
{
public:
    static constexpr int maxMessagesPerBlock = 256;  // Max Messages per audio block

    MIDIMessageQueue();
    ~MIDIMessageQueue();

    /** Add a MIDI message to the queue (from plugin processing) */
    void addMessage(const juce::MidiMessage& message);

    /** Get all messages from the queue and clear it (for track processing) */
    void getMessages(juce::MidiBuffer& destination);

    /** Clear all messages */
    void clear();

    /** Get message count */
    int getMessageCount() const { return messageCount.load(); }

    /** Check if queue has messages */
    bool hasMessages() const { return messageCount.load() > 0; }

private:
    juce::MidiMessage messageBuffer[maxMessagesPerBlock];
    std::atomic<int> messageCount{0};
    int readIndex = 0;
    int writeIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIMessageQueue)
};

//==============================================================================
/**
 * MIDIOutputHandler - Zentrale Verwaltung aller MIDI-Output-Routings
 *
 * Diese Klasse verwaltet:
 *   - Alle aktiven MIDI-Routings
 *   - MIDI-Queues pro Track
 *   - Plugin MIDI-Output-Fähigkeiten
 *   - Thread-Safe Routing-Konfiguration
 *
 * Workflow:
 *   1. Nach Plugin-Verarbeitung: MIDI-Output in Queue speichern
 *   2. Vor Track-Verarbeitung: MIDI an Ziel-Tracks weitergeben
 *   3. MIDI-Thru: Original MIDI an andere Tracks weiterleiten
 */
class MIDIOutputHandler
{
public:
    static constexpr int maxTracks = 16;
    static constexpr int maxRoutes = 64;

    MIDIOutputHandler();
    ~MIDIOutputHandler();

    /** Initialisierung */
    void initialize(int numTracks);

    /** MIDI-Output von einem Plugin zur Queue hinzufügen */
    void addMIDIOutput(int sourceTrackIndex, const juce::MidiBuffer& midiBuffer);

    /** MIDI-Messages für einen Track abrufen (aus allen aktiven Routings) */
    void getMIDIForTrack(int targetTrackIndex, juce::MidiBuffer& destination);

    /** Original MIDI vom Sequencer weiterleiten (MIDI-Thru) */
    void addOriginalMidi(int trackIndex, const juce::MidiBuffer& midiBuffer);

    /** MIDI-Queues am Block-Anfang clearen */
    void clearAllQueues();

    // ============================================================
    // Routing-Konfiguration
    // ============================================================

    /** Neue MIDI-Route hinzufügen.
        @return Route-ID oder -1 bei Fehler */
    int addRoute(const MIDIRoute& route);

    /** Route entfernen */
    bool removeRoute(int routeId);

    /** Route aktivieren/deaktivieren */
    bool setRouteEnabled(int routeId, bool enabled);

    /** Route abrufen */
    const MIDIRoute* getRoute(int routeId) const;

    /** Alle Routings für einen Target-Track abrufen */
    std::vector<MIDIRoute*> getRoutesForTarget(int targetTrackIndex);
    std::vector<const MIDIRoute*> getRoutesForTarget(int targetTrackIndex) const;

    /** Alle Routings abrufen (für UI-Visualisierung) */
    const std::vector<MIDIRoute>& getAllRoutes() const { return routes; }

    // ============================================================
    // MIDI-Thru (MIDI-Durchleitung)
    // ============================================================

    /** MIDI-Thru aktivieren/deaktivieren (globale Einstellung) */
    void setMidiThruEnabled(bool enabled) { midiThruEnabled.store(enabled); }

    /** MIDI-Thru Status abrufen */
    bool isMidiThruEnabled() const { return midiThruEnabled.load(); }

    /** MIDI-Thru Kanal-Filter setzen (-1 = alle Kanäle) */
    void setMidiThruChannelFilter(int channel) { midiThruChannelFilter.store(channel); }

    /** MIDI-Thru Kanal-Filter abrufen */
    int getMidiThruChannelFilter() const { return midiThruChannelFilter.load(); }

    // ============================================================
    // Plugin MIDI-Output-Fähigkeiten
    // ============================================================

    /** Plugin MIDI-Output-Informationen registrieren */
    void registerPluginMIDIInfo(const PluginMIDIOutputInfo& info);

    /** Plugin MIDI-Output-Informationen abrufen */
    const PluginMIDIOutputInfo* getPluginMIDIInfo(uint32_t nodeId) const;

    /** Alle Plugins mit MIDI-Output abrufen */
    std::vector<PluginMIDIOutputInfo> getPluginsWithMIDIOutput() const;

    // ============================================================
    // UI-Visualisierung
    // ============================================================

    /** Prüfen, ob ein Track als MIDI-Source genutzt wird */
    bool isTrackUsedAsMidiSource(int trackIndex) const;

    /** Prüfen, ob ein Track als MIDI-Target genutzt wird */
    bool isTrackUsedAsMidiTarget(int trackIndex) const;

    /** Anzahl aktiver Routings abrufen */
    int getActiveRouteCount() const;

    /** Debug-Informationen ausgeben */
    juce::String getDebugInfo() const;

private:
    // Routing-Konfiguration
    std::vector<MIDIRoute> routes;
    int nextRouteId = 1;  // IDs beginnen bei 1 (0 = ungültig)

    // MIDI-Queues pro Track
    std::vector<std::unique_ptr<MIDIMessageQueue>> midiQueues;

    // Original MIDI für MIDI-Thru (vom Sequencer generiert)
    std::vector<juce::MidiBuffer> originalMidiBuffers;

    // Plugin MIDI-Output-Informationen
    std::unordered_map<uint32_t, PluginMIDIOutputInfo> pluginMidiInfos;

    // MIDI-Thru Einstellungen
    std::atomic<bool> midiThruEnabled{false};
    std::atomic<int> midiThruChannelFilter{-1};

    int numTracks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIOutputHandler)
};
