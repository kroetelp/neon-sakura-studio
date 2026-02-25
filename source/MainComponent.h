#pragma once

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
#include "TrackComponent.h"
#include "PatternGenerator.h"
#include "AudioEngine.h"
#include "RhythmExplorer.h"
#include "MelodyPanel.h"
#include "WavetableUI/WavetableSynthEditor.h"

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

    // Audio format management (kept for TrackComponent creation)
    juce::AudioFormatManager formatManager;

    // GUI Components - Top controls
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton setFolderButton;
    juce::TextButton clearAllButton;
    juce::TextButton audioSettingsButton;  // Audio device selection
    juce::TextButton rhythmExplorerButton;  // Toggle for RhythmExplorer panel
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

    // Rhythm Explorer panel
    std::unique_ptr<RhythmExplorer> rhythmExplorer;
    bool rhythmExplorerVisible = false;
    int selectedTrackForRhythm = 0;

    // Melody Workstation panel
    std::unique_ptr<MelodyPanel> melodyPanel;
    bool melodyPanelVisible = false;
    juce::TextButton melodyWorkstationButton;  // Toggle for MelodyPanel

    // Wavetable Synth (separate window)
    std::unique_ptr<WavetableSynthEditor> wavetableSynthEditor;
    std::unique_ptr<juce::DocumentWindow> wavetableSynthWindow;
    bool wavetableSynthVisible = false;
    juce::TextButton wavetableSynthButton;

    // Track Wavetable Editor (for editing track synth params)
    std::unique_ptr<WavetableSynthEditor> trackWavetableEditor;
    std::unique_ptr<juce::DocumentWindow> trackWavetableWindow;
    int currentEditingTrack = -1;

    // Track components
    std::array<std::unique_ptr<TrackComponent>, numTracks> tracks;

    // Audio Engine (handles all audio processing)
    std::unique_ptr<AudioEngine> audioEngine;

    // Sample directory
    juce::File sampleDirectory;
    juce::StringArray sampleCategories;
    juce::CriticalSection directoryLock;

    // File chooser for folder selection
    std::unique_ptr<juce::FileChooser> chooser;

    // UI state
    bool isResizing = false;  // Flag to prevent updates during resize

    // BPM atomic for PatternGenerator (synced with AudioEngine)
    std::atomic<double> bpm{120.0};

    // Pending sample loads
    struct PendingSampleLoad
    {
        int trackIndex;
        juce::String category;
    };
    std::vector<PendingSampleLoad> pendingSampleLoads;
    juce::CriticalSection pendingLoadLock;

    // Colors
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    // Helper methods
    void updatePlayhead();
    void togglePlay();
    void stopPlayback();
    void autoDetectSampleDirectory();
    void openFolderChooser();
    void scanSampleDirectory(const juce::File& directory);
    void loadPendingSamples();
    void showAudioSettingsDialog();  // Opens audio device selection dialog
    void openTrackWavetableEditor(int trackIndex, std::shared_ptr<WavetableParams> params);

    // Pattern generator
    std::unique_ptr<PatternGenerator> patternGenerator;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
