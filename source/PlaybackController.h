#pragma once

/**
 * PlaybackController - Manages playback state and timing
 *
 * Responsibilities:
 * - Play/Pause/Stop control
 * - BPM and Swing management
 * - Loop length control
 * - Playhead position tracking
 * - Callbacks for UI updates
 *
 * This class owns the playback state atomics and provides thread-safe access.
 * AudioEngine references this controller instead of owning its own state.
 */

#include <atomic>
#include <functional>

class PlaybackController
{
public:
    PlaybackController();
    ~PlaybackController() = default;

    // Playback control
    void startPlayback();
    void stopPlayback();
    void resetPlaybackPosition();
    bool isPlaying() const { return playing.load(); }

    // Timing control (thread-safe)
    void setBPM(double newBpm);
    double getBPM() const { return bpm.load(); }
    void setSwingAmount(float swing);
    float getSwingAmount() const { return swingAmount.load(); }
    void setLoopLength(int steps);
    int getLoopLength() const { return loopLength.load(); }

    // Master volume (thread-safe)
    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume.load(); }

    // Reverb wet level (thread-safe)
    void setReverbWetLevel(float wetLevel);
    float getReverbWetLevel() const { return reverbWetLevel.load(); }

    // Position access (for UI)
    uint64_t getSamplePosition() const { return samplePosition.load(); }
    int getSamplesPerStep() const { return samplesPerStep.load(); }

    // Audio thread updates (called from AudioEngine)
    void advancePosition(uint64_t samples);
    void updateSamplesPerStep(int samples);
    void setSamplePosition(uint64_t position);

    // Sample rate for calculations
    void setSampleRate(double rate);
    double getSampleRate() const { return sampleRate.load(); }

    // Recalculate samples per step (called when BPM or sample rate changes)
    void calculateSamplesPerStep();

    // UI Callbacks
    std::function<void(double)> onBpmChanged;
    std::function<void(bool)> onPlaybackStateChanged;
    std::function<void()> onReset;

private:
    // Playback state
    std::atomic<bool> playing{false};
    std::atomic<double> bpm{120.0};
    std::atomic<float> swingAmount{0.0f};
    std::atomic<int> loopLength{16};
    std::atomic<int> samplesPerStep{0};
    std::atomic<uint64_t> samplePosition{0};
    std::atomic<double> sampleRate{44100.0};

    // Master controls
    std::atomic<float> masterVolume{0.8f};
    std::atomic<float> reverbWetLevel{0.3f};
};
