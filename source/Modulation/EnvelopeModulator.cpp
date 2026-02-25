#include "EnvelopeModulator.h"

EnvelopeModulator::EnvelopeModulator()
{
}

void EnvelopeModulator::reset()
{
    currentStage = Stage::Idle;
    currentValue = 0.0f;
    stageProgress = 0.0f;
    samplesInStage = 0;
    stageLengthSamples = 0;
    releaseStartValue = 0.0f;
}

void EnvelopeModulator::noteOn()
{
    enterStage(Stage::Delay);
}

void EnvelopeModulator::noteOff(bool allowTailOff)
{
    if (currentStage != Stage::Idle)
    {
        releaseStartValue = currentValue;
        enterStage(allowTailOff ? Stage::Release : Stage::ForceRelease);
    }
}

void EnvelopeModulator::enterStage(Stage newStage)
{
    currentStage = newStage;
    samplesInStage = 0;
    stageProgress = 0.0f;

    // Calculate stage length in samples
    switch (newStage)
    {
        case Stage::Delay:
            stageLengthSamples = static_cast<int>(delay * sampleRate);
            break;
        case Stage::Attack:
            stageLengthSamples = static_cast<int>(attack * sampleRate);
            stageLengthSamples = juce::jmax(1, stageLengthSamples);
            break;
        case Stage::Hold:
            stageLengthSamples = static_cast<int>(hold * sampleRate);
            break;
        case Stage::Decay:
            stageLengthSamples = static_cast<int>(decay * sampleRate);
            stageLengthSamples = juce::jmax(1, stageLengthSamples);
            break;
        case Stage::Release:
        case Stage::ForceRelease:
            {
                float releaseTime = (newStage == Stage::ForceRelease) ? release * 0.1f : release;
                stageLengthSamples = static_cast<int>(releaseTime * sampleRate);
                stageLengthSamples = juce::jmax(1, stageLengthSamples);
            }
            break;
        default:
            stageLengthSamples = 0;
            break;
    }
}

float EnvelopeModulator::applyCurve(float linearValue, float curve) const
{
    if (std::abs(curve) < 0.01f)
        return linearValue;

    if (curve > 0.0f)
    {
        // Exponential curve (slow start, fast end)
        return std::pow(linearValue, 1.0f + curve * 3.0f);
    }
    else
    {
        // Logarithmic curve (fast start, slow end)
        float c = -curve;
        return 1.0f - std::pow(1.0f - linearValue, 1.0f + c * 3.0f);
    }
}

float EnvelopeModulator::calculateStageValue() const
{
    switch (currentStage)
    {
        case Stage::Idle:
            return 0.0f;

        case Stage::Delay:
            return 0.0f;

        case Stage::Attack:
            return applyCurve(stageProgress, attackCurve);

        case Stage::Hold:
            return 1.0f;

        case Stage::Decay:
            {
                float decayed = 1.0f - applyCurve(stageProgress, decayCurve);
                return sustain + (1.0f - sustain) * (1.0f - decayed);
            }

        case Stage::Sustain:
            return sustain;

        case Stage::Release:
        case Stage::ForceRelease:
            {
                float released = applyCurve(stageProgress, releaseCurve);
                return releaseStartValue * (1.0f - released);
            }

        default:
            return 0.0f;
    }
}

float EnvelopeModulator::process()
{
    if (!enabled || currentStage == Stage::Idle)
    {
        currentValue = 0.0f;
        return 0.0f;
    }

    // Calculate current value
    currentValue = calculateStageValue();

    // Advance through stage
    samplesInStage++;

    if (stageLengthSamples > 0)
    {
        stageProgress = static_cast<float>(samplesInStage) / stageLengthSamples;
    }
    else
    {
        stageProgress = 1.0f;
    }

    // Check for stage completion
    if (samplesInStage >= stageLengthSamples)
    {
        switch (currentStage)
        {
            case Stage::Delay:
                enterStage(Stage::Attack);
                break;
            case Stage::Attack:
                currentValue = 1.0f;
                enterStage(Stage::Hold);
                break;
            case Stage::Hold:
                enterStage(Stage::Decay);
                break;
            case Stage::Decay:
                currentValue = sustain;
                enterStage(Stage::Sustain);
                break;
            case Stage::Release:
            case Stage::ForceRelease:
                currentValue = 0.0f;
                enterStage(Stage::Idle);
                break;
            default:
                break;
        }
    }

    return currentValue;
}
