#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

/**
 * Modulator - Base class for all modulation sources (LFOs, Envelopes, etc.)
 */
class Modulator
{
public:
    Modulator() = default;
    virtual ~Modulator() = default;

    // Process one sample and return the current modulation value (0.0 to 1.0 typically)
    virtual float process() = 0;

    // Reset the modulator to initial state
    virtual void reset() = 0;

    // Set sample rate
    virtual void setSampleRate(double sampleRate) { this->sampleRate = sampleRate; }

    // Enable/disable the modulator
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

    // Get current output value without processing
    virtual float getCurrentValue() const = 0;

protected:
    double sampleRate = 44100.0;
    bool enabled = true;
};
