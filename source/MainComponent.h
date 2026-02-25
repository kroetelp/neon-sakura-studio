#pragma once

/**
 * MainComponent - Main application component (UI coordination only)
 *
 * After refactoring, MainComponent is no longer a "God Class".
 * It delegates responsibilities to specialized managers:
 * - TrackManager: Track array ownership and access
 * - SampleManager: Sample directory and category management
 * - PlaybackController: Playback state management
 * - PanelManager: Side panels and floating windows
 * - AudioEngine: Audio processing (uses ITrackDataProvider interface)
 * - PatternGenerator: Algorithmic pattern generation
 *
 * MainComponent now focuses on:
 * - Top-level UI layout
 * - UI event handlers (buttons, sliders)
 * - Connecting UI to controllers
 * - AudioAppComponent delegation
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <memory>

// Forward declarations
class TrackManager;
class SampleManager;
class PlaybackController;
class PanelManager;
class AudioEngine;
class PatternGenerator;
class RhythmExplorer;
class MelodyPanel;
class WavetableParams;

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

private:
    static constexpr int numTracks = 8;

    // ========================================================================
    // MANAGERS AND CONTROLLERS (Decoupled responsibilities)
    // ========================================================================

    // Audio format management (kept for TrackManager creation)
    juce::AudioFormatManager formatManager;

    // Track management (owns track array)
    std::unique_ptr<TrackManager> trackManager;

    // Sample management (directory, categories, loading)
    std::unique_ptr<SampleManager> sampleManager;

    // Playback state (BPM, swing, playing, etc.)
    std::unique_ptr<PlaybackController> playbackController;

    // Panel management (side panels, floating windows)
    std::unique_ptr<PanelManager> panelManager;

    // Audio processing
    std::unique_ptr<AudioEngine> audioEngine;

    // Pattern generation
    std::unique_ptr<PatternGenerator> patternGenerator;

    // ========================================================================
    // GUI COMPONENTS (Top controls)
    // ========================================================================

    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton setFolderButton;
    juce::TextButton clearAllButton;
    juce::TextButton audioSettingsButton;
    juce::TextButton rhythmExplorerButton;
    juce::TextButton melodyWorkstationButton;
    juce::TextButton wavetableSynthButton;

    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    juce::Slider masterVolumeSlider;
    juce::Label masterVolumeLabel;
    juce::ComboBox loopLengthComboBox;
    juce::Label loopLengthLabel;
    juce::Label folderLabel;
    juce::ComboBox genreComboBox;
    juce::TextButton generateButton;
    juce::Slider swingSlider;
    juce::Label swingLabel;
    juce::Slider reverbSlider;
    juce::Label reverbLabel;

    // Track selection state (for RhythmExplorer and MelodyPanel)
    int selectedTrackForRhythm = 0;

    // File chooser for folder selection
    std::unique_ptr<juce::FileChooser> chooser;

    // UI state
    bool isResizing = false;

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    void initializeManagers();
    void initializeUI();
    void connectTrackCallbacks();
    void connectPanelCallbacks();
    void connectUICallbacks();

    void updatePlayhead();
    void togglePlay();
    void stopPlayback();
    void openFolderChooser();
    void showAudioSettingsDialog();

    // Colors
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
