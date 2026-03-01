#pragma once

#include "TimelineClip.h"
#include "TimelineTrack.h"
#include "TimelineData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <memory>

class RecordingManager
{
public:
    RecordingManager(TimelineData& data);
    ~RecordingManager() = default;

    // Recording state
    enum class RecordingState { Idle, Recording, Stopping };
    enum class RecordingSource { AudioInput, WavetableSynth };

    RecordingState getState() const { return state.load(); }
    bool isRecording() const { return state.load() == RecordingState::Recording; }

    // Start/Stop recording
    void startRecording(int trackIndex, RecordingSource source);
    std::unique_ptr<TimelineClip> stopRecording();

    // Audio thread callback - call this from AudioEngine
    void processRecording(const juce::AudioBuffer<float>& inputBuffer,
                          const juce::AudioBuffer<float>& synthBuffer,
                          const juce::MidiBuffer& midiMessages,
                          double sampleRate,
                          double currentBeat);

    // Get current recording info
    int getRecordingTrackIndex() const { return recordingTrackIndex.load(); }
    double getRecordingStartBeat() const { return recordingStartBeat.load(); }
    RecordingSource getRecordingSource() const { return currentSource.load(); }

    // Metronome
    void setMetronomeEnabled(bool enabled) { metronomeEnabled.store(enabled); }
    bool isMetronomeEnabled() const { return metronomeEnabled.load(); }

private:
    TimelineData& timelineData;

    std::atomic<RecordingState> state{RecordingState::Idle};
    std::atomic<int> recordingTrackIndex{-1};
    std::atomic<double> recordingStartBeat{0.0};
    std::atomic<RecordingSource> currentSource{RecordingSource::AudioInput};

    // Recording buffers (only accessed from audio thread during recording)
    std::unique_ptr<juce::AudioBuffer<float>> recordingAudioBuffer;
    std::vector<TimelineClip::MidiNote> recordingMidiNotes;
    int64_t recordedSamples{0};
    double recordingSampleRate{44100.0};

    // Metronome
    std::atomic<bool> metronomeEnabled{true};
    double lastMetronomeBeat{0.0};
    int metronomeSampleCounter{0};

    void ensureBufferCapacity(int samplesNeeded);
    void generateMetronomeClick(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
};
