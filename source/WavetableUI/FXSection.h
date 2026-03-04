#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableEngine.h"
#include "../Theme/ThemeManager.h"
#include <memory>

/**
 * FXSection - Controls for the master effects rack
 *
 * Provides controls for:
 * - Chorus (mix, rate, depth)
 * - Delay (mix, time, feedback)
 * - Reverb (mix, size, damping)
 */
class FXSection : public juce::Component
{
public:
    FXSection();
    ~FXSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Connect to engine FX params
    void connectToEngine(WavetableEngine* engine);

    // Update UI from parameter values
    void updateFromParams(const WavetableEngine::FXParams& params);

private:
    WavetableEngine* engine = nullptr;

    // === CHORUS ===
    juce::Slider chorusMixSlider;
    juce::Slider chorusRateSlider;
    juce::Slider chorusDepthSlider;

    // === DELAY ===
    juce::Slider delayMixSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;

    // === REVERB ===
    juce::Slider reverbMixSlider;
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampingSlider;

    // Labels
    juce::Label titleLabel;

    juce::Label chorusLabel;
    juce::Label chorusMixLabel;
    juce::Label chorusRateLabel;
    juce::Label chorusDepthLabel;

    juce::Label delayLabel;
    juce::Label delayMixLabel;
    juce::Label delayTimeLabel;
    juce::Label delayFeedbackLabel;

    juce::Label reverbLabel;
    juce::Label reverbMixLabel;
    juce::Label reverbSizeLabel;
    juce::Label reverbDampingLabel;

    // Colors (delegated to ThemeManager)
    static juce::Colour getNeonCyan() { return ThemeManager::getInstance().getInfoColor(); }
    static juce::Colour getNeonPink() { return ThemeManager::getInstance().getAccentColor(); }
    static juce::Colour getNeonPurple() { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    static juce::Colour getNeonGreen() { return ThemeManager::getInstance().getSuccessColor(); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name,
                     double minVal, double maxVal, double defaultVal,
                     const juce::String& suffix = {});
};
