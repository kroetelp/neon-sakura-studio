#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <functional>

class VSTPluginManager;
class PluginInstance;
class PluginWindowManager;

/**
 * PluginBrowserComponent - UI Panel for browsing and loading VST/AU plugins
 *
 * Features:
 *   - TreeView grouped by Manufacturer or Format
 *   - Search/filter functionality
 *   - Scan progress indicator
 *   - Double-click to load plugin
 *   - Rescan button
 *   - Cyberpunk/Neon design matching Neon Sakura theme
 *
 * Layout:
 *   ┌─────────────────────────────────────────────┐
 *   │ [Search Box]              [Rescan] [Options]│  <- Header
 *   ├─────────────────────────────────────────────┤
 *   │ ▼ Native Instruments                       │  <- TreeView
 *   │     └─ Kontakt 7                           │     (grouped by
 *   │     └─ Massive X                           │      manufacturer)
 *   │ ▼ Xfer Records                             │
 *   │     └─ Serum                               │
 *   │     └─ OTT                                 │
 *   │ ...                                        │
 *   ├─────────────────────────────────────────────┤
 *   │ Scanning: 45% [████████░░░░░░░░]           │  <- Progress bar
 *   └─────────────────────────────────────────────┘
 */
class PluginBrowserComponent : public juce::Component,
                                private juce::Timer,
                                private juce::KeyListener
{
public:
    /**
     * Create a plugin browser component.
     * @param pluginManager The VST plugin manager to use
     * @param windowManager Optional window manager for opening plugin GUIs
     */
    PluginBrowserComponent(VSTPluginManager& pluginManager,
                           PluginWindowManager* windowManager = nullptr);
    ~PluginBrowserComponent() override;

    // ============================================================
    // Component overrides
    // ============================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    // ============================================================
    // Plugin Loading Callbacks
    // ============================================================

    /** Set callback for when a plugin is selected (single click). */
    void setOnPluginSelected(std::function<void(const juce::PluginDescription&)> callback)
    {
        onPluginSelected = callback;
    }

    /** Set callback for when a plugin is loaded (double click). */
    void setOnPluginLoaded(std::function<void(std::unique_ptr<PluginInstance>)> callback)
    {
        onPluginLoaded = callback;
    }

    /** Set the target track index for plugin loading. */
    void setTargetTrack(int trackIndex) { targetTrackIndex = trackIndex; }
    int getTargetTrack() const { return targetTrackIndex; }

    // ============================================================
    // View Options
    // ============================================================

    enum class GroupMode
    {
        ByManufacturer,  // Group plugins by manufacturer name
        ByFormat,        // Group by format (VST3, AU, etc.)
        ByCategory,      // Group by plugin category
        Flat             // No grouping, flat list
    };

    void setGroupMode(GroupMode mode);
    GroupMode getGroupMode() const { return currentGroupMode; }

    // ============================================================
    // Actions
    // ============================================================

    /** Refresh the plugin list from the manager. */
    void refreshList();

    /** Start a plugin scan. */
    void startScan();

    /** Cancel ongoing scan. */
    void cancelScan();

    /** Clear search filter. */
    void clearSearch();

    // ============================================================
    // State
    // ============================================================

    /** Check if currently scanning. */
    bool isScanning() const;

    /** Get scan progress (0.0 to 1.0). */
    float getScanProgress() const;

private:
    // ============================================================
    // Timer callback for progress updates
    // ============================================================

    void timerCallback() override;

    // ============================================================
    // KeyListener for keyboard shortcuts
    // ============================================================

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    // ============================================================
    // Internal Classes
    // ============================================================

    /** TreeViewItem for a plugin group (manufacturer, format, etc.) */
    class PluginGroupItem;
    /** TreeViewItem for an individual plugin */
    class PluginTreeItem;
    /** Custom search box with neon styling */
    class SearchBox;
    /** Custom progress bar with neon styling */
    class NeonProgressBar;

    // ============================================================
    // Helper Methods
    // ============================================================

    void setupUI();
    void setupCallbacks();
    void applyNeonStyle();
    void rebuildTree();
    void filterTree(const juce::String& searchTerm);
    void loadSelectedPlugin();
    void onPluginDoubleClicked(const juce::PluginDescription& desc);

    // Internal callbacks from tree items (must be public for access)
    void onPluginSelectedSafe(const juce::PluginDescription& desc);
    void onPluginDoubleClickedSafe(const juce::PluginDescription& desc);

    // ============================================================
    // Members
    // ============================================================

    VSTPluginManager& pluginManager;
    PluginWindowManager* windowManager;

    // UI Components
    std::unique_ptr<SearchBox> searchBox;
    std::unique_ptr<juce::TreeView> pluginTree;
    std::unique_ptr<juce::TextButton> rescanButton;
    std::unique_ptr<juce::TextButton> addFolderButton;
    std::unique_ptr<juce::TextButton> optionsButton;
    std::unique_ptr<NeonProgressBar> progressBar;
    std::unique_ptr<juce::Label> statusLabel;

    // State
    GroupMode currentGroupMode = GroupMode::ByManufacturer;
    int targetTrackIndex = 0;
    float lastScanProgress = 0.0f;

    // Callbacks
    std::function<void(const juce::PluginDescription&)> onPluginSelected;
    std::function<void(std::unique_ptr<PluginInstance>)> onPluginLoaded;

    // Cached plugin list
    juce::Array<juce::PluginDescription> cachedPlugins;

    // File chooser for custom folder selection
    std::unique_ptr<juce::FileChooser> folderChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserComponent)
};
