#pragma once

/**
 * SidechainManager - Verwaltet Sidechain-Routings zwischen Tracks und Plugins
 *
 * Sidechain ermöglicht es, Audio von einem Track als Steuersignal für ein Plugin
 * auf einem anderen Track zu nutzen (z.B. Ducking für Vocals, Sidechain-Compression
 * für Bass/Kick-Interaction).
 *
 * Features:
 *   - Track-to-Track Sidechain Routing
 *   - Plugin Sidechain-Input Detection
 *   - Thread-Safe Routing-Konfiguration
 *   - Visualisierung für UI
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <memory>
#include <atomic>
#include <unordered_map>

//==============================================================================
/**
 * SidechainRoute - Definiert eine einzelne Sidechain-Verbindung
 *
 * Eine Route definiert:
 *   - sourceTrackIndex: Welcher Track liefert das Sidechain-Signal
 *   - targetTrackIndex: Welcher Track erhält das Signal
 *   - targetPluginNodeId: Welches Plugin empfängt das Signal (0 = alle Plugins auf Track)
 *   - busIndex: Welcher Sidechain-Bus wird genutzt
 *   - gain: Verstärkung des Sidechain-Signals
 */
struct SidechainRoute
{
    int sourceTrackIndex = -1;       // -1 = keine Route
    int targetTrackIndex = -1;       // -1 = keine Route
    uint32_t targetPluginNodeId = 0;  // 0 = alle Plugins auf Track
    int busIndex = 0;                // Sidechain-Bus-Index (meist 0-3)
    float gain = 1.0f;              // Verstärkung (0.0 - 2.0)
    bool enabled = true;             // Route aktiv/inaktiv

    bool isValid() const
    {
        return sourceTrackIndex >= 0 && targetTrackIndex >= 0 &&
               sourceTrackIndex != targetTrackIndex && enabled;
    }

    juce::String toString() const
    {
        return juce::String::formatted(
            "Track %d -> Track %d (Plugin: %u, Bus: %d, Gain: %.2f)",
            sourceTrackIndex, targetTrackIndex, targetPluginNodeId, busIndex, gain
        );
    }
};

//==============================================================================
/**
 * SidechainBuffer - Verwaltet den Sidechain-Buffer für einen Track
 *
 * Jeder Track hat einen Sidechain-Buffer, der das gemischte Sidechain-Signal
 * aller Quell-Tracks enthält. Dieser Buffer wird an Plugins mit Sidechain-Input
 * weitergeleitet.
 */
class SidechainBuffer
{
public:
    static constexpr int maxBufferChannels = 2;  // Stereo Sidechain
    static constexpr int maxBufferSize = 4096;

    SidechainBuffer();

    void prepare(double sampleRate, int maxSamplesPerBlock);
    void clear();

    /** Add samples to the sidechain buffer (mixes with existing content) */
    void addBuffer(const juce::AudioBuffer<float>& source, int numSamples, float gain = 1.0f);

    /** Get the sidechain buffer for routing to plugins */
    juce::AudioBuffer<float>& getBuffer() { return buffer; }
    const juce::AudioBuffer<float>& getBuffer() const { return buffer; }

    /** Check if buffer has content (for optimization) */
    bool hasContent() const { return hasContentFlag.load(); }
    void setHasContent(bool flag) { hasContentFlag.store(flag); }

private:
    juce::AudioBuffer<float> buffer;
    std::atomic<bool> hasContentFlag{false};
    double currentSampleRate = 44100.0;
};

//==============================================================================
/**
 * SidechainPluginInfo - Informationen über Sidechain-Fähigkeiten eines Plugins
 *
 * Ermöglicht Erkennung von Plugins mit Sidechain-Input für UI-Integration.
 */
struct SidechainPluginInfo
{
    uint32_t nodeId = 0;
    juce::String pluginName;
    int trackIndex = -1;
    bool hasSidechainInput = false;
    int numSidechainBuses = 0;
    std::vector<int> sidechainBusChannels;  // Anzahl Channels pro Bus

    juce::String getSidechainBusInfo() const
    {
        if (!hasSidechainInput)
            return "No sidechain support";

        juce::String info = "Sidechain buses: " + juce::String(numSidechainBuses) + " (";
        for (size_t i = 0; i < sidechainBusChannels.size(); ++i)
        {
            info += juce::String(sidechainBusChannels[i]);
            if (i < sidechainBusChannels.size() - 1)
                info += ", ";
        }
        info += ")";
        return info;
    }
};

