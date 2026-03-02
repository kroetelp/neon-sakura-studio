#pragma once

/**
 * WootingSettingsPanel - UI for configuring Wooting analog keyboard settings
 *
 * Provides controls for:
 * - Velocity curve selection (Linear, Soft, Hard)
 * - Pressure (Aftertouch) curve selection
 * - Octave offset (-2 to +2)
 * - Connection status display
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WootingManager.h"

class WootingSettingsPanel : public juce::Component,
                              private juce::Timer
{
public:
    explicit WootingSettingsPanel(WootingManager& manager);
    ~WootingSettingsPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    WootingManager& wootingManager;

    juce::Label titleLabel;
    juce::Label statusLabel;

    juce::ComboBox velocityCurveComboBox;
    juce::Label velocityCurveLabel;

    juce::ComboBox pressureCurveComboBox;
    juce::Label pressureCurveLabel;

    juce::Slider octaveSlider;
    juce::Label octaveLabel;

    juce::TextButton testButton;
    juce::Label activeKeysLabel;

    // Colors
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonGreen() const { return juce::Colour(0, 255, 100); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WootingSettingsPanel)
};
