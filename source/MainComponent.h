#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <unordered_map>
#include <functional>
#include <random>
#include <chrono>
#include <atomic>
#include <memory>

#include "WavetableUI/NeonSakuraLookAndFeel.h"
#include "Theme/ThemeManager.h"
#include "UI/TransportBar.h"
#include "UI/GlobalControlsBar.h"
#include "UI/TrackToolsBar.h"
#include "WootingManager.h"
#include "AudioRouting/AudioRoutingGraph.h"  // For LoadedPlugin struct

// Forward declarations for PluginLoadingCoordinator
class PluginLoadingCoordinator;

// Forward declarations
class TrackManager;
class SampleManager;
class PlaybackController;
class AudioEngine;
class PatternGenerator;
class WavetablePanel;
class RhythmExplorerPanel;
class MelodyPanelPanel;
class WavetableParams;
class TimelineComponent;
class VSTPluginManager;
class PluginWindowManager;
class PluginBrowserComponent;
class PluginInstance;
class PluginLoadingCoordinator;
// class PluginSandboxManager;  // TODO: Phase 6.2 - Disabled for now
class CPUProfiler;

/**
 * MainComponent - Das Hauptfenster der DAW (Single-Window Workspace)
 *
 * Layout-Struktur:
 * ┌─────────────────────────────────────────────────────────┐
 * │ TOP BAR (Transport Controls) - 90px                     │
 * ├─────────────────────────────────────────────────────────┤
 * │                                                         │
 * │                 WAVETABLE SYNTH                         │
 * │                 (vertikal resizebar)                    │
 * │                                                         │
 * ├─────────────────────────────────────────────────────────┤
 * │ ════════════════ StretchableBar ═══════════════════════ │
 * ├─────────────────────────────────────────────────────────┤
 * │  [Timeline] [Step Sequencer]  ← TABS                    │
 * │ ─────────────────────────────────────────────────────── │
 * │                                                         │
 * │              TAB CONTENT (Timeline/Sequencer)           │
 * │                                                         │
 * └─────────────────────────────────────────────────────────┘
 */
class MainComponent : public juce::AudioAppComponent,
                      public juce::Timer,
                      public juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void timerCallback() override;

    // === MenuBarModel Interface ===
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    // === DockingManager Integration ===
    void triggerLayoutUpdate() { resized(); }

    // === VST Plugin Hosting ===
    VSTPluginManager& getVSTPluginManager() { return *vstPluginManager; }
    PluginWindowManager& getPluginWindowManager() { return *pluginWindowManager; }
    PluginLoadingCoordinator& getPluginLoadingCoordinator() { return *pluginLoadingCoordinator; }
    // PluginSandboxManager& getPluginSandboxManager() { return *pluginSandboxManager; }  // TODO: Phase 6.2
    CPUProfiler& getCPUProfiler() { return *cpuProfiler; }

    /** Get the currently selected track index for plugin loading. */
    int getSelectedTrackIndex() const { return selectedTrackIndex; }

    /** Set the selected track index. */
    void setSelectedTrackIndex(int index) { selectedTrackIndex = index; }

private:
    static constexpr int numTracks = 8;

    NeonSakuraLookAndFeel customLookAndFeel;

    // ========================================================================
    // MANAGERS AND CONTROLLERS
    // ========================================================================
    juce::AudioFormatManager formatManager;
    std::unique_ptr<TrackManager> trackManager;
    std::unique_ptr<SampleManager> sampleManager;
    std::unique_ptr<PlaybackController> playbackController;
        std::unique_ptr<AudioEngine> audioEngine;
    std::unique_ptr<PatternGenerator> patternGenerator;
    std::unique_ptr<WootingManager> wootingManager;

    // ========================================================================
    // VST HOSTING (External Plugin Support)
    // ========================================================================
    std::unique_ptr<VSTPluginManager> vstPluginManager;
    std::unique_ptr<PluginWindowManager> pluginWindowManager;
    std::unique_ptr<PluginBrowserComponent> pluginBrowserComponent;

    // Phase 6.1: Lock-Free Plugin Loading Coordinator
    std::unique_ptr<PluginLoadingCoordinator> pluginLoadingCoordinator;

    // Phase 6.2: Plugin Sandbox Manager (Optional) - Disabled for now
    // std::unique_ptr<PluginSandboxManager> pluginSandboxManager;

    // Phase 6.3: CPU Profiler
    std::unique_ptr<CPUProfiler> cpuProfiler;

    // Track loaded plugins for window management and state access
    std::vector<LoadedPlugin> loadedPlugins;

    // ========================================================================
    // FLOATING WINDOW MANAGEMENT
    // ========================================================================
    std::unordered_map<int, std::unique_ptr<juce::DocumentWindow>> floatingWindows;

    // Panel storage for floating windows
    std::unique_ptr<WavetablePanel> wavetablePanelForWindow;
    std::unique_ptr<RhythmExplorerPanel> rhythmExplorerPanelForWindow;
    std::unique_ptr<MelodyPanelPanel> melodyPanelPanelForWindow;

    // ========================================================================
    // MAIN COMPONENT EXTENSION (Floating Workspace Layout)
    // ========================================================================
    
    // ========================================================================
    // TAB COMPONENT (Timeline + Step Sequencer)
    // ========================================================================
    std::unique_ptr<juce::TabbedComponent> workspaceTabs;

    // Tab-Indizes
    static constexpr int timelineTabIndex = 0;
    static constexpr int stepSequencerTabIndex = 1;

    // ========================================================================
    // GUI COMPONENTS - TOP BAR (New Modular Design)
    // ========================================================================
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<GlobalControlsBar> globalControlsBar;
    std::unique_ptr<TrackToolsBar> trackToolsBar;
    std::unique_ptr<juce::MenuBarComponent> menuBar;

    // ========================================================================
    // MENU BAR IDs
    // ========================================================================
    static constexpr int menuWavetableSynth = 1001;
    static constexpr int menuRhythmExplorer = 1002;
    static constexpr int menuMelodyPanel = 1003;
    static constexpr int menuPluginBrowser = 1004;

    // ========================================================================
    // LAYOUT CONSTANTS
    // ========================================================================
    static constexpr int menuBarHeight = 24;
    static constexpr int controlsRowHeight = 45;

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================
    int selectedTrackForRhythm = 0;
    int selectedTrackIndex = 0;  // Aktuell ausgewählter Track für Plugin-Loading
    std::unique_ptr<juce::FileChooser> chooser;
    bool isResizing = false;
    bool pluginBrowserVisible = false;  // State für Plugin Browser Sidebar
    static constexpr int pluginBrowserWidth = 280;  // Breite der Plugin Browser Sidebar

    // ========================================================================
    // HELPER METHODS
    // ========================================================================
    void initializeManagers();
    void initializeUI();
    void connectTrackCallbacks();

    void updatePlayhead();
    void openFolderChooser();
    void showAudioSettingsDialog();

    // === VST Hosting Helper ===
    void initializeVSTHosting();
    void connectVSTCallbacks();
    void loadPluginToSelectedTrack(std::unique_ptr<PluginInstance> instance);

    // === Floating Window Management ===
    void openWavetableSynthWindow();
    void openRhythmExplorerWindow();
    void openMelodyPanelWindow();
    void openPluginBrowserWindow();
    void closeFloatingWindow(int windowID);

    // === Color Helpers (delegated to ThemeManager) ===
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getNeonPurple() const { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    juce::Colour getNeonGreen() const { return ThemeManager::getInstance().getSuccessColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getBackgroundColor(); }
    juce::Colour getStepInactive() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
