#pragma once

/**
 * PluginParameterAutomation - Automatisiert Plugin-Parameter über Timeline
 *
 * Diese Klasse ermöglicht die Automation von VST-Plugin-Parametern
 * über die Timeline-Architektur, ähnlich wie Wavetable-Automation.
 *
 * Features:
 *   - Plugin-Parameter Mapping (Parameter-ID zu Plugin-Parameter)
 *   - Automation-Werte für Plugin-Parameter anwenden
 *   - Bypass-Support für einzelne Automationen
 *   - Plugin-Parameter-Discovery (welche Parameter können automiert werden)
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>

//==============================================================================
/**
 * PluginParameterMapping - Verknüpft Automation-Parameter mit Plugin-Parameter
 *
 * Eine Mapping definiert:
 *   - automationParameterId: Die eindeutige ID des Automation-Parameters
 *   - pluginNodeId: Der NodeID des Plugins
 *   - pluginParameterIndex: Der Index des Plugin-Parameters (für AudioPluginInstance::setParameter)
 *   - enabled: Automation aktiv/inaktiv
 *   - gain: Verstärkungsfaktor für Automation-Werte
 */
struct PluginParameterMapping
{
    juce::String automationParameterId;  // z.B. "plugin_1001_cutoff"
    uint32_t pluginNodeId = 0;           // NodeID des Plugins
    int pluginParameterIndex = -1;          // Parameter-Index (0-1023)
    bool enabled = true;                    // Automation aktiv/inaktiv
    float gain = 1.0f;                   // Gain-Faktor (0.0-2.0)
    float minOverride = 0.0f;             // Min-Wert-Override (optional)
    float maxOverride = 1.0f;             // Max-Wert-Override (optional)

    bool isValid() const
    {
        return automationParameterId.isNotEmpty() &&
               pluginNodeId > 0 &&
               pluginParameterIndex >= 0;
    }

    juce::String toString() const
    {
        return juce::String::formatted(
            "Plugin %u Param %d (ID: %s)",
            pluginNodeId, pluginParameterIndex,
            automationParameterId.toRawUTF8()
        );
    }
};

//==============================================================================
/**
 * PluginParameterInfo - Informationen über einen automatisierbaren Plugin-Parameter
 *
 * Erfasst alle Informationen die für die UI benötigt werden:
 *   - Parameter-Name, Label, Kategorie
 *   - Wertebereich (Min, Max, Default)
 *   - Schrittweite, Automatisierbar?
 *   - Einheiten (Hz, %, dB, etc.)
 */
struct PluginParameterInfo
{
    juce::String parameterId;         // Eindeutige ID
    juce::String name;                // Anzeigename
    juce::String label;               // Kurz-Beschreibung
    juce::String category;            // Kategorie (z.B. "Filter", "Envelope")
    float minValue = 0.0f;              // Minimaler Wert
    float maxValue = 1.0f;              // Maximaler Wert
    float defaultValue = 0.0f;          // Standardwert
    float stepSize = 0.01f;            // Schrittweite
    bool isAutomatable = true;          // Kann automatisiert werden?
    bool isDiscrete = false;            // Diskreter oder kontinuierlich?
    juce::String unit;               // Einheit (Hz, %, dB, etc.)
    int numSteps = 0;                // Bei diskreten Parametern: Anzahl Schritte

    juce::String getRangeString() const
    {
        if (isDiscrete && numSteps > 0)
            return juce::String(numSteps) + " steps";

        juce::String minStr(minValue, 2);
        juce::String maxStr(maxValue, 2);
        return "[" + minStr + " - " + maxStr + "]";
    }
};

//==============================================================================
/**
 * PluginParameterLane - Automation-Lane für einen Plugin-Parameter
 *
 * Speichert alle Automation-Clips für einen bestimmten Plugin-Parameter
 * und interpoliert Werte für den Audio-Thread.
 */
class PluginParameterLane
{
public:
    PluginParameterLane(const juce::String& parameterId = {});

    // === Parameter Info ===
    void setParameterInfo(const PluginParameterInfo& info) { parameterInfo = info; }
    const PluginParameterInfo& getParameterInfo() const { return parameterInfo; }

    // === Clip Management ===
    void addClip(double startBeat, double endBeat, float startValue, float endValue,
               float curve = 0.0f);
    void removeClip(int clipIndex);
    int getNumClips() const { return static_cast<int>(clips.size()); }

