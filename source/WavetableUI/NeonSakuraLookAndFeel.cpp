#include "NeonSakuraLookAndFeel.h"

NeonSakuraLookAndFeel::NeonSakuraLookAndFeel()
{
    // Generelle UI-Einstellungen für das gesamte Plugin
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    setColour(juce::ComboBox::backgroundColourId, getPanelBackground());
    setColour(juce::ComboBox::textColourId, getNeonCyan());
    setColour(juce::ComboBox::outlineColourId, getNeonPurple());
    setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    setColour(juce::TextButton::textColourOnId, getNeonPink());
    setColour(juce::TextButton::textColourOffId, getNeonPink());
}

void NeonSakuraLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, const float rotaryStartAngle,
                                             const float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float)x + (float)width  * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Hintergrund-Spur (dunkel)
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    g.strokePath(backgroundArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Farbe aus dem Slider auslesen (z.B. NeonPink, NeonCyan)
    auto fillColour = slider.findColour(juce::Slider::thumbColourId, true);
    if (fillColour.isTransparent()) fillColour = getNeonCyan(); // Fallback

    if (slider.isEnabled())
    {
        // Glow-Effekt (breiter, halb-transparent)
        juce::Path glowArc;
        glowArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(fillColour.withAlpha(0.3f));
        g.strokePath(glowArc, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Innerer heller Ring (der eigentliche Wert)
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(fillColour);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Leuchtender Punkt als Zeiger
    auto pointerRadius = radius - 7.0f;
    juce::Path pointer;
    pointer.addEllipse(centreX + pointerRadius * std::cos(angle - juce::MathConstants<float>::halfPi) - 3.0f,
                       centreY + pointerRadius * std::sin(angle - juce::MathConstants<float>::halfPi) - 3.0f,
                       6.0f, 6.0f);
    g.setColour(juce::Colours::white);
    g.fillPath(pointer);
}

void NeonSakuraLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float minSliderPos, float maxSliderPos,
                                             const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    auto trackColour = slider.findColour(juce::Slider::trackColourId);
    auto thumbColour = slider.findColour(juce::Slider::thumbColourId);
    if (trackColour.isTransparent()) trackColour = getNeonGreen().withAlpha(0.3f);
    if (thumbColour.isTransparent()) thumbColour = getNeonGreen();

    if (slider.isHorizontal())
    {
        g.setColour(trackColour);
        g.fillRoundedRectangle((float)x, (float)y + (float)height * 0.5f - 2.0f, (float)width, 4.0f, 2.0f);
        
        g.setColour(thumbColour);
        g.fillRoundedRectangle((float)x, (float)y + (float)height * 0.5f - 2.0f, sliderPos - (float)x, 4.0f, 2.0f);
        
        g.setColour(juce::Colours::white);
        g.fillEllipse(sliderPos - 6.0f, (float)y + (float)height * 0.5f - 6.0f, 12.0f, 12.0f);
    }
    else // Vertikal (z.B. Envelopes)
    {
        g.setColour(trackColour);
        g.fillRoundedRectangle((float)x + (float)width * 0.5f - 2.0f, (float)y, 4.0f, (float)height, 2.0f);
        
        g.setColour(thumbColour);
        g.fillRoundedRectangle((float)x + (float)width * 0.5f - 2.0f, sliderPos, 4.0f, (float)y + (float)height - sliderPos, 2.0f);
        
        // Glow für den Thumb
        g.setColour(thumbColour.withAlpha(0.4f));
        g.fillEllipse((float)x + (float)width * 0.5f - 10.0f, sliderPos - 10.0f, 20.0f, 20.0f);
        
        // Weißer Kern
        g.setColour(juce::Colours::white);
        g.fillEllipse((float)x + (float)width * 0.5f - 6.0f, sliderPos - 6.0f, 12.0f, 12.0f);
    }
}