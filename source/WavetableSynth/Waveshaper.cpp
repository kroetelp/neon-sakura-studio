#include "Waveshaper.h"

Waveshaper::Waveshaper()
{
}

void Waveshaper::reset()
{
    heldLeftSample = 0.0f;
    heldRightSample = 0.0f;
    bitcrushPhase = 0;
}

void Waveshaper::process(float& leftSample, float& rightSample)
{
    Mode currentMode = mode.load();
    float currentAmount = amount.load();
    float currentMix = mix.load();

    if (currentMode == Mode::Off || currentMix < 0.001f)
        return;

    // Apply shaping
    float shapedLeft = leftSample;
    float shapedRight = rightSample;

    switch (currentMode)
    {
        case Mode::SoftClip:
            shapedLeft = applySoftClip(shapedLeft, currentAmount);
            shapedRight = applySoftClip(shapedRight, currentAmount);
            break;

        case Mode::HardClip:
            shapedLeft = applyHardClip(shapedLeft, currentAmount);
            shapedRight = applyHardClip(shapedRight, currentAmount);
            break;

        case Mode::Foldback:
            shapedLeft = applyFoldback(shapedLeft, currentAmount);
            shapedRight = applyFoldback(shapedRight, currentAmount);
            break;

        case Mode::Bitcrush:
            shapedLeft = applyBitcrush(shapedLeft, currentAmount);
            shapedRight = applyBitcrush(shapedRight, currentAmount);
            break;

        case Mode::Rectify:
            shapedLeft = applyRectify(shapedLeft, currentAmount);
            shapedRight = applyRectify(shapedRight, currentAmount);
            break;

        case Mode::Saturate:
            shapedLeft = applySaturate(shapedLeft, currentAmount);
            shapedRight = applySaturate(shapedRight, currentAmount);
            break;

        default:
            break;
    }

    // Mix dry/wet
    leftSample = leftSample * (1.0f - currentMix) + shapedLeft * currentMix;
    rightSample = rightSample * (1.0f - currentMix) + shapedRight * currentMix;
}

float Waveshaper::applySoftClip(float sample, float amt)
{
    // Smooth tanh saturation with adjustable drive
    float drive = 1.0f + amt * 4.0f;  // 1x to 5x drive
    return std::tanh(sample * drive) / std::tanh(drive);
}

float Waveshaper::applyHardClip(float sample, float amt)
{
    // Aggressive digital clipping
    float threshold = 1.0f - amt * 0.9f;  // 1.0 to 0.1 threshold

    if (sample > threshold)
        return threshold;
    else if (sample < -threshold)
        return -threshold;

    return sample;
}

float Waveshaper::applyFoldback(float sample, float amt)
{
    // Wave folding - creates metallic, harmonically rich sounds
    // Perfect for cyberpunk textures!
    float folds = 1.0f + amt * 4.0f;  // 1 to 5 folds
    float gain = 1.0f + amt * 2.0f;   // Pre-gain

    sample *= gain;

    for (int i = 0; i < static_cast<int>(folds); ++i)
    {
        // Fold back when exceeding [-1, 1]
        if (sample > 1.0f)
            sample = 2.0f - sample;
        else if (sample < -1.0f)
            sample = -2.0f - sample;
    }

    // Additional fractional fold for smooth transition
    float fractionalFold = folds - static_cast<int>(folds);
    if (fractionalFold > 0.001f)
    {
        if (sample > 1.0f)
            sample = juce::jmap(fractionalFold, 2.0f - sample, sample);
        else if (sample < -1.0f)
            sample = juce::jmap(fractionalFold, -2.0f - sample, sample);
    }

    return juce::jlimit(-1.0f, 1.0f, sample);
}

float Waveshaper::applyBitcrush(float sample, float amt)
{
    // Sample rate reduction (downsampling)
    int downsampleFactor = 1 + static_cast<int>(amt * 15.0f);  // 1 to 16

    bitcrushPhase++;
    if (bitcrushPhase >= downsampleFactor)
    {
        bitcrushPhase = 0;
        heldLeftSample = sample;  // Note: This is per-channel but we track one phase
    }

    // Bit reduction (quantization)
    float bits = 16.0f - amt * 12.0f;  // 16 to 4 bits
    float quantizationSteps = std::pow(2.0f, bits);
    float crushed = std::round(sample * quantizationSteps) / quantizationSteps;

    // Combine downsample held value with bit reduction
    float output = heldLeftSample;
    output = std::round(output * quantizationSteps) / quantizationSteps;

    return output;
}

float Waveshaper::applyRectify(float sample, float amt)
{
    // Full-wave rectification with mix
    float rectified = std::abs(sample);

    // Half-wave rectification option based on amount
    if (amt > 0.5f && sample < 0.0f)
    {
        float halfWaveMix = (amt - 0.5f) * 2.0f;
        rectified = rectified * (1.0f - halfWaveMix) + 0.0f * halfWaveMix;
    }

    // Add harmonic enhancement at higher amounts
    if (amt > 0.0f)
    {
        // Create asymmetrical clipping for even harmonics
        float asymmetry = amt * 0.3f;
        if (sample > 0.0f)
            rectified *= (1.0f + asymmetry);
        else
            rectified *= (1.0f - asymmetry * 0.5f);
    }

    // Scale back to original range but keep the punch
    return rectified * (1.0f - amt * 0.3f);
}

float Waveshaper::applySaturate(float sample, float amt)
{
    // Warm tube-like saturation using a combination of techniques
    float drive = 1.0f + amt * 3.0f;

    // Pre-gain
    sample *= drive;

    // Asymmetric saturation (tube-like behavior)
    float positiveSlope = 1.0f + amt * 0.5f;
    float negativeSlope = 1.0f - amt * 0.2f;

    if (sample > 0.0f)
    {
        // Positive half: softer saturation
        sample = sample / (1.0f + std::abs(sample) * positiveSlope);
    }
    else
    {
        // Negative half: slightly different character
        sample = sample / (1.0f + std::abs(sample) * negativeSlope);
    }

    // Add subtle 2nd harmonic for warmth
    float fundamental = sample;
    float secondHarmonic = sample * sample * 0.15f * amt;
    sample = fundamental + secondHarmonic;

    // Soft clip to keep in range
    if (sample > 1.0f)
        sample = 1.0f - std::exp(-(sample - 1.0f) * 2.0f) * 0.5f;
    else if (sample < -1.0f)
        sample = -1.0f + std::exp(-(-sample - 1.0f) * 2.0f) * 0.5f;

    return sample;
}
