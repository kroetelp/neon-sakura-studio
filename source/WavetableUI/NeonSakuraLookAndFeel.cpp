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
    // === Berechnungen ===
    const float boundsScale = 0.9f;
    auto diameter = (float)juce::jmin(width, height) * boundsScale;
    auto radius = diameter * 0.5f;
    auto centreX = (float)x + (float)width  * 0.5f;
    auto centreY = (float)y + (float)height * 0.5f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Hover-State erkennen
    const bool isHover = slider.isMouseOver() && slider.isEnabled();
    const bool isDragging = slider.isMouseButtonDown();

    // Farbe aus dem Slider auslesen (z.B. NeonPink, NeonCyan)
    auto accentColour = slider.findColour(juce::Slider::thumbColourId, true);
    if (accentColour.isTransparent())
        accentColour = getNeonCyan();

    // === 1. Äußerer Glow-Ring (nur bei Hover/Drag) ===
    if (isHover || isDragging)
    {
        float glowIntensity = isDragging ? 0.5f : 0.3f;
        float glowRadius = radius + 8.0f;

        juce::Path outerGlow;
        outerGlow.addCentredArc(centreX, centreY, glowRadius, glowRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);

        // Mehrstufiger Glow für weichen Übergang
        for (float t = 20.0f; t > 2.0f; t -= 4.0f)
        {
            g.setColour(accentColour.withAlpha(glowIntensity * (1.0f - t / 20.0f)));
            g.strokePath(outerGlow, juce::PathStrokeType(t,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    // === 2. Dunkler Körper (Flat Design) ===
    float bodyRadius = radius * 0.85f;
    juce::Path knobBody;
    knobBody.addEllipse(centreX - bodyRadius, centreY - bodyRadius,
                        bodyRadius * 2.0f, bodyRadius * 2.0f);

    // Sanfter Farbverlauf für Tiefe
    juce::ColourGradient bodyGradient(
        getDarkBackground().brighter(isHover ? 0.08f : 0.03f),
        centreX - bodyRadius * 0.3f, centreY - bodyRadius * 0.3f,
        getDarkBackground().darker(0.1f),
        centreX + bodyRadius * 0.5f, centreY + bodyRadius * 0.5f,
        true
    );
    g.setGradientFill(bodyGradient);
    g.fillPath(knobBody);

    // Subtiler Rand
    g.setColour(juce::Colours::white.withAlpha(isHover ? 0.15f : 0.05f));
    g.strokePath(knobBody, juce::PathStrokeType(1.0f));

    // === 3. Hintergrund-Arc (inaktive Spur) ===
    float arcRadius = radius * 0.92f;
    float arcThickness = 3.5f;

    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.25f));
    g.strokePath(backgroundArc, juce::PathStrokeType(arcThickness,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // === 4. Wert-Bogen (Neon-Leuchtspur) ===
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                               0.0f, rotaryStartAngle, angle, true);

        // Neon-Glow unter dem Wert-Bogen
        float glowAlpha = isDragging ? 0.5f : (isHover ? 0.35f : 0.25f);
        for (float t = 14.0f; t > arcThickness; t -= 2.5f)
        {
            g.setColour(accentColour.withAlpha(glowAlpha * (1.0f - t / 14.0f)));
            g.strokePath(valueArc, juce::PathStrokeType(t,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Solider Kern des Wert-Bogens mit Farbverlauf
        juce::ColourGradient arcGradient(
            accentColour.brighter(isDragging ? 0.6f : 0.3f),
            centreX - arcRadius, centreY,
            accentColour.darker(0.1f),
            centreX + arcRadius, centreY,
            false
        );
        g.setGradientFill(arcGradient);
        g.strokePath(valueArc, juce::PathStrokeType(arcThickness,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // === 5. Zeiger-Punkt auf dem Bogen ===
    float pointerRadius = arcRadius;
    float pointerX = centreX + pointerRadius * std::cos(angle - juce::MathConstants<float>::halfPi);
    float pointerY = centreY + pointerRadius * std::sin(angle - juce::MathConstants<float>::halfPi);

    // Glow unter dem Punkt
    float pointerGlowSize = isDragging ? 14.0f : (isHover ? 12.0f : 10.0f);
    g.setColour(accentColour.withAlpha(isDragging ? 0.7f : 0.5f));
    g.fillEllipse(pointerX - pointerGlowSize * 0.5f, pointerY - pointerGlowSize * 0.5f,
                  pointerGlowSize, pointerGlowSize);

    // Weißer Kern
    float pointerCoreSize = isDragging ? 7.0f : (isHover ? 6.0f : 5.0f);
    g.setColour(juce::Colours::white.withAlpha(isDragging ? 1.0f : 0.95f));
    g.fillEllipse(pointerX - pointerCoreSize * 0.5f, pointerY - pointerCoreSize * 0.5f,
                  pointerCoreSize, pointerCoreSize);

    // === 6. Zentrale Wert-Anzeige (optional, für große Knobs) ===
    if (diameter > 60.0f && slider.isEnabled())
    {
        // Kleiner innerer Punkt für visuellen Fokus
        float innerDotSize = 4.0f;
        g.setColour(accentColour.withAlpha(isHover ? 0.4f : 0.2f));
        g.fillEllipse(centreX - innerDotSize * 0.5f, centreY - innerDotSize * 0.5f,
                      innerDotSize, innerDotSize);
    }
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