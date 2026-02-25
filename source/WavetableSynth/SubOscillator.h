#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
 * SubOscillator - Simple sub oscillator for adding low-end weight
 * Generates sine, triangle, or square wave at 1 or 2 octaves below main pitch
 */
class SubOscillator
{
public:
    enum class Waveform
    {
        Sine,
        Triangle,
        Square,
        Noise
    };

    SubOscillator();
    ~SubOscillator() = default;

    // Parameters
    void setFrequency(float freq) { frequency = freq; updatePhaseIncrement(); }
    void setOctave(int oct) { octaveDown = juce::jlimit(0, 2, oct); updatePhaseIncrement(); }
    void setWaveform(Waveform wf) { waveform = wf; }
    void setLevel(float lvl) { level = lvl; }
    void setSampleRate(double sr) { sampleRate = sr; updatePhaseIncrement(); }

    // Reset for new note
    void resetPhase() { phase = 0.0f; }

    // Process and get mono sample
    float process();

private:
    float frequency = 440.0f;
    int octaveDown = 1;       // 0, 1, or 2 octaves down
    Waveform waveform = Waveform::Sine;
    float level = 0.0f;
    double sampleRate = 44100.0;

    float phase = 0.0f;
    float phaseIncrement = 0.0f;

    void updatePhaseIncrement();
    float generateWaveform(float phase) const;
};
