#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

/**
 * Waveshaper - Per-voice waveshaping/distortion effects
 *
 * Provides various shaping algorithms perfect for aggressive cyberpunk sounds:
 * - SoftClip: Smooth tanh saturation
 * - HardClip: Aggressive digital clipping
 * - Foldback: Wave folding for metallic sounds
 * - Bitcrush: Sample rate and bit reduction
 * - Rectify: Full/half wave rectification
 * - Saturate: Warm tube-like saturation
 */
class Waveshaper
{
public:
    enum class Mode
    {
        Off = 0,
        SoftClip,
        HardClip,
        Foldback,
        Bitcrush,
        Rectify,
        Saturate,
        Count
    };

    Waveshaper();
    ~Waveshaper() = default;

    // Parameter setters (thread-safe)
    void setMode(Mode m) { mode.store(m); }
    void setAmount(float amt) { amount.store(juce::jlimit(0.0f, 1.0f, amt)); }
    void setMix(float m) { mix.store(juce::jlimit(0.0f, 1.0f, m)); }

    // Parameter getters
    Mode getMode() const { return mode.load(); }
    float getAmount() const { return amount.load(); }
    float getMix() const { return mix.load(); }

    // Process stereo sample in-place
    void process(float& leftSample, float& rightSample);

    // Reset state (for voice reuse)
    void reset();

    // Set sample rate for bitcrush
    void setSampleRate(double sr) { sampleRate = sr; }

private:
    std::atomic<Mode> mode{Mode::Off};
    std::atomic<float> amount{0.5f};
    std::atomic<float> mix{1.0f};

    double sampleRate = 44100.0;

    // Bitcrush state
    float heldLeftSample = 0.0f;
    float heldRightSample = 0.0f;
    int bitcrushPhase = 0;

    // Individual shaping algorithms
    float applySoftClip(float sample, float amt);
    float applyHardClip(float sample, float amt);
    float applyFoldback(float sample, float amt);
    float applyBitcrush(float sample, float amt);
    float applyRectify(float sample, float amt);
    float applySaturate(float sample, float amt);
};
