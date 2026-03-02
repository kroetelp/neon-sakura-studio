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
#include "WootingManager.h"

// Forward declarations
class TrackManager;
class SampleManager;
class PlaybackController;
class PanelManager;
class DockingManager;
class AudioEngine;
class PatternGenerator;
class RhythmExplorer;
class MelodyPanel;
class WavetableParams;
class TimelineComponent;

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
    // GUI COMPONENTS - TOP BAR
    // ========================================================================
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton setFolderButton;
    juce::TextButton clearAllButton;
    juce::TextButton audioSettingsButton;
    juce::TextButton wootingSettingsButton;  // NEU: Wooting Keyboard Settings
    juce::TextButton rhythmExplorerButton;
    juce::TextButton melodyWorkstationButton;
    juce::TextButton wavetableSynthButton;
    juce::TextButton timelineButton;        // NEU: Button für Timeline Tab
    juce::TextButton stepSequencerButton;  // NEU: Button für Step Sequencer Tab

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
    std::unique_ptr<juce::FileChooser> chooser;
    bool isResizing = false;

    // ========================================================================
    // HELPER METHODS
    // ========================================================================
    void initializeManagers();
    void initializeUI();
    void initializeDockingPanels();
    void initializeBottomTabs();  // NEU: Tab-Component initialisieren
    void connectTrackCallbacks();
    void connectPanelCallbacks();
    void connectUICallbacks();

    void updatePlayhead();
    void togglePlay();
    void stopPlayback();
    void openFolderChooser();
    void showAudioSettingsDialog();

    // === Layout Helper ===
    void layoutTopBar(juce::Rectangle<int>& area);
    void layoutTracks(juce::Rectangle<int> area);
    void layoutOldSidebarPanels(const juce::Rectangle<int>& rhythmArea,
                                  const juce::Rectangle<int>& melodyArea);

    // === Tab Helper ===
    void switchToTimelineTab();
    void switchToStepSequencerTab();

    // === Color Helpers ===
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getNeonGreen() const { return juce::Colour(0, 255, 127); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