//==============================================================================
/**
 * SidechainManager - Zentrale Verwaltung aller Sidechain-Routings
 *
 * Diese Klasse verwaltet:
 *   - Alle aktiven Sidechain-Routings
 *   - Sidechain-Buffer pro Track
 *   - Plugin Sidechain-Fähigkeiten
 *   - Thread-Safe Routing-Konfiguration
 *
 * Workflow:
 *   1. Am Anfang eines Audio-Blocks: Alle Sidechain-Buffer clearen
 *   2. Nach Track-Verarbeitung: Source-Track in Buffer schreiben
 *   3. Vor Plugin-Verarbeitung: Sidechain-Buffer an Plugins weitergeben
 *   4. Am Ende eines Audio-Blocks: Buffer clearen für nächsten Block
 */
class SidechainManager
{
public:
    static constexpr int maxTracks = 16;
    static constexpr int maxRoutes = 64;

    SidechainManager();
    ~SidechainManager();

    /** Initialisierung mit Sample-Rate und Block-Size */
    void prepare(double sampleRate, int maxSamplesPerBlock);
    void releaseResources();

    // ============================================================
    // Routing-Konfiguration
    // ============================================================

    /** Neue Sidechain-Route hinzufügen.
        @return Route-ID oder -1 bei Fehler */
    int addRoute(const SidechainRoute& route);

    /** Route entfernen */
    bool removeRoute(int routeId);

    /** Route aktualisieren */
    bool updateRoute(int routeId, const SidechainRoute& route);

    /** Route aktivieren/deaktivieren */
    bool setRouteEnabled(int routeId, bool enabled);

    /** Route abrufen */
    const SidechainRoute* getRoute(int routeId) const;
    SidechainRoute* getRoute(int routeId);

    /** Alle Routings für einen Target-Track abrufen */
    std::vector<SidechainRoute*> getRoutesForTarget(int targetTrackIndex);
    std::vector<const SidechainRoute*> getRoutesForTarget(int targetTrackIndex) const;

    /** Alle Routings für einen Source-Track abrufen */
    std::vector<SidechainRoute*> getRoutesForSource(int sourceTrackIndex);

    /** Alle Routings abrufen (für UI-Visualisierung) */
    const std::vector<SidechainRoute>& getAllRoutes() const { return routes; }

    /** Route nach ID suchen */
    const SidechainRoute* findRouteByNodeId(uint32_t targetPluginNodeId) const;

    // ============================================================
    // Audio-Processing (Audio-Thread)
    // ============================================================

    /** Alle Sidechain-Buffer am Block-Anfang clearen */
    void clearAllBuffers();

    /** Track-Audio zum Sidechain-Buffer hinzufügen */
    void addSourceToBuffer(int sourceTrackIndex,
                           const juce::AudioBuffer<float>& audio,
                           int numSamples);

    /** Sidechain-Buffer für einen Track abrufen (für Plugin-Input) */
    SidechainBuffer* getSidechainBuffer(int trackIndex);
    const SidechainBuffer* getSidechainBuffer(int trackIndex) const;

    // ============================================================
    // Plugin Sidechain-Fähigkeiten
    // ============================================================

    /** Plugin Sidechain-Informationen registrieren (wird von AudioRoutingGraph aufgerufen) */
    void registerPluginSidechainInfo(const SidechainPluginInfo& info);

    /** Alle Plugin Sidechain-Informationen löschen (für rescan) */
    void clearPluginSidechainInfo() { pluginSidechainInfos.clear(); }

    /** Plugin Sidechain-Informationen abrufen */
    const SidechainPluginInfo* getPluginSidechainInfo(uint32_t nodeId) const;

    /** Alle Plugins mit Sidechain-Input für einen Track abrufen */
    std::vector<SidechainPluginInfo> getPluginsWithSidechainForTrack(int trackIndex) const;

    // ============================================================
    // UI-Visualisierung
    // ============================================================

    /** Prüfen, ob ein Track als Sidechain-Source genutzt wird */
    bool isTrackUsedAsSource(int trackIndex) const;

    /** Prüfen, ob ein Track als Sidechain-Target genutzt wird */
    bool isTrackUsedAsTarget(int trackIndex) const;

    /** Anzahl aktiver Routings abrufen */
    int getActiveRouteCount() const;

    /** Debug-Informationen ausgeben */
    juce::String getDebugInfo() const;

private:
    // Routing-Konfiguration
    std::vector<SidechainRoute> routes;
    int nextRouteId = 1;  // IDs beginnen bei 1 (0 = ungültig)

    // Sidechain-Buffer pro Track
    std::array<std::unique_ptr<SidechainBuffer>, maxTracks> sidechainBuffers;

    // Plugin Sidechain-Informationen
    std::unordered_map<uint32_t, SidechainPluginInfo> pluginSidechainInfos;

    // Audio-Parameter
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainManager)
};
