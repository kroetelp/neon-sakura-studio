// ============================================================================
// PanelGlassEffect.cpp - Implementierung
// ============================================================================

#include "PanelGlassEffect.h"
#include "../Theme/ThemeManager.h"
#include <algorithm>

//==============================================================================
// Konstruktor
//==============================================================================
PanelGlassEffect::PanelGlassEffect()
{
    applyMediumPreset();
}

PanelGlassEffect::PanelGlassEffect(GlassStyle style)
    : glassStyle(style)
{
    switch (style)
    {
        case GlassStyle::Subtle:
            applySubtlePreset();
            break;
        case GlassStyle::Medium:
            applyMediumPreset();
            break;
        case GlassStyle::Heavy:
            applyHeavyPreset();
            break;
        case GlassStyle::Frosted:
            applyFrostedPreset();
            break;
        default:
            applyMediumPreset();
            break;
    }
}

//==============================================================================
// Destruktor
//==============================================================================
PanelGlassEffect::~PanelGlassEffect()
{
}

//==============================================================================
// Glass Style
//==============================================================================
void PanelGlassEffect::setGlassStyle(GlassStyle style)
{
    glassStyle = style;
}

//==============================================================================
// Blur Configuration
//==============================================================================
void PanelGlassEffect::setBlurRadius(float radius)
{
    blurRadius = juce::jlimit(0.0f, 50.0f, radius);
}

void PanelGlassEffect::setBlurEnabled(bool enabled)
{
    blurEnabled = enabled;
}

//==============================================================================
// Border Configuration
//==============================================================================
void PanelGlassEffect::setBorderType(GlassBorderType type)
{
    borderType = type;
}

void PanelGlassEffect::setBorderWidth(float width)
{
    borderWidth = juce::jmax(0.0f, width);
}

void PanelGlassEffect::setBorderColour(const juce::Colour& colour)
{
    borderColour = colour;
}

//==============================================================================
// Corner Radius
//==============================================================================
void PanelGlassEffect::setCornerRadius(float radius)
{
    cornerRadius = juce::jmax(0.0f, radius);
}

//==============================================================================
// Opacity / Transparency
//==============================================================================
void PanelGlassEffect::setOpacity(float opacity)
{
    this->opacity = juce::jlimit(0.0f, 1.0f, opacity);
}

void PanelGlassEffect::setBackgroundTransparent(bool transparent)
{
    backgroundTransparent = transparent;
}

//==============================================================================
// Shadow Configuration
//==============================================================================
void PanelGlassEffect::setShadowEnabled(bool enabled)
{
    shadowEnabled = enabled;
}

void PanelGlassEffect::setShadowColour(const juce::Colour& colour)
{
    shadowColour = colour;
}

void PanelGlassEffect::setShadowOffset(const juce::Point<int>& offset)
{
    shadowOffset = offset;
}

void PanelGlassEffect::setShadowOffset(int x, int y)
{
    shadowOffset = juce::Point<int>(x, y);
}

void PanelGlassEffect::setShadowSpread(float spread)
{
    shadowSpread = juce::jmax(0.0f, spread);
}

//==============================================================================
// Presets
//==============================================================================
void PanelGlassEffect::applySubtlePreset()
{
    glassStyle = GlassStyle::Subtle;
    blurRadius = 4.0f;
    opacity = 0.9f;
    cornerRadius = 6.0f;
    borderWidth = 0.5f;
    borderType = GlassBorderType::Gradient;
    borderColour = juce::Colour(100, 100, 120);
    shadowEnabled = true;
    shadowOffset = juce::Point<int>(0, 1);
    shadowSpread = 3.0f;
}

void PanelGlassEffect::applyMediumPreset()
{
    glassStyle = GlassStyle::Medium;
    blurRadius = 8.0f;
    opacity = 0.85f;
    cornerRadius = 8.0f;
    borderWidth = 1.0f;
    borderType = GlassBorderType::Solid;
    borderColour = juce::Colour(100, 100, 120);
    shadowEnabled = true;
    shadowOffset = juce::Point<int>(0, 2);
    shadowSpread = 4.0f;
}

void PanelGlassEffect::applyHeavyPreset()
{
    glassStyle = GlassStyle::Heavy;
    blurRadius = 15.0f;
    opacity = 0.8f;
    cornerRadius = 12.0f;
    borderWidth = 1.5f;
    borderType = GlassBorderType::Glow;
    borderColour = juce::Colour(120, 140, 180);
    shadowEnabled = true;
    shadowOffset = juce::Point<int>(0, 4);
    shadowSpread = 8.0f;
}

void PanelGlassEffect::applyFrostedPreset()
{
    glassStyle = GlassStyle::Frosted;
    blurRadius = 12.0f;
    opacity = 0.75f;
    cornerRadius = 10.0f;
    borderWidth = 1.0f;
    borderType = GlassBorderType::Solid;
    borderColour = juce::Colour(130, 140, 160);
    shadowEnabled = true;
    shadowOffset = juce::Point<int>(0, 3);
    shadowSpread = 6.0f;
}

//==============================================================================
// Drawing
//==============================================================================
void PanelGlassEffect::drawGlassEffect(juce::Graphics& g, const juce::Rectangle<int>& bounds) const
{
    drawGlassEffect(g, bounds, juce::Colour(static_cast<juce::uint8>(40), static_cast<juce::uint8>(40), static_cast<juce::uint8>(50), static_cast<juce::uint8>(200)));
}

void PanelGlassEffect::drawGlassEffect(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const
{
    if (glassStyle == GlassStyle::None || bounds.isEmpty())
        return;

    auto rect = bounds.toFloat();

    // Shadow zuerst zeichnen
    if (shadowEnabled)
    {
        drawShadow(g, bounds);
    }

    // Hintergrund zeichnen
    if (!backgroundTransparent)
    {
        juce::Path glassPath = createGlassPath(bounds);

        juce::Colour actualBgColour = bgColour.withAlpha(opacity);

        // Gradient für Glass-Look
        juce::ColourGradient gradient(
            actualBgColour.brighter(0.1f),
            rect.getTopLeft(),
            actualBgColour.darker(0.15f),
            rect.getBottomRight(),
            true
        );
        g.setGradientFill(gradient);
        g.fillPath(glassPath);
    }

    // Border/Glow zeichnen
    if (borderType != GlassBorderType::None)
    {
        if (borderType == GlassBorderType::Glow)
        {
            drawGlow(g, bounds);
        }
        else
        {
            drawBorder(g, bounds);
        }
    }

    // Inner Glow für Highlight-Effekt
    if (!backgroundTransparent)
    {
        juce::Rectangle<float> innerRect = rect.reduced(borderWidth + 1.0f);
        juce::Path glowPath;
        glowPath.addRoundedRectangle(innerRect, cornerRadius - borderWidth - 1.0f);

        juce::ColourGradient innerGlow(
            juce::Colours::white.withAlpha(0.05f),
            rect.getCentreX(), rect.getCentreY(),
            juce::Colours::transparentBlack,
            rect.getCentreX(), rect.getBottom(),
            true
        );
        g.setGradientFill(innerGlow);
        g.fillPath(glowPath);
    }
}

juce::Path PanelGlassEffect::createGlassPath(const juce::Rectangle<int>& bounds) const
{
    juce::Path path;
    auto rect = bounds.toFloat();
    path.addRoundedRectangle(rect, cornerRadius);
    return path;
}

//==============================================================================
// Utility
//==============================================================================
PanelGlassEffect::GlassPresetValues PanelGlassEffect::getPresetValues(GlassStyle style)
{
    GlassPresetValues values;

    switch (style)
    {
        case GlassStyle::Subtle:
            values.blurRadius = 4.0f;
            values.opacity = 0.9f;
            values.cornerRadius = 6.0f;
            values.borderWidth = 0.5f;
            values.borderType = GlassBorderType::Gradient;
            values.borderColour = juce::Colour(100, 100, 120);
            break;

        case GlassStyle::Medium:
            values.blurRadius = 8.0f;
            values.opacity = 0.85f;
            values.cornerRadius = 8.0f;
            values.borderWidth = 1.0f;
            values.borderType = GlassBorderType::Solid;
            values.borderColour = juce::Colour(100, 100, 120);
            break;

        case GlassStyle::Heavy:
            values.blurRadius = 15.0f;
            values.opacity = 0.8f;
            values.cornerRadius = 12.0f;
            values.borderWidth = 1.5f;
            values.borderType = GlassBorderType::Glow;
            values.borderColour = juce::Colour(120, 140, 180);
            break;

        case GlassStyle::Frosted:
            values.blurRadius = 12.0f;
            values.opacity = 0.75f;
            values.cornerRadius = 10.0f;
            values.borderWidth = 1.0f;
            values.borderType = GlassBorderType::Solid;
            values.borderColour = juce::Colour(130, 140, 160);
            break;

        default:
            values.blurRadius = 8.0f;
            values.opacity = 0.85f;
            values.cornerRadius = 8.0f;
            values.borderWidth = 1.0f;
            values.borderType = GlassBorderType::Solid;
            values.borderColour = juce::Colour(100, 100, 120);
            break;
    }

    return values;
}

//==============================================================================
// Private Methods
//==============================================================================
void PanelGlassEffect::drawSubtleGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const
{
    auto rect = bounds.toFloat();
    juce::Path glassPath = createGlassPath(bounds);

    // Sanfter Gradient
    juce::ColourGradient gradient(
        bgColour.brighter(0.05f),
        rect.getTopLeft(),
        bgColour.darker(0.1f),
        rect.getBottomRight(),
        true
    );
    g.setGradientFill(gradient);
    g.fillPath(glassPath);

    // Dünner Gradient Border
    juce::ColourGradient borderGradient(
        juce::Colour(120, 130, 150),
        rect.getTopLeft(),
        juce::Colour(80, 90, 110),
        rect.getBottomRight(),
        true
    );
    g.setGradientFill(borderGradient);
    g.strokePath(glassPath, juce::PathStrokeType(0.5f));
}

void PanelGlassEffect::drawMediumGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const
{
    auto rect = bounds.toFloat();
    juce::Path glassPath = createGlassPath(bounds);

    // Mittlerer Gradient
    juce::ColourGradient gradient(
        bgColour.brighter(0.1f),
        rect.getTopLeft(),
        bgColour.darker(0.15f),
        rect.getBottomRight(),
        true
    );
    g.setGradientFill(gradient);
    g.fillPath(glassPath);

    // Solider Border
    g.setColour(borderColour);
    g.strokePath(glassPath, juce::PathStrokeType(borderWidth));
}

void PanelGlassEffect::drawHeavyGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const
{
    auto rect = bounds.toFloat();
    juce::Path glassPath = createGlassPath(bounds);

    // Starker Gradient
    juce::ColourGradient gradient(
        bgColour.brighter(0.15f),
        rect.getTopLeft(),
        bgColour.darker(0.2f),
        rect.getBottomRight(),
        true
    );
    g.setGradientFill(gradient);
    g.fillPath(glassPath);

    // Glow statt Border
    drawGlow(g, bounds);
}

void PanelGlassEffect::drawFrostedGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const
{
    auto rect = bounds.toFloat();
    juce::Path glassPath = createGlassPath(bounds);

    // Frost-Gradient
    juce::ColourGradient gradient(
        bgColour.brighter(0.08f),
        rect.getTopLeft(),
        bgColour.darker(0.18f),
        rect.getBottomRight(),
        false
    );
    g.setGradientFill(gradient);
    g.fillPath(glassPath);

    // Texturierter Border
    g.setColour(borderColour);
    g.strokePath(glassPath, juce::PathStrokeType(borderWidth));

    // Subtile Textur-Elemente
    g.setColour(juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(255), static_cast<juce::uint8>(255), static_cast<juce::uint8>(10)));
    for (int i = 0; i < 3; ++i)
    {
        auto lineRect = rect.reduced(5.0f + i * 8.0f);
        g.drawLine(
            juce::Line<float>(lineRect.getTopLeft(), lineRect.getTopRight()),
            0.5f
        );
    }
}

void PanelGlassEffect::drawShadow(juce::Graphics& g, const juce::Rectangle<int>& bounds) const
{
    auto rect = bounds.toFloat();

    // Shadow mittels JUCE DropShadow
    juce::DropShadow dropShadow(
        shadowColour,
        shadowSpread,
        shadowOffset
    );

    juce::Path shadowPath = createGlassPath(bounds);
    dropShadow.drawForPath(g, shadowPath);
}

void PanelGlassEffect::drawBorder(juce::Graphics& g, const juce::Rectangle<int>& bounds) const
{
    auto rect = bounds.toFloat();
    juce::Path glassPath = createGlassPath(bounds);

    if (borderType == GlassBorderType::Gradient)
    {
        juce::ColourGradient gradient = createGradientForBounds(bounds, borderColour);
        g.setGradientFill(gradient);
        g.strokePath(glassPath, juce::PathStrokeType(borderWidth));
    }
    else // Solid
    {
        g.setColour(borderColour);
        g.strokePath(glassPath, juce::PathStrokeType(borderWidth));
    }
}

void PanelGlassEffect::drawGlow(juce::Graphics& g, const juce::Rectangle<int>& bounds) const
{
    auto rect = bounds.toFloat();

    // Glow-Effekt
    juce::ColourGradient glowGradient(
        borderColour.withAlpha(0.6f),
        rect.getCentreX(), rect.getCentreY(),
        borderColour.withAlpha(0.0f),
        rect.getCentreX(), rect.getCentreY() + std::min(rect.getWidth(), rect.getHeight()) * 0.5f,
        true
    );

    g.setGradientFill(glowGradient);
    g.fillPath(createGlassPath(bounds));
}

juce::ColourGradient PanelGlassEffect::createGradientForBounds(const juce::Rectangle<int>& bounds, const juce::Colour& colour) const
{
    auto rect = bounds.toFloat();
    return juce::ColourGradient(
        colour.brighter(0.3f),
        rect.getTopLeft(),
        colour.darker(0.2f),
        rect.getBottomRight(),
        true
    );
}
