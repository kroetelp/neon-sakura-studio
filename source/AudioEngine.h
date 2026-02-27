#pragma once

/**
 * AudioEngine - Handles all audio processing for the step sequencer
 *
 * This class is decoupled from MainComponent through interfaces:
 * - ITrackDataProvider: Abstracts track data access
 * - PlaybackController: Manages playback state (optional, can be null)
 *
 * Thread Safety:
 * - All public methods are thread-safe
 * - Audio processing happens on the audio thread
 * - UI updates use atomic variables for communication
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <atomic>
#include <random>
#include "ITrackDataProvider.h"
#include "WavetableSynth/WavetableEngine.h"

class PlaybackController;

class AudioEngine
{
public:
    static constexpr int numTracks = 8;

    /**
     * Constructor with ITrackDataProvider interface
     * @param trackProvider Interface for accessing track data (dependency injection)
     */
    explicit AudioEngine(ITrackDataProvider* trackProvider);

    /**
     * Constructor with optional PlaybackController
     * If PlaybackController is provided, AudioEngine syncs with it for timing state
     */
    AudioEngine(ITrackDataProvider* trackProvider, PlaybackController* playbackController);

    // AudioAppComponent Delegation Methods
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    // Playback Control (called from UI thread)
    void startPlayback();
    void stopPlayback();
    void resetPlaybackPosition();
    bool isPlaying() const;

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
    uint64_t getSamplePosition() const;
    int getSamplesPerStep() const;

    // Wavetable Synth access
    WavetableEngine& getWavetableEngine() { return wavetableEngine; }
    const WavetableEngine& getWavetableEngine() const { return wavetableEngine; }

private:
    // Dependencies (injected)
    ITrackDataProvider* trackProvider;
    PlaybackController* playbackController;  // Optional - may be null

    // Audio Thread State (owned by AudioEngine if no PlaybackController)
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
    uint64_t lastGlobalLoopCheck = 0;
    std::array<int, numTracks> trackLastStep = {-1, -1, -1, -1, -1, -1, -1, -1};
    std::array<int, numTracks> trackLastRatchet = {-1, -1, -1, -1, -1, -1, -1, -1};

    // Maximum buffer size for safety (prevents dynamic allocation in audio thread)
    static constexpr int maxBufferSize = 4096;

    // Fast LCG-based RNG for probability modifier (real-time safe, no allocations)
    class FastRNG
    {
    public:
        explicit FastRNG(uint32_t seed = 42) : state(seed) {}

        // Linear Congruential Generator - fast, deterministic, no heap usage
        uint32_t next()
        {
            state = state * 1103515245u + 12345u;
            return state;
        }

        // Returns value in range [1, max]
        int nextInt(int max)
        {
            return static_cast<int>((next() % static_cast<uint32_t>(max)) + 1);
        }

    private:
        uint32_t state;
    };

    FastRNG probabilityRng{42};

    // Buffers
    std::array<std::unique_ptr<juce::AudioBuffer<float>>, numTracks> trackBuffers;

    // StateVariableTPTFilter is safe for real-time parameter modulation without allocations!
    std::array<std::unique_ptr<juce::dsp::StateVariableTPTFilter<float>>, numTracks> modulationFilters;

    // Effects
    juce::Reverb masterReverb;
    juce::Reverb::Parameters reverbParams;

    // Wavetable Synth
    WavetableEngine wavetableEngine;
    std::unique_ptr<juce::AudioBuffer<float>> wavetableBuffer;
    juce::MidiBuffer wavetableMidiBuffer;

    // Helper Methods
    void calculateSamplesPerStep();
    void generateMidiForBuffer(const juce::AudioSourceChannelInfo& bufferToFill,
                             std::array<juce::MidiBuffer, numTracks>& trackMidiBuffers);
    void renderAndMixTracks(const juce::AudioSourceChannelInfo& bufferToFill,
                         const std::array<juce::MidiBuffer, numTracks>& trackMidiBuffers);
    void applyMasterReverb(juce::AudioBuffer<float>& buffer);
};
