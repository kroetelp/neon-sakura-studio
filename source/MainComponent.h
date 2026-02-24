#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "TrackComponent.h"

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
    static constexpr int numTracks = 4;

    // Audio format management
    juce::AudioFormatManager formatManager;
    double currentSampleRate = 44100.0;

    // GUI Components - Top controls
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton setFolderButton;
    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    juce::ComboBox loopLengthComboBox;
    juce::Label loopLengthLabel;
    juce::Label folderLabel;

    // Track components
    std::array<std::unique_ptr<TrackComponent>, numTracks> tracks;

    // Sample directory
    juce::File sampleDirectory;
    juce::StringArray sampleCategories;
    juce::CriticalSection directoryLock;

    // File chooser for folder selection
    std::unique_ptr<juce::FileChooser> chooser;

    // Playback state
    bool isPlaying = false;
    int lastPlayedStep = -1;
    int globalLoopCounter = 0;  // For Slow (/) modifier
    std::atomic<int> loopLength{16};

    // Timing (thread-safe)
    std::atomic<double> bpm{120.0};

    // Audio thread timing - SAMPLE ACCURATE
    std::atomic<uint64_t> samplePosition{0};
    std::atomic<int> samplesPerStep{0};

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
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    // Helper methods
    void updatePlayhead();
    void togglePlay();
    void stopPlayback();
    void calculateSamplesPerStep();
    void autoDetectSampleDirectory();
    void openFolderChooser();
    void scanSampleDirectory(const juce::File& directory);
    void loadPendingSamples();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
