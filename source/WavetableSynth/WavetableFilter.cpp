#include "WavetableFilter.h"
#include <cmath>

WavetableFilter::WavetableFilter()
{
    updateCoefficients();
}

void WavetableFilter::setMode(Mode m)
{
    mode = m;
}

void WavetableFilter::setSampleRate(double sr)
{
    sampleRate = sr;
    lastCutoff = -1.0f;  // Force coefficient update
    updateCoefficients();
}

void WavetableFilter::reset()
{
    leftState = FilterState();
    rightState = FilterState();
}

void WavetableFilter::updateCoefficients()
{
    float currentCutoff = cutoff.load();
    float currentResonance = resonance.load();

    // Only update if parameters changed
    if (currentCutoff == lastCutoff && currentResonance == lastResonance && mode == lastMode)
        return;

    lastCutoff = currentCutoff;
    lastResonance = currentResonance;
    lastMode = mode;

    // Calculate normalized frequency
    float omega = 2.0f * juce::MathConstants<float>::pi * currentCutoff / static_cast<float>(sampleRate);
    float sinOmega = std::sin(omega);
    float cosOmega = std::cos(omega);

    // Q factor from resonance (0-1 -> 0.5 to 20)
    float Q = 0.5f + currentResonance * 19.5f;
    float alpha = sinOmega / (2.0f * Q);

    switch (mode)
    {
        case Mode::LowPass:
            {
                float b0 = (1.0f - cosOmega) / 2.0f;
                float b1 = 1.0f - cosOmega;
                float b2 = (1.0f - cosOmega) / 2.0f;
                float a0 = 1.0f + alpha;
                float a1 = -2.0f * cosOmega;
                float a2 = 1.0f - alpha;

                // Normalize
                this->b0 = b0 / a0;
                this->b1 = b1 / a0;
                this->b2 = b2 / a0;
                this->a1 = a1 / a0;
                this->a2 = a2 / a0;
            }
            break;

        case Mode::HighPass:
            {
                float b0 = (1.0f + cosOmega) / 2.0f;
                float b1 = -(1.0f + cosOmega);
                float b2 = (1.0f + cosOmega) / 2.0f;
                float a0 = 1.0f + alpha;
                float a1 = -2.0f * cosOmega;
                float a2 = 1.0f - alpha;

                this->b0 = b0 / a0;
                this->b1 = b1 / a0;
                this->b2 = b2 / a0;
                this->a1 = a1 / a0;
                this->a2 = a2 / a0;
            }
            break;

        case Mode::BandPass:
            {
                float b0 = alpha;
                float b1 = 0.0f;
                float b2 = -alpha;
                float a0 = 1.0f + alpha;
                float a1 = -2.0f * cosOmega;
                float a2 = 1.0f - alpha;

                this->b0 = b0 / a0;
                this->b1 = b1 / a0;
                this->b2 = b2 / a0;
                this->a1 = a1 / a0;
                this->a2 = a2 / a0;
            }
            break;

        case Mode::Notch:
            {
                float b0 = 1.0f;
                float b1 = -2.0f * cosOmega;
                float b2 = 1.0f;
                float a0 = 1.0f + alpha;
                float a1 = -2.0f * cosOmega;
                float a2 = 1.0f - alpha;

                this->b0 = b0 / a0;
                this->b1 = b1 / a0;
                this->b2 = b2 / a0;
                this->a1 = a1 / a0;
                this->a2 = a2 / a0;
            }
            break;

        default:
            // Pass-through
            this->b0 = 1.0f;
            this->b1 = 0.0f;
            this->b2 = 0.0f;
            this->a1 = 0.0f;
            this->a2 = 0.0f;
            break;
    }
}

float WavetableFilter::applyDrive(float sample)
{
    float currentDrive = drive.load();

    if (currentDrive <= 0.0f)
        return sample;

    // Soft saturation / waveshaping
    float driveGain = 1.0f + currentDrive * 3.0f;
    float drivenSample = sample * driveGain;

    // Tanh saturation
    return std::tanh(drivenSample);
}

float WavetableFilter::processSample(float sample, FilterState& state)
{
    // Update coefficients if needed
    updateCoefficients();

    // Apply drive before filter
    sample = applyDrive(sample);

    // Biquad filter (Direct Form I)
    float output = b0 * sample + b1 * state.x1 + b2 * state.x2
                   - a1 * state.y1 - a2 * state.y2;

    // Update state
    state.x2 = state.x1;
    state.x1 = sample;
    state.y2 = state.y1;
    state.y1 = output;

    // Prevent denormals
    if (std::abs(output) < 1e-10f)
        output = 0.0f;

    return output;
}

void WavetableFilter::process(float& leftSample, float& rightSample)
{
    leftSample = processSample(leftSample, leftState);
    rightSample = processSample(rightSample, rightState);
}
