#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

class VSTPluginManager;

/**
 * PluginScanner - Professional background scanner for VST3/AU plugins
 *
 * Features:
 *   - Asynchronous scanning in background thread
 *   - Progress tracking with callbacks
 *   - Platform-specific default search paths
 *   - Cancellation support
 *   - Plugin blacklist management (crashed/problematic plugins)
 *
 * Usage:
 *   scanner.startScan();  // Uses default paths
 *   scanner.setCompletionCallback([](int numFound) { ... });
 *
 * Architecture:
 *
 *   Main Thread                     Background Thread
 *   ───────────                     ─────────────────
 *   startScan() ──────────────────► runScan()
 *       │                                │
 *       │                                ├─► scanDirectory()
 *       │                                │      │
 *       │                                │      └─► findAllTypesForFile()
 *       │                                │
 *       │   ◄────────────────────────────┼─ progress updates (atomic)
 *       │                                │
 *       │   ◄────────────────────────────┴─ completion callback
 *       │
 *   timerCallback() ──► UI updates
 */
class PluginScanner : private juce::Timer
{
public:
    explicit PluginScanner(VSTPluginManager& owner);
    ~PluginScanner() override;

    // ============================================================
    // Scanning Control
    // ============================================================

    /** Start a background scan using default system paths. */
    void startScan();

    /** Start a scan for specific directories only. */
    void startScan(const juce::FileSearchPath& searchPath);

    /** Cancel any ongoing scan (blocks until scan thread stops). */
    void cancelScan();

    /** Check if a scan is currently running. */
    bool isScanning() const { return scanning.load(); }

    // ============================================================
    // Progress Tracking
    // ============================================================

    /** Get the scan progress (0.0 to 1.0). */
    float getProgress() const { return progress.load(); }

    /** Get the current file being scanned. */
    juce::String getCurrentFile() const;

    /** Get the number of plugins found so far. */
    int getPluginsFound() const { return pluginsFound.load(); }

    /** Get the number of files scanned so far. */
    int getFilesScanned() const { return filesScanned.load(); }

    /** Get the total number of files to scan. */
    int getTotalFiles() const { return totalFilesToScan.load(); }

    // ============================================================
    // Callbacks
    // ============================================================

    /** Set a callback to receive progress updates (called on main thread). */
    void setProgressCallback(std::function<void(float)> callback) { progressCallback = callback; }

    /** Set a callback for when a plugin is found. */
    void setPluginFoundCallback(std::function<void(const juce::String& name)> callback) { pluginFoundCallback = callback; }

    /** Set a callback to receive scan completion notification. */
    void setCompletionCallback(std::function<void(int numPluginsFound)> callback) { completionCallback = callback; }

    // ============================================================
    // Search Paths
    // ============================================================

    /** Get the list of directories that will be scanned by default. */
    juce::FileSearchPath getDefaultSearchPath() const;

    /** Get the list of directories that were actually scanned. */
    juce::FileSearchPath getLastSearchPath() const { return lastSearchPath; }

    // ============================================================
    // Plugin Validation
    // ============================================================

    /** Check if a plugin is blacklisted (known to crash). */
    bool isBlacklisted(const juce::String& fileOrIdentifier) const;

    /** Manually add a plugin to the blacklist. */
    void addToBlacklist(const juce::String& fileOrIdentifier);

private:
    // ============================================================
    // Timer Callback (Main Thread)
    // ============================================================

    void timerCallback() override;

    // ============================================================
    // Background Scanning (Worker Thread)
    // ============================================================

    void runScan(const juce::FileSearchPath& searchPath);
    void scanForFormat(juce::AudioPluginFormat* format, const juce::FileSearchPath& searchPath);
    void scanFile(const juce::File& file, juce::AudioPluginFormat* format);

    /** Count total plugin files to scan for progress calculation. */
    int countPluginFiles(const juce::FileSearchPath& searchPath);

    // ============================================================
    // Thread-Safe State Updates
    // ============================================================

    void updateProgress(float newProgress);
    void setCurrentFile(const juce::String& filename);
    void incrementPluginsFound();
    void incrementFilesScanned();

    // ============================================================
    // Member Variables
    // ============================================================

    VSTPluginManager& owner;

    // Thread control
    std::atomic<bool> scanning{false};
    std::atomic<bool> cancelRequested{false};
    std::thread scanThread;
    std::mutex scanMutex;

    // Progress tracking (atomic for thread safety)
    std::atomic<float> progress{0.0f};
    std::atomic<int> pluginsFound{0};
    std::atomic<int> filesScanned{0};
    std::atomic<int> totalFilesToScan{0};

    // Current file being scanned (protected by mutex)
    mutable std::mutex currentFileMutex;
    juce::String currentFileScanning;

    // Search paths
    juce::FileSearchPath lastSearchPath;

    // Temporary storage for scan results
    juce::KnownPluginList tempPluginList;
    juce::StringArray tempBlacklist;

    // Callbacks (called on main thread via timer)
    std::function<void(float)> progressCallback;
    std::function<void(const juce::String&)> pluginFoundCallback;
    std::function<void(int)> completionCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginScanner)
};
