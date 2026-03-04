#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"
#include <memory>

class WavetableParams;

/**
 * ModulationSection - Controls for FM/AM modulation between oscillators
 *
 * Routing:
 * - FM12: OSC1 -> OSC2 (Frequency Modulation)
 * - FM13: OSC1 -> OSC3 (Frequency Modulation)
 * - FM23: OSC2 -> OSC3 (Frequency Modulation)
 * - AM12: OSC1 -> OSC2 (Amplitude Modulation)
 * - AM13: OSC1 -> OSC3 (Amplitude Modulation)
 * - AM23: OSC2 -> OSC3 (Amplitude Modulation)
 */
class ModulationSection : public juce::Component
{
public:
    ModulationSection();
    ~ModulationSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void connectToSharedParams(std::shared_ptr<WavetableParams> params);

    // Update UI from parameter values
    void updateFromParams(float fm12, float fm13, float fm23,
                          float am12, float am13, float am23);

private:
    // FM sliders
    juce::Slider fm12Slider;
    juce::Slider fm13Slider;
    juce::Slider fm23Slider;

    // AM sliders
    juce::Slider am12Slider;
    juce::Slider am13Slider;
    juce::Slider am23Slider;

    // Labels
    juce::Label titleLabel;
    juce::Label fmLabel;
    juce::Label amLabel;
    juce::Label fm12Label;
    juce::Label fm13Label;
    juce::Label fm23Label;
    juce::Label am12Label;
    juce::Label am13Label;
    juce::Label am23Label;

    // Routing diagram labels
    juce::Label routingLabel;

    // Colors - Purple/Violet for modulation (delegated to ThemeManager)
    static juce::Colour getNeonViolet() { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    static juce::Colour getNeonPurple() { return ThemeManager::getInstance().getAccentColor().withHue(0.75f); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }
};
