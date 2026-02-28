#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/Waveshaper.h"
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

    // Colors - Magenta/Pink for shaper to distinguish from filter (cyan)
    static juce::Colour getNeonPink() { return juce::Colour(255, 0, 255); }
    static juce::Colour getNeonMagenta() { return juce::Colour(255, 50, 150); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }
};
