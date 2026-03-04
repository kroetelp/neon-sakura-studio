#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include <vector>
#include <functional>

#include "PluginHostConfiguration.h"

class PluginInstance;
class PluginScanner;

/**
 * VSTPluginManager - Central manager for VST hosting in Neon Sakura Studio
 *
 * Responsibilities:
 *   - Manage AudioPluginFormatManager for loading VST3/AU plugins
 *   - Maintain AudioProcessorGraph for plugin routing
 *   - Handle plugin lifecycle (load, unload, bypass)
 *   - Coordinate with TrackManager for track-plugin assignments
 *   - Persist known plugin list to disk (XML in AppData)
 *
 * Architecture:
 *
 *   VSTPluginManager
 *       ├── AudioPluginFormatManager (scans/loads plugin binaries)
 *       │   └── VST3PluginFormat, AUPluginFormat (platform-specific)
 *       ├── AudioProcessorGraph (routing graph for plugins)
 *       ├── KnownPluginList (cached plugin descriptions)
 *       └── PluginScanner (background plugin scanning)
 *
 * Startup Flow:
 *   1. initialize() - Add default formats (VST3, AU)
 *   2. loadPluginList() - Load cached plugin list from XML
 *   3. If cache empty or outdated, start background scan
 *   4. UI displays known plugins from getKnownPlugins()
 */
class VSTPluginManager
{
public:
    VSTPluginManager();
    ~VSTPluginManager();

    // ============================================================
    // Initialization
    // ============================================================

    /** Initialize the plugin manager. Must be called before any plugins can be loaded. */
    void initialize();

    /** Check if the manager has been initialized. */
    bool isInitialized() const { return initialized; }

    // ============================================================
    // Plugin Format Management
    // ============================================================

    /** Get the AudioPluginFormatManager for direct access. */
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    const juce::AudioPluginFormatManager& getFormatManager() const { return formatManager; }

    /** Get list of available plugin formats (VST3, AU, etc.) */
    juce::StringArray getAvailableFormats() const;

    /** Get a specific format by name (e.g., "VST3") */
    juce::AudioPluginFormat* getFormat(const juce::String& formatName) const;

    // ============================================================
    // Plugin Loading
    // ============================================================

    /**
     * Load a plugin from a file path.
     * @param pluginPath Path to the plugin file (.vst3, .component, etc.)
     * @param callback Called when loading completes (success or failure)
     */
    void loadPlugin(const juce::File& pluginPath,
                    std::function<void(std::unique_ptr<PluginInstance>)> callback);

    /**
     * Load a plugin from a PluginDescription.
     * @param desc The plugin description (from scan results)
     * @param callback Called when loading completes
     */
    void loadPlugin(const juce::PluginDescription& desc,
                    std::function<void(std::unique_ptr<PluginInstance>)> callback);

    /**
     * Load a plugin synchronously (blocks until loaded).
     * @param desc The plugin description
     * @return PluginInstance or nullptr on failure
     */
    std::unique_ptr<PluginInstance> loadPluginSync(const juce::PluginDescription& desc);

    /**
     * Load a plugin by identifier and format name (for state restoration).
     * This is a synchronous version that blocks until loading completes.
     * @param fileOrIdentifier Plugin path or identifier
     * @param formatName Plugin format name (e.g., "VST3")
     * @return AudioPluginInstance or nullptr on failure
     */
    std::unique_ptr<juce::AudioPluginInstance> loadPluginForState(
        const juce::String& fileOrIdentifier,
        const juce::String& formatName);

    // ============================================================
    // AudioProcessorGraph (Plugin Routing)
    // ============================================================

    /** Get the AudioProcessorGraph for the master plugin chain. */
    juce::AudioProcessorGraph& getPluginGraph() { return *pluginGraph; }
    const juce::AudioProcessorGraph& getPluginGraph() const { return *pluginGraph; }

    /** Add a plugin instance to the graph. Returns the node ID. */
    juce::AudioProcessorGraph::NodeID addPluginToGraph(std::unique_ptr<PluginInstance> instance);

