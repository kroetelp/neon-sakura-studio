#include "SubOscillator.h"
#include <random>

SubOscillator::SubOscillator()
{
    updatePhaseIncrement();
}

void SubOscillator::updatePhaseIncrement()
{
    if (sampleRate <= 0.0 || frequency <= 0.0f)
    {
        phaseIncrement = 0.0f;
        return;
    }

    // Calculate frequency with octave offset
    float subFreq = frequency;
    for (int i = 0; i < octaveDown; ++i)
        subFreq *= 0.5f;

    phaseIncrement = subFreq / static_cast<float>(sampleRate);
}

float SubOscillator::generateWaveform(float p) const
{
    switch (waveform)
    {
        case Waveform::Sine:
            return std::sin(p * 2.0f * juce::MathConstants<float>::pi);

        case Waveform::Triangle:
            {
                float t = p * 4.0f;
                if (t < 2.0f)
                    return t - 1.0f;
                else
                    return 3.0f - t;
            }

        case Waveform::Square:
            return (p < 0.5f) ? 1.0f : -1.0f;

        case Waveform::Noise:
            // Simple noise (not really a waveform but useful for texture)
            return (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;

        default:
            return 0.0f;
    }
}

float SubOscillator::process()
{
    if (level <= 0.0f || phaseIncrement <= 0.0f)
        return 0.0f;

    float sample = generateWaveform(phase) * level;

    // Advance phase
    phase += phaseIncrement;
    while (phase >= 1.0f)
        phase -= 1.0f;

    return sample;
}
