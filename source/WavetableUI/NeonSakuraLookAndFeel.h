#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class NeonSakuraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonSakuraLookAndFeel();

    // Zentrale Farbpalette
    static juce::Colour getNeonPink()   { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan()   { return juce::Colour(0, 255, 255); }
    static juce::Colour getNeonPurple() { return juce::Colour(180, 0, 255); }
    static juce::Colour getNeonGreen()  { return juce::Colour(0, 255, 128); }
    static juce::Colour getNeonOrange() { return juce::Colour(255, 165, 0); }
    static juce::Colour getDarkBackground() { return juce::Colour(15, 15, 25); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

    // Überschreiben der Standard-JUCE-Elemente
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override;
};