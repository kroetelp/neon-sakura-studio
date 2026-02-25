#include "LFOModulator.h"
#include <cstdlib>
#include <ctime>

LFOModulator::LFOModulator()
{
    static bool seeded = false;
    if (!seeded)
    {
        std::srand(static_cast<unsigned>(std::time(nullptr)));
        seeded = true;
    }
    nextRandomValue = static_cast<float>(std::rand()) / RAND_MAX;
    updatePhaseIncrement();
}

void LFOModulator::updatePhaseIncrement()
{
    if (sampleRate <= 0.0)
        return;

    if (tempoSync)
    {
        // Convert beats to Hz
        float beatsPerSecond = currentBPM / 60.0f;
        float hz = beatsPerSecond / tempoRateBeats;
        phaseIncrement = hz / static_cast<float>(sampleRate);
    }
    else
    {
        phaseIncrement = rateHz / static_cast<float>(sampleRate);
    }
}

void LFOModulator::setBPM(float bpm)
{
    currentBPM = bpm;
    if (tempoSync)
        updatePhaseIncrement();
}

void LFOModulator::reset()
{
    phase = 0.0f;
    currentValue = 0.0f;
    samplesUntilNextRandom = 0;
    nextRandomValue = static_cast<float>(std::rand()) / RAND_MAX;
}

float LFOModulator::generateRandomValue()
{
    // Sample and hold: change random value at each cycle
    if (samplesUntilNextRandom <= 0)
    {
        randomValue = nextRandomValue;
        nextRandomValue = static_cast<float>(std::rand()) / RAND_MAX;

        // Calculate samples until next change (based on rate)
        samplesUntilNextRandom = static_cast<int>(sampleRate / (tempoSync ? (currentBPM / 60.0f / tempoRateBeats) : rateHz));
    }
    else
    {
        samplesUntilNextRandom--;
    }

    return randomValue;
}

float LFOModulator::generateWaveform(float ph)
{
    switch (waveform)
    {
        case Waveform::Sine:
            return std::sin(ph * 2.0f * juce::MathConstants<float>::pi);

        case Waveform::Triangle:
            return 2.0f * std::abs(2.0f * ph - 1.0f) - 1.0f;

        case Waveform::SawUp:
            return 2.0f * ph - 1.0f;

        case Waveform::SawDown:
            return 1.0f - 2.0f * ph;

        case Waveform::Square:
            return (ph < 0.5f) ? 1.0f : -1.0f;

        case Waveform::SampleAndHold:
            return generateRandomValue() * 2.0f - 1.0f;

        case Waveform::RandomSmooth:
            {
                generateRandomValue();
                // Interpolate between random values
                float t = 1.0f - static_cast<float>(samplesUntilNextRandom) /
                         (sampleRate / (tempoSync ? (currentBPM / 60.0f / tempoRateBeats) : rateHz));
                t = juce::jlimit(0.0f, 1.0f, t);
                // Smooth interpolation
                float smoothT = t * t * (3.0f - 2.0f * t);
                float value = randomValue * (1.0f - smoothT) + nextRandomValue * smoothT;
                return value * 2.0f - 1.0f;
            }

        default:
            return 0.0f;
    }
}

float LFOModulator::process()
{
    if (!enabled || depth <= 0.0f)
    {
        currentValue = 0.0f;
        return 0.0f;
    }

    // Generate waveform value (-1 to 1)
    float waveValue = generateWaveform(phase);

    // Scale to 0-1 range and apply depth
    currentValue = (waveValue + 1.0f) * 0.5f * depth;

    // Advance phase
    phase += phaseIncrement;
    while (phase >= 1.0f)
        phase -= 1.0f;

    return currentValue;
}
