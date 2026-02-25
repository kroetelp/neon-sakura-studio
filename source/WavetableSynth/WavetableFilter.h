#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

/**
 * WavetableFilter - Multi-mode filter for wavetable synth
 * Supports Low-pass, High-pass, Band-pass, and Notch modes
 * With drive/distortion and resonance control
 */
class WavetableFilter
{
public:
    enum class Mode
    {
        LowPass,
        HighPass,
        BandPass,
        Notch,
        Count
    };

    WavetableFilter();
    ~WavetableFilter() = default;

    // Filter parameters
    void setMode(Mode m);
    void setCutoff(float freq) { cutoff.store(juce::jlimit(20.0f, 20000.0f, freq)); }
    void setResonance(float res) { resonance.store(juce::jlimit(0.0f, 1.0f, res)); }
    void setDrive(float drv) { drive.store(juce::jlimit(0.0f, 1.0f, drv)); }
    void setSampleRate(double sr);

    // Process stereo sample
    void process(float& leftSample, float& rightSample);

    // Reset filter state
    void reset();

    // Get current values (thread-safe)
    float getCutoff() const { return cutoff.load(); }
    float getResonance() const { return resonance.load(); }
    float getDrive() const { return drive.load(); }
    Mode getMode() const { return mode; }

private:
    std::atomic<float> cutoff{1000.0f};
    std::atomic<float> resonance{0.0f};
    std::atomic<float> drive{0.0f};
    Mode mode = Mode::LowPass;

    double sampleRate = 44100.0;

    // Filter coefficients and state (per channel)
    struct FilterState
    {
        float x1 = 0.0f, x2 = 0.0f;  // Input history
        float y1 = 0.0f, y2 = 0.0f;  // Output history
    };
    FilterState leftState, rightState;

    // Coefficients
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    // Cached values for coefficient update
    float lastCutoff = -1.0f;
    float lastResonance = -1.0f;
    Mode lastMode = Mode::Count;

    void updateCoefficients();
    float processSample(float sample, FilterState& state);
    float applyDrive(float sample);
};
