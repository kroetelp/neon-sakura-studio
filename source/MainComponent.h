#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <memory>

#include "WavetableUI/NeonSakuraLookAndFeel.h"
#include "Theme/ThemeManager.h"
#include "UI/TransportBar.h"
#include "UI/GlobalControlsBar.h"
#include "UI/TrackToolsBar.h"
#include "UI/PanelTogglesBar.h"
#include "WootingManager.h"
#include "AudioRouting/AudioRoutingGraph.h"  // For LoadedPlugin struct

// Forward declarations for PluginLoadingCoordinator
class PluginLoadingCoordinator;

// Forward declarations
class TrackManager;
class SampleManager;
class PlaybackController;
class PanelManager;
class DockingManager;
class AudioEngine;
class PatternGenerator;
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
                      public juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

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
    std::unique_ptr<PanelManager> panelManager;
    std::unique_ptr<DockingManager> dockingManager;
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
    // STRETCHABLE LAYOUT (für vertikales Resizing)
    // ========================================================================
    juce::StretchableLayoutManager stretchableManager;
    std::unique_ptr<juce::StretchableLayoutResizerBar> verticalResizerBar;

    // Layout-Item-IDs für StretchableLayoutManager
    static constexpr int synthLayoutId = 1;
    static constexpr int resizerLayoutId = 2;
    static constexpr int bottomTabsLayoutId = 3;
    static constexpr int timelineLayoutId = 4;

    // ========================================================================
    // TAB COMPONENT (Timeline + Step Sequencer)
    // ========================================================================
    std::unique_ptr<juce::TabbedComponent> bottomTabs;

    // Tab-Indizes
    static constexpr int timelineTabIndex = 0;
    static constexpr int stepSequencerTabIndex = 1;

    // ========================================================================
    // GUI COMPONENTS - TOP BAR (New Modular Design)
    // ========================================================================
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<GlobalControlsBar> globalControlsBar;
    std::unique_ptr<TrackToolsBar> trackToolsBar;
    std::unique_ptr<PanelTogglesBar> panelTogglesBar;

    // Additional Top Bar Controls (kept for compatibility)
    juce::TextButton clearAllButton;
    juce::TextButton audioSettingsButton;
    juce::TextButton wootingSettingsButton;

    // Legacy Controls (will be phased out)
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton setFolderButton;
    juce::TextButton rhythmExplorerButton;
    juce::TextButton melodyWorkstationButton;
    juce::TextButton wavetableSynthButton;
    juce::TextButton timelineButton;
    juce::TextButton stepSequencerButton;
    juce::TextButton pluginBrowserButton;
    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    juce::Slider masterVolumeSlider;
    juce::Label masterVolumeLabel;
    juce::ComboBox loopLengthComboBox;
    juce::Label loopLengthLabel;
    juce::Label folderLabel;
    juce::ComboBox genreComboBox;
    juce::ComboBox drumTargetTrackCombo;
    juce::Label drumTargetLabel;
    juce::TextButton generateButton;
    juce::Slider swingSlider;
    juce::Label swingLabel;
    juce::Slider reverbSlider;
    juce::Label reverbLabel;

    // ========================================================================
    // LAYOUT CONSTANTS
    // ========================================================================
    static constexpr int topBarHeight = 90;
    static constexpr int resizerBarHeight = 6;
    static constexpr int defaultSynthHeight = 350;
    static constexpr int defaultBottomTabsHeight = 400;
    static constexpr int minPanelHeight = 100;
    static constexpr int tabBarHeight = 32;

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
    void initializeDockingPanels();
    void initializeBottomTabs();  // NEU: Tab-Component initialisieren
    void connectTrackCallbacks();
    void connectUICallbacks();

    void updatePlayhead();
    void togglePlay();
    void stopPlayback();
    void openFolderChooser();
    void showAudioSettingsDialog();

    // === Layout Helper ===
    void layoutTopBar(juce::Rectangle<int>& area);
    void layoutTracks(juce::Rectangle<int> area);

    // === Tab Helper ===
    void switchToTimelineTab();
    void switchToStepSequencerTab();

    // === VST Hosting Helper ===
    void initializeVSTHosting();
    void connectVSTCallbacks();
    void togglePluginBrowser();
    void loadPluginToSelectedTrack(std::unique_ptr<PluginInstance> instance);
    juce::Rectangle<int> layoutPluginBrowser(juce::Rectangle<int>& mainArea);

    // === Color Helpers (delegated to ThemeManager) ===
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getNeonPurple() const { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    juce::Colour getNeonGreen() const { return ThemeManager::getInstance().getSuccessColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getBackgroundColor(); }
    juce::Colour getStepInactive() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
