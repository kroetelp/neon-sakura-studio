#pragma once

#include "Modulator.h"
#include <cmath>

/**
 * EnvelopeModulator - ADSR envelope with optional additional stages
 *
 * Supports standard ADSR plus optional:
 * - Delay before attack
 * - Hold after attack
 * - Curve shapes for each stage
 */
class EnvelopeModulator : public Modulator
{
public:
    enum class Stage
    {
        Idle,
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        ForceRelease
    };

    EnvelopeModulator();
    ~EnvelopeModulator() override = default;

    // Modulator interface
    float process() override;
    void reset() override;
    float getCurrentValue() const override { return currentValue; }

    // Envelope control
    void noteOn();
    void noteOff(bool allowTailOff = true);

    // Parameter setters (all thread-safe via direct call from UI)
    void setAttack(float seconds) { attack = juce::jlimit(0.001f, 30.0f, seconds); }
    void setDecay(float seconds) { decay = juce::jlimit(0.001f, 30.0f, seconds); }
    void setSustain(float level) { sustain = juce::jlimit(0.0f, 1.0f, level); }
    void setRelease(float seconds) { release = juce::jlimit(0.001f, 30.0f, seconds); }

    // Optional stages
    void setDelay(float seconds) { delay = juce::jlimit(0.0f, 5.0f, seconds); }
    void setHold(float seconds) { hold = juce::jlimit(0.0f, 5.0f, seconds); }

    // Curve shape (0 = linear, 1 = exponential, -1 = logarithmic)
    void setAttackCurve(float curve) { attackCurve = juce::jlimit(-1.0f, 1.0f, curve); }
    void setDecayCurve(float curve) { decayCurve = juce::jlimit(-1.0f, 1.0f, curve); }
    void setReleaseCurve(float curve) { releaseCurve = juce::jlimit(-1.0f, 1.0f, curve); }

    // Getters
    Stage getStage() const { return currentStage; }
    float getAttack() const { return attack; }
    float getDecay() const { return decay; }
    float getSustain() const { return sustain; }
    float getRelease() const { return release; }

    // Check if envelope is finished
    bool isActive() const { return currentStage != Stage::Idle; }
    bool isInRelease() const { return currentStage == Stage::Release || currentStage == Stage::ForceRelease; }

private:
    // Envelope parameters
    float delay = 0.0f;
    float attack = 0.01f;
    float hold = 0.0f;
    float decay = 0.1f;
    float sustain = 0.7f;
    float release = 0.3f;

    // Curve shapes
    float attackCurve = 0.0f;
    float decayCurve = 0.5f;
    float releaseCurve = 0.5f;

    // State
    Stage currentStage = Stage::Idle;
    float currentValue = 0.0f;
    float stageProgress = 0.0f;   // 0 to 1 progress through current stage
    float releaseStartValue = 0.0f;

    // Sample counters
    int samplesInStage = 0;
    int stageLengthSamples = 0;

    // Helper methods
    void enterStage(Stage newStage);
    float calculateStageValue() const;
    float applyCurve(float linearValue, float curve) const;
};
