#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"

/**
 * ProfessionalLookAndFeel - Professionelles LookAndFeel im FL Studio/Ableton Stil
 *
 * Keine Neon-Glow-Effekte, sondern subtles, professionelles Design.
 * Unterstützt die Color Schemes: DarkOrange, DarkBlue, DarkGray
 */
class ProfessionalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ProfessionalLookAndFeel();
    ~ProfessionalLookAndFeel() override = default;

    // ========================================================================
    // Slider Drawing
    // ========================================================================

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // ========================================================================
    // Button Drawing
    // ========================================================================

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawTickBox(juce::Graphics& g, juce::Component& component,
                     float x, float y, float w, float h,
                     bool ticked, bool isEnabled,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override;

    // ========================================================================
    // ComboBox Drawing
    // ========================================================================

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;

    // ========================================================================
    // Label Drawing
    // ========================================================================

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // ========================================================================
    // Text Editor Drawing
    // ========================================================================

    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                   juce::TextEditor& textEditor) override;

    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;

    // ========================================================================
    // Scrollbar Drawing
    // ========================================================================

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbPosition,
                       int thumbSize, bool isMouseOver, bool isMouseDown) override;

    // ========================================================================
    // Document Window Drawing
    // ========================================================================

    void drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                     juce::Graphics& g, int w, int h,
                                     int titleSpaceX, int titleSpaceW,
                                     const juce::Image* icon,
                                     bool drawTitleTextOnLeft) override;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    /** Get current theme manager colors */
    ThemeManager& getThemeManager() const { return ThemeManager::getInstance(); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfessionalLookAndFeel)
};
