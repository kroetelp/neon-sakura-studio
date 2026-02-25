#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <atomic>

class TrackComponent;
struct StepModifierState;

class AudioEngine
{
public:
    static constexpr int numTracks = 8;

    AudioEngine(std::array<std::unique_ptr<TrackComponent>, numTracks>& tracksRef);

    // AudioAppComponent Delegation Methods
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    // Playback Control (called from UI thread)
    void startPlayback();
    void stopPlayback();
    void resetPlaybackPosition();
    bool isPlaying() const { return playing.load(); }

    // Timing Control (thread-safe)
    void setBPM(double newBpm);
    void setSwingAmount(float swing);
    void setLoopLength(int steps);

    // Master Volume (thread-safe)
    void setMasterVolume(float volume);

    // Reverb Control (thread-safe)
    void setReverbWetLevel(float wetLevel);

    // Per-track state reset (called from UI thread)
    void resetTrackStates();

    // Position getters for UI (thread-safe)
    uint64_t getSamplePosition() const { return samplePosition.load(); }
    int getSamplesPerStep() const { return samplesPerStep.load(); }

private:
    // Dependencies
    std::array<std::unique_ptr<TrackComponent>, numTracks>& tracks;

    // Audio Thread State
    std::atomic<bool> playing{false};
    std::atomic<double> bpm{120.0};
    std::atomic<float> swingAmount{0.0f};
    std::atomic<int> loopLength{16};
    std::atomic<int> samplesPerStep{0};
    std::atomic<uint64_t> samplePosition{0};
    std::atomic<float> masterVolume{0.8f};
    std::atomic<float> reverbWetLevel{0.3f};

    // Audio-thread only variables
    double currentSampleRate = 44100.0;
    int globalLoopCounter = 0;
    std::array<int, numTracks> trackLastStep = {-1, -1, -1, -1, -1, -1, -1, -1};
    std::array<int, numTracks> trackLastRatchet = {-1, -1, -1, -1, -1, -1, -1, -1};

    // Buffers
    std::array<std::unique_ptr<juce::AudioBuffer<float>>, numTracks> trackBuffers;

    // Effects
    juce::Reverb masterReverb;
    juce::Reverb::Parameters reverbParams;

    // Helper Methods
    void calculateSamplesPerStep();
    void generateMidiForBuffer(const juce::AudioSourceChannelInfo& bufferToFill,
                             std::array<juce::MidiBuffer, numTracks>& trackMidiBuffers);
    void renderAndMixTracks(const juce::AudioSourceChannelInfo& bufferToFill,
                         const std::array<juce::MidiBuffer, numTracks>& trackMidiBuffers);
    void applyMasterReverb(juce::AudioBuffer<float>& buffer);
};