    // === Value Lookup ===
    /**
     * Get the automation value at a given beat position.
     * Supports linear, bezier, and step interpolation.
     */
    float getValueAtBeat(double beatPosition) const;

    // === State ===
    void setActive(bool active) { isActive = active; }
    bool getActive() const { return isActive; }

private:
    struct AutomationClip
    {
        double startBeat = 0.0;
        double endBeat = 0.0;
        float startValue = 0.0f;
        float endValue = 0.0f;
        float curve = 0.0f;  // 0 = linear, >0 = bezier tension
    };

    juce::String parameterId;
    PluginParameterInfo parameterInfo;
    std::vector<AutomationClip> clips;
    bool isActive = true;

    /** Find active clip at given position */
    const AutomationClip* findActiveClip(double beatPosition) const;
};

//==============================================================================
/**
 * PluginParameterAutomation - Verwaltet alle Plugin-Parameter-Automationen
 *
 * Diese Klasse:
 *   - Verwaltet alle Plugin-Parameter-Mappings
 *   - Stellt Automation-Werte für Audio-Engine bereit
 *   - Unterstützt Plugin-Parameter-Discovery
 *   - Ist Thread-sicher für Audio-Thread-Zugriff
 */
class PluginParameterAutomation
{
public:
    PluginParameterAutomation();
    ~PluginParameterAutomation();

    // === Plugin-Parameter Discovery ===
    /**
     * Scan all plugins and discover automatable parameters.
     * Called when plugins are loaded into the graph.
     */
    void scanPlugins(const std::unordered_map<uint32_t, juce::AudioPluginInstance*>& plugins);

    /**
     * Get all automatable parameters for a plugin.
     */
    std::vector<PluginParameterInfo> getPluginParameters(uint32_t pluginNodeId) const;

    /**
     * Get a specific plugin parameter info.
     */
    const PluginParameterInfo* getParameterInfo(uint32_t pluginNodeId, int paramIndex) const;

    // === Mapping Management ===
    /**
     * Create a new parameter mapping.
     * @return Mapping ID (index in mappings array)
     */
    int addMapping(const PluginParameterMapping& mapping);

    /**
     * Remove a mapping by ID.
     */
    bool removeMapping(int mappingId);

    /**
     * Update a mapping.
     */
    bool updateMapping(int mappingId, const PluginParameterMapping& mapping);

    /**
     * Get a mapping by ID.
     */
    const PluginParameterMapping* getMapping(int mappingId) const;

    /**
     * Get all mappings.
     */
    const std::vector<PluginParameterMapping>& getAllMappings() const { return mappings; }

    /**
     * Get mappings for a specific plugin.
     */
    std::vector<PluginParameterMapping*> getMappingsForPlugin(uint32_t pluginNodeId);

    // === Value Processing ===
    /**
     * Process all automation for current playhead position.
     * Returns a map of pluginNodeId to parameter values.
     */
    std::unordered_map<uint32_t, std::unordered_map<int, float>> processAutomation(
        double currentBeat);

    /**
     * Get automation value for a specific plugin parameter.
     */
    float getParameterValue(uint32_t pluginNodeId, int paramIndex, double beat, float defaultValue = 0.5f) const;

    // === Bypass Management ===
    /**
     * Enable/disable a mapping (bypass individual automation).
     */
    bool setMappingEnabled(int mappingId, bool enabled);

    /**
     * Check if a mapping is enabled.
     */
    bool isMappingEnabled(int mappingId) const;

    /**
     * Enable/disable all plugin parameter automation.
     */
    void setEnabled(bool enabled);

    /**
     * Check if plugin parameter automation is enabled.
     */
    bool isEnabled() const { return automationEnabled.load(); }

    // === Clear ===
    /** Clear all mappings */
    void clearMappings();

    /** Clear all automation lanes */
    void clearLanes();

    // === Debug ===
    /** Get debug information */
    juce::String getDebugInfo() const;

private:
    // Mappings
    std::vector<PluginParameterMapping> mappings;
    int nextMappingId = 1;

    // Plugin parameter cache (discovered from plugins)
    std::unordered_map<uint32_t, std::vector<PluginParameterInfo>> pluginParameters;

    // Automation lanes (per parameter)
    std::unordered_map<juce::String, std::unique_ptr<PluginParameterLane>> lanes;

    std::atomic<bool> automationEnabled{true};
    mutable std::mutex automationMutex;

    /** Create parameter ID from plugin node ID and param index */
    juce::String createParameterId(uint32_t pluginNodeId, int paramIndex) const;
};
