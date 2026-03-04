#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableVoice.h"
#include "../WavetableSynth/WavetableFilter.h"
#include "../Theme/ThemeManager.h"
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

    // Colors (delegated to ThemeManager)
    static juce::Colour getNeonCyan() { return ThemeManager::getInstance().getInfoColor(); }
    static juce::Colour getNeonOrange() { return ThemeManager::getInstance().getWarningColor(); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }
};