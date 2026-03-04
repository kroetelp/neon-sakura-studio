#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/Waveshaper.h"
#include "../Theme/ThemeManager.h"
#include <memory>

class WavetableParams;

/**
 * ShaperSection - Controls for the waveshaper/distortion effect
 *
 * Provides UI for:
 * - Mode selection (Off, SoftClip, HardClip, Foldback, Bitcrush, Rectify, Saturate)
 * - Amount control (intensity of the effect)
 * - Mix control (dry/wet blend)
 */
class ShaperSection : public juce::Component
{
public:
    ShaperSection();
    ~ShaperSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void connectToSharedParams(std::shared_ptr<WavetableParams> params);

    // Update UI from parameter values
    void updateFromParams(int mode, float amount, float mix);

private:
    // Controls
    juce::ComboBox modeCombo;
    juce::Slider amountSlider;
    juce::Slider mixSlider;

    // Labels
    juce::Label titleLabel;
    juce::Label modeLabel;
    juce::Label amountLabel;
    juce::Label mixLabel;

    // Colors - Magenta/Pink for shaper (delegated to ThemeManager)
    static juce::Colour getNeonPink() { return ThemeManager::getInstance().getAccentColor(); }
    static juce::Colour getNeonMagenta() { return ThemeManager::getInstance().getAccentColor().withHue(0.9f); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }
};
