#pragma once

#include "Modulator.h"
#include <cmath>
#include <functional>

/**
 * LFOModulator - Low Frequency Oscillator for modulation
 * Supports multiple waveforms and tempo sync
 */
class LFOModulator : public Modulator
{
public:
    enum class Waveform
    {
        Sine,
        Triangle,
        SawUp,
        SawDown,
        Square,
        SampleAndHold,
        RandomSmooth,
        Count
    };

    LFOModulator();
    ~LFOModulator() override = default;

    // Modulator interface
    float process() override;
    void reset() override;
    float getCurrentValue() const override { return currentValue; }

    // LFO-specific parameters
    void setWaveform(Waveform wf) { waveform = wf; }
    void setRate(float hz) { rateHz = juce::jlimit(0.01f, 50.0f, hz); updatePhaseIncrement(); }
    void setDepth(float depth) { this->depth = juce::jlimit(0.0f, 1.0f, depth); }
    void setPhase(float ph) { phase = juce::jlimit(0.0f, 1.0f, ph); }

    // Tempo sync (rate in beats instead of Hz)
    void setTempoSync(bool sync) { tempoSync = sync; }
    void setTempoRate(float beats) { tempoRateBeats = beats; updatePhaseIncrement(); }
    void setBPM(float bpm);

    // Getters
    Waveform getWaveform() const { return waveform; }
    float getRate() const { return rateHz; }
    float getDepth() const { return depth; }
    float getPhase() const { return phase; }

private:
    Waveform waveform = Waveform::Sine;
    float rateHz = 1.0f;
    float depth = 1.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;

    bool tempoSync = false;
    float tempoRateBeats = 1.0f;  // 1 = quarter note
    float currentBPM = 120.0f;

    float currentValue = 0.0f;

    // For random waveforms
    float randomValue = 0.0f;
    float nextRandomValue = 0.0f;
    float randomSmoothing = 0.0f;
    int samplesUntilNextRandom = 0;

    void updatePhaseIncrement();
    float generateWaveform(float phase);
    float generateRandomValue();
};