    /** Remove a plugin from the graph by node ID. */
    void removePluginFromGraph(juce::AudioProcessorGraph::NodeID nodeId);

    /** Get a plugin node by ID. */
    juce::AudioProcessorGraph::Node::Ptr getPluginNode(juce::AudioProcessorGraph::NodeID nodeId) const;

    /** Clear all plugins from the graph. */
    void clearGraph();

    // ============================================================
    // Plugin Scanning
    // ============================================================

    /** Get the plugin scanner. */
    PluginScanner& getScanner() { return *scanner; }
    const PluginScanner& getScanner() const { return *scanner; }

    /** Start a background scan for plugins. */
    void scanForPlugins();

    /** Start a background scan for plugins in a custom path. */
    void scanForPlugins(const juce::FileSearchPath& searchPath);

    /** Check if a scan is currently running. */
    bool isScanning() const;

    /** Get scan progress (0.0 to 1.0). */
    float getScanProgress() const;

    // ============================================================
    // Known Plugin List Management
    // ============================================================

    /** Get all known plugin descriptions (from previous scans). */
    juce::KnownPluginList& getKnownPlugins() { return knownPlugins; }
    const juce::KnownPluginList& getKnownPlugins() const { return knownPlugins; }

    /** Search known plugins by name (for UI filtering). */
    juce::Array<juce::PluginDescription> searchPlugins(const juce::String& searchTerm) const;

    /** Get plugins by manufacturer. */
    juce::Array<juce::PluginDescription> getPluginsByManufacturer(const juce::String& manufacturer) const;

    /** Get plugins by category. */
    juce::Array<juce::PluginDescription> getPluginsByCategory(const juce::String& category) const;

    // ============================================================
    // Persistence (Save/Load Plugin List)
    // ============================================================

    /** Get the file where the plugin list is stored. */
    juce::File getPluginListFile() const;

    /** Get the directory where app data is stored. */
    juce::File getAppDataDirectory() const;

    /** Save the known plugin list to disk (XML). */
    bool savePluginList();

    /** Load the known plugin list from disk (XML). */
    bool loadPluginList();

    /** Check if a cached plugin list exists. */
    bool hasCachedPluginList() const;

    /** Clear the plugin list cache (forces rescan). */
    void clearPluginListCache();

    /** Remove a plugin from the blacklist. */
    void removeFromBlacklist(const juce::String& fileOrIdentifier);

    /** Get the blacklist. */
    juce::StringArray getBlacklist() const;

    // ============================================================
    // Audio Processing
    // ============================================================

    /** Prepare the plugin graph for playback. */
    void prepareToPlay(double sampleRate, int blockSize);

    /** Release resources. */
    void releaseResources();

    /** Process audio through the plugin graph. */
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // ============================================================
    // Callbacks for UI Updates
    // ============================================================

    /** Set callback for when scan progress updates. */
    void setScanProgressCallback(std::function<void(float)> callback);

    /** Set callback for when scan completes. */
    void setScanCompleteCallback(std::function<void(int numPluginsFound)> callback);

    /** Set callback for when plugin list changes. */
    void setPluginListChangedCallback(std::function<void()> callback);

private:
    bool initialized = false;

    // Plugin format manager (handles VST3, AU, etc.)
    juce::AudioPluginFormatManager formatManager;

    // Plugin routing graph
    std::unique_ptr<juce::AudioProcessorGraph> pluginGraph;

    // Known plugins from scanning
    juce::KnownPluginList knownPlugins;

    // Plugin scanner (background thread)
    std::unique_ptr<PluginScanner> scanner;

    // Audio settings
    double currentSampleRate = PluginHostConfig::defaultSampleRate;
    int currentBlockSize = PluginHostConfig::defaultBlockSize;

    // Callbacks
    std::function<void()> pluginListChangedCallback;

    /** Ensure the AppData directory exists. */
    bool ensureAppDataDirectoryExists();

    /** Called when scanner finds new plugins. */
    void onScanComplete(int numPluginsFound);

    /** Configure LV2 search paths (Phase 4.4) */
    void configureLV2SearchPaths(juce::AudioPluginFormat* lv2Format);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VSTPluginManager)
};
