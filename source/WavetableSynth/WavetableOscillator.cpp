#include "WavetableOscillator.h"
#include <cmath>

WavetableOscillator::WavetableOscillator()
{
    resetPhase();
}

void WavetableOscillator::setWavetable(std::shared_ptr<WavetableData> table)
{
    wavetable = table;
}

void WavetableOscillator::setUnisonCount(int count)
{
    unisonCount = juce::jlimit(1, maxUnisonVoices, count);
    updatePhaseIncrements();
    updatePanPositions();
}

void WavetableOscillator::resetPhase()
{
    // Initialize all unison voices with slight phase offsets for richness
    for (int i = 0; i < maxUnisonVoices; ++i)
    {
        unisonVoices[i].phase = static_cast<float>(i) / maxUnisonVoices;
    }
}

void WavetableOscillator::updatePhaseIncrements()
{
    if (sampleRate <= 0.0 || frequency <= 0.0f)
        return;

    float basePhaseIncrement = frequency / static_cast<float>(sampleRate);

    for (int i = 0; i < unisonCount; ++i)
    {
        // Calculate detune ratio for this voice
        float voiceDetune = 0.0f;

        if (unisonCount > 1)
        {
            // Spread detune across voices
            // Center voice (or first of even count) has no detune
            float normalizedPos = (static_cast<float>(i) / (unisonCount - 1)) - 0.5f;
            voiceDetune = normalizedPos * detuneCents;

            // Convert cents to frequency ratio: 2^(cents/1200)
            float detuneRatio = std::pow(2.0f, voiceDetune / 1200.0f);
            unisonVoices[i].detuneRatio = detuneRatio;
        }
        else
        {
            unisonVoices[i].detuneRatio = 1.0f;
        }

        unisonVoices[i].phaseIncrement = basePhaseIncrement * unisonVoices[i].detuneRatio;
    }
}

void WavetableOscillator::updatePanPositions()
{
    if (unisonCount == 1)
    {
        // Single voice uses the main pan position
        float pan = (panPosition + 1.0f) * 0.5f;  // Convert -1..1 to 0..1
        unisonVoices[0].panLeft = std::sqrt(1.0f - pan);
        unisonVoices[0].panRight = std::sqrt(pan);
    }
    else
    {
        // Spread voices across stereo field
        for (int i = 0; i < unisonCount; ++i)
        {
            float normalizedPos = (unisonCount > 1)
                ? static_cast<float>(i) / (unisonCount - 1)
                : 0.5f;

            // Apply pan spread
            float spreadPan = (normalizedPos - 0.5f) * panSpread + 0.5f;

            // Also apply main pan position
            float finalPan = spreadPan + (panPosition + 1.0f) * 0.25f;
            finalPan = juce::jlimit(0.0f, 1.0f, finalPan);

            // Constant power panning
            unisonVoices[i].panLeft = std::sqrt(1.0f - finalPan);
            unisonVoices[i].panRight = std::sqrt(finalPan);
        }
    }
}

float WavetableOscillator::getSampleFromWavetable(float phase)
{
    if (!wavetable)
        return 0.0f;

    return wavetable->getSample(phase, morphPosition);
}

void WavetableOscillator::process(float& leftOut, float& rightOut)
{
    leftOut = 0.0f;
    rightOut = 0.0f;

    if (!wavetable || frequency <= 0.0f)
        return;

    float unisonGain = 1.0f / std::sqrt(static_cast<float>(unisonCount));

    for (int i = 0; i < unisonCount; ++i)
    {
        auto& voice = unisonVoices[i];

        // Get sample from wavetable
        float sample = getSampleFromWavetable(voice.phase);

        // Apply gain and panning
        sample *= level * unisonGain;
        leftOut += sample * voice.panLeft;
        rightOut += sample * voice.panRight;

        // Advance phase
        voice.phase += voice.phaseIncrement;

        // Wrap phase
        while (voice.phase >= 1.0f)
            voice.phase -= 1.0f;
    }
}
