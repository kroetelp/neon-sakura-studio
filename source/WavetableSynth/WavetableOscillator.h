#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <memory>
#include <atomic>
#include "WavetableData.h"

/**
 * WavetableOscillator - Per-voice wavetable oscillator with:
 * - Wavetable morphing
 * - Unison mode with multiple detuned voices
 * - Pan spread for stereo width
 * - Linear interpolation between wavetable frames
 * - FM and AM modulation inputs
 */
class WavetableOscillator
{
public:
    static constexpr int maxUnisonVoices = 8;

    WavetableOscillator();
    ~WavetableOscillator() = default;

    // Set the wavetable data (shared pointer for multiple oscillators)
    void setWavetable(std::shared_ptr<WavetableData> table);

    // Oscillator parameters (thread-safe via atomics in parent)
    void setFrequency(float freq) { frequency = freq; updatePhaseIncrements(); }
    void setMorphPosition(float pos) { morphPosition = juce::jlimit(0.0f, 1.0f, pos); }
    void setLevel(float lvl) { level = lvl; }
    void setPan(float pan) { panPosition = juce::jlimit(-1.0f, 1.0f, pan); }

    // Unison parameters
    void setUnisonCount(int count);
    void setDetune(float cents) { detuneCents = cents; updatePhaseIncrements(); }
    void setPanSpread(float spread) { panSpread = spread; updatePanPositions(); }

    // FM/AM Modulation inputs (applied during process())
    void setFMInput(float fmSignal) { fmInput = fmSignal; }
    void setAMInput(float amSignal) { amInput = amSignal; }
    void setFMAmount(float amt) { fmAmount = amt; }
    void setAMAmount(float amt) { amAmount = amt; }

    // Phase reset for new note
    void resetPhase();

    // Sample rate must be set before processing
    void setSampleRate(double sr) { sampleRate = sr; updatePhaseIncrements(); }

    // Process and get next sample (stereo output)
    void process(float& leftOut, float& rightOut);

    // Get current frequency
    float getFrequency() const { return frequency; }

    // Get last mono output (for modulation routing)
    float getLastOutput() const { return lastOutput; }

private:
    std::shared_ptr<WavetableData> wavetable;

    // Base parameters
    float frequency = 440.0f;
    float morphPosition = 0.0f;
    float level = 1.0f;
    float panPosition = 0.0f;
    double sampleRate = 44100.0;

    // Unison parameters
    int unisonCount = 1;
    float detuneCents = 0.0f;    // Detune in cents
    float panSpread = 0.0f;       // Stereo spread

    // FM/AM modulation
    float fmInput = 0.0f;        // External FM signal (-1 to 1)
    float amInput = 1.0f;        // External AM signal (0 to 1 typically)
    float fmAmount = 0.0f;       // FM depth in semitones
    float amAmount = 0.0f;       // AM depth (0-1)
    float lastOutput = 0.0f;     // Last mono output for modulation

    // Per-unison-voice state
    struct UnisonVoice
    {
        float phase = 0.0f;
        float phaseIncrement = 0.0f;
        float detuneRatio = 1.0f;
        float panLeft = 0.5f;
        float panRight = 0.5f;
    };
    std::array<UnisonVoice, maxUnisonVoices> unisonVoices;

    // Helper methods
    void updatePhaseIncrements();
    void updatePanPositions();
    float getSampleFromWavetable(float phase);
};
