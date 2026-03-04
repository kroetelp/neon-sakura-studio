#pragma once

/**
 * PluginWindow - Floating window that hosts an external plugin's GUI editor
 *
 * This window wraps native plugin editor (VST3/AU) and provides:
 *   - Draggable, resizable floating window
 *   - Close button with callback
 *   - Proper lifecycle management (editor deletion)
 *   - Bypass toggle
 *
 * Usage:
 *   auto* window = new PluginWindow(pluginInstance);
 *   window->setVisible(true);
 *
 * Architecture:
 *
 *   PluginWindow (DocumentWindow)
 *       └── PluginEditorHolder (content component)
 *               └── AudioProcessorEditor (plugin's native GUI)
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <functional>

// Forward declaration
class PluginInstance;

/**
 * PluginWindow - Floating window that hosts an external plugin's GUI editor
 *
 * This window wraps native plugin editor (VST3/AU) and provides:
 *   - Draggable, resizable floating window
 *   - Close button with callback
 *   - Proper lifecycle management (editor deletion)
 *   - Bypass toggle
 */
class PluginWindow : public juce::DocumentWindow
{
public:
    /**
     * Create a plugin window for the given plugin instance.
     * @param plugin The plugin instance to display
     * @param onCloseCallback Called when window is closed
     */
    PluginWindow(PluginInstance& plugin,
                 std::function<void()> onCloseCallback = {});

    ~PluginWindow() override;

    // ============================================================
    // DocumentWindow overrides
    // ============================================================

    void closeButtonPressed() override;
    void resized() override;
    void moved() override;

    // ============================================================
    // Plugin Access
    // ============================================================

    /** Get the plugin instance this window is hosting. */
    PluginInstance& getPluginInstance() { return pluginInstance; }

    /** Get the plugin instance this window is hosting (const version). */
    const PluginInstance& getPluginInstance() const { return pluginInstance; }

    /** Get plugin name. */
    juce::String getPluginName() const;

    // ============================================================
    // Window State
    // ============================================================

    /** Check if window is still valid (plugin not deleted). */
    bool isValid() const { return valid; }

    /** Mark the window as invalid (plugin was deleted). */
    void invalidate();

    /** Bring window to front. */
    void bringToFront();

    // ============================================================
    // Bypass Control
    // ============================================================

    /** Toggle plugin bypass. */
    void toggleBypass();

    /** Check if plugin is bypassed. */
    bool isBypassed() const;

    /** Update size constraints based on plugin editor. */
    void updateSizeConstraints();

private:
    class PluginEditorHolder;

    PluginInstance& pluginInstance;
    std::function<void()> onCloseCallback;
    bool valid = true;
    juce::Rectangle<int> lastBounds;
    std::unique_ptr<PluginEditorHolder> editorHolder;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};

/**
 * PluginWindowManager - Manages all open plugin windows
 *
 * Responsibilities:
 *   - Track all open plugin windows
 *   - Close windows when plugins are removed
 *   - Prevent duplicate windows for same plugin
 *   - Manage window z-order (always-on-top option)
 */
class PluginWindowManager : private juce::Timer
{
public:
    PluginWindowManager() = default;
    ~PluginWindowManager();

    // ============================================================
    // Window Management
    // ============================================================

    /**
     * Open a plugin window for the given plugin.
     * If a window already exists for this plugin, brings it to front.
     * @param plugin The plugin instance to show
     * @return The plugin window (existing or newly created)
     */
    PluginWindow* openWindowForPlugin(PluginInstance& plugin);

    /**
     * Close a window for a specific plugin.
     * @param plugin The plugin whose window should be closed
     */
    void closeWindowForPlugin(PluginInstance& plugin);

    /** Close all plugin windows. */
    void closeAllWindows();

    /**
     * Check if a window is open for the given plugin.
     * @param plugin The plugin to check
     * @return true if window is open
     */
    bool hasWindowForPlugin(PluginInstance& plugin) const;

    /**
     * Get window for a specific plugin (nullptr if not open).
     * @param plugin The plugin instance
     * @return The plugin window (existing or nullptr)
     */
    PluginWindow* getWindowForPlugin(PluginInstance& plugin) const;

    /**
     * Get all open windows.
     * @return Array of all open windows
     */
    juce::Array<PluginWindow*> getAllWindows() const;

    /**
     * Get number of open windows.
     * @return Number of open windows
     */
    int getNumWindows() const { return windows.size(); }

    // ============================================================
    // Callbacks
    // ============================================================

    /**
     * Set callback for when any window is closed.
     * @param callback Function to call when a window is closed
     */
    void setOnWindowClosed(std::function<void(PluginWindow*)> callback)
    {
        onWindowClosed = callback;
    }

private:
    void timerCallback() override;

    juce::OwnedArray<PluginWindow> windows;
    std::function<void(PluginWindow*)> onWindowClosed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindowManager)
};
