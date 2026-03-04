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
#include "../Theme/ThemeManager.h"

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

    // Colors (delegated to ThemeManager)
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonGreen() const { return ThemeManager::getInstance().getSuccessColor(); }
    juce::Colour getNeonPurple() const { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WootingSettingsPanel)
};
