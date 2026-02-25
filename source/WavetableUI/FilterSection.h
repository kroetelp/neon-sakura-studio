#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableVoice.h"
#include "../WavetableSynth/WavetableFilter.h"
#include <memory>

class WavetableParams;

/**
 * FilterSection - Controls for the multi-mode filter
 */
class FilterSection : public juce::Component
{
public:
    FilterSection();
    ~FilterSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void connectToParams(WavetableVoice::VoiceParams& params);
    void connectToSharedParams(std::shared_ptr<WavetableParams> params);

    // Update UI from parameter values
    void updateFromParams(float cutoff, float resonance, float drive, int mode);

private:
    // Controls
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider driveSlider;
    juce::ComboBox modeCombo;

    // Labels
    juce::Label titleLabel;
    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label driveLabel;
    juce::Label modeLabel;

    // Colors
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getNeonOrange() { return juce::Colour(255, 165, 0); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }
};
