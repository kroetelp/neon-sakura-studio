// ============================================================================
// ExtendedThemeSystem.cpp - Implementierung
// ============================================================================

#include "ExtendedThemeSystem.h"
#include "../UI/FloatingPanelEnums.h"
#include <juce_core/juce_core.h>
#include <algorithm>

//==============================================================================
// Konstruktor
//==============================================================================
ExtendedThemeSystem::ExtendedThemeSystem()
{
    initializeDefaultColours();
    applyGlassUITheme();
}

//==============================================================================
// Destruktor
//==============================================================================
ExtendedThemeSystem::~ExtendedThemeSystem()
{
}

//==============================================================================
// Theme Management
//==============================================================================
void ExtendedThemeSystem::setThemeType(ExtendedThemeType themeType)
{
    if (currentThemeType == themeType)
        return;

    ExtendedThemeType oldThemeType = currentThemeType;
    currentThemeType = themeType;

    applyThemeColours(themeType);

    // Trigger Callback
    triggerThemeChangeCallback(themeType);
}

juce::String ExtendedThemeSystem::getThemeTypeName(ExtendedThemeType themeType)
{
    switch (themeType)
    {
        case ExtendedThemeType::GlassUI:
            return "Glass UI";
        case ExtendedThemeType::Cyberpunk:
            return "Cyberpunk";
        case ExtendedThemeType::Minimal:
            return "Minimal";
        case ExtendedThemeType::Liquid:
            return "Liquid";
        case ExtendedThemeType::NeonSakura:
            return "Neon Sakura";
        case ExtendedThemeType::Professional:
            return "Professional";
        default:
            return "Unknown";
    }
}

ExtendedThemeType ExtendedThemeSystem::getThemeTypeFromName(const juce::String& name)
{
    if (name == "Glass UI")
        return ExtendedThemeType::GlassUI;
    if (name == "Cyberpunk")
        return ExtendedThemeType::Cyberpunk;
    if (name == "Minimal")
        return ExtendedThemeType::Minimal;
    if (name == "Liquid")
        return ExtendedThemeType::Liquid;
    if (name == "Neon Sakura")
        return ExtendedThemeType::NeonSakura;
    if (name == "Professional")
        return ExtendedThemeType::Professional;
    return ExtendedThemeType::GlassUI;
}

//==============================================================================
// Farbps Management
//==============================================================================
void ExtendedThemeSystem::setColour(ThemeColourRole role, const juce::Colour& colour)
{
    colourMap[role] = colour;
}

juce::Colour ExtendedThemeSystem::getColour(ThemeColourRole role) const
{
    auto it = colourMap.find(role);
    if (it != colourMap.end())
        return it->second;
    return juce::Colours::transparentBlack;
}

juce::Colour ExtendedThemeSystem::getColourWithOpacity(ThemeColourRole role, float opacity) const
{
    return getColour(role).withAlpha(juce::jlimit(0.0f, 1.0f, opacity));
}

juce::ColourGradient ExtendedThemeSystem::getGradient(ThemeColourRole role, const juce::Rectangle<int>& bounds, bool vertical) const
{
    auto it = gradientMap.find(role);
    if (it != gradientMap.end())
        return it->second;

    // Erzeuge einen Gradient aus der Farbe
    juce::Colour colour = getColour(role);
    juce::Colour darkColour = colour.darker(0.3f);

    if (vertical)
    {
        return juce::ColourGradient(
            colour,
            static_cast<float>(bounds.getX()),
            static_cast<float>(bounds.getY()),
            darkColour,
            static_cast<float>(bounds.getX()),
            static_cast<float>(bounds.getBottom()),
            false
        );
    }
    else
    {
        return juce::ColourGradient(
            colour,
            static_cast<float>(bounds.getX()),
            static_cast<float>(bounds.getY()),
            darkColour,
            static_cast<float>(bounds.getRight()),
            static_cast<float>(bounds.getY()),
            false
        );
    }
}

void ExtendedThemeSystem::setGradient(ThemeColourRole role, const juce::ColourGradient& gradient)
{
    gradientMap[role] = gradient;
}

//==============================================================================
// Theme Metrics
//==============================================================================
void ExtendedThemeSystem::setMetrics(const ThemeMetrics& newMetrics)
{
    metrics = newMetrics;
}

float ExtendedThemeSystem::getCornerRadius(ThemeMetrics::CornerSize size) const
{
    switch (size)
    {
        case ThemeMetrics::Tiny:
            return metrics.cornerRadiusSmall * 0.5f;
        case ThemeMetrics::Small:
            return metrics.cornerRadiusSmall;
        case ThemeMetrics::Medium:
            return metrics.cornerRadiusMedium;
        case ThemeMetrics::Large:
            return metrics.cornerRadiusLarge;
        case ThemeMetrics::XL:
            return metrics.cornerRadiusXL;
        default:
            return metrics.cornerRadiusMedium;
    }
}

float ExtendedThemeSystem::getPadding(ThemeMetrics::PaddingSize size) const
{
    switch (size)
    {
        case ThemeMetrics::PTiny:
            return metrics.paddingTiny;
        case ThemeMetrics::PSmall:
            return metrics.paddingSmall;
        case ThemeMetrics::PMedium:
            return metrics.paddingMedium;
        case ThemeMetrics::PLarge:
            return metrics.paddingLarge;
        case ThemeMetrics::PXL:
            return metrics.paddingXL;
        default:
            return metrics.paddingMedium;
    }
}

float ExtendedThemeSystem::getSpacing(ThemeMetrics::SpacingSize size) const
{
    switch (size)
    {
        case ThemeMetrics::STiny:
            return metrics.spacingTiny;
        case ThemeMetrics::SSmall:
            return metrics.spacingSmall;
        case ThemeMetrics::SMedium:
            return metrics.spacingMedium;
        case ThemeMetrics::SLarge:
            return metrics.spacingLarge;
        case ThemeMetrics::SXL:
            return metrics.spacingXL;
        default:
            return metrics.spacingMedium;
    }
}

//==============================================================================
// Preset Themes
//==============================================================================
void ExtendedThemeSystem::applyGlassUITheme()
{
    // Glass UI Theme - Moderner Glass-Morphismus
    colourMap[ThemeColourRole::Background] = juce::Colour(20, 20, 25);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(15, 15, 20);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(35, 35, 45);

    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(static_cast<juce::uint8>(40), static_cast<juce::uint8>(40), static_cast<juce::uint8>(50), static_cast<juce::uint8>(200));
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(80, 80, 90);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(100, 100, 120);

    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(240, 240, 245);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(180, 180, 190);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(100, 100, 110);

    colourMap[ThemeColourRole::AccentPrimary] = juce::Colour(0, 180, 255);
    colourMap[ThemeColourRole::AccentSecondary] = juce::Colour(150, 80, 255);
    colourMap[ThemeColourRole::AccentSuccess] = juce::Colour(80, 200, 100);
    colourMap[ThemeColourRole::AccentWarning] = juce::Colour(255, 180, 50);
    colourMap[ThemeColourRole::AccentError] = juce::Colour(255, 80, 80);

    colourMap[ThemeColourRole::ButtonNormal] = juce::Colour(60, 60, 75);
    colourMap[ThemeColourRole::ButtonHover] = juce::Colour(80, 80, 95);
    colourMap[ThemeColourRole::ButtonPressed] = juce::Colour(50, 50, 65);
    colourMap[ThemeColourRole::ButtonDisabled] = juce::Colour(static_cast<juce::uint8>(40), static_cast<juce::uint8>(40), static_cast<juce::uint8>(50), static_cast<juce::uint8>(128));

    colourMap[ThemeColourRole::SliderTrack] = juce::Colour(50, 50, 60);
    colourMap[ThemeColourRole::SliderThumb] = juce::Colour(100, 180, 255);
    colourMap[ThemeColourRole::KnobBackground] = juce::Colour(45, 45, 55);
    colourMap[ThemeColourRole::KnobForeground] = juce::Colour(80, 160, 255);

    colourMap[ThemeColourRole::GlassTint] = juce::Colour(static_cast<juce::uint8>(60), static_cast<juce::uint8>(60), static_cast<juce::uint8>(75), static_cast<juce::uint8>(100));
    colourMap[ThemeColourRole::GlassBorder] = juce::Colour(static_cast<juce::uint8>(100), static_cast<juce::uint8>(100), static_cast<juce::uint8>(120), static_cast<juce::uint8>(150));
    colourMap[ThemeColourRole::Glow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(180), static_cast<juce::uint8>(255), static_cast<juce::uint8>(50));
    colourMap[ThemeColourRole::Shadow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(150));

    // Glass-spezifische Metriken
    metrics.cornerRadiusMedium = 12.0f;
    metrics.cornerRadiusLarge = 16.0f;
    metrics.shadowBlur = 15.0f;
}

void ExtendedThemeSystem::applyCyberpunkTheme()
{
    // Cyberpunk Theme - Neon und harte Kanten
    colourMap[ThemeColourRole::Background] = juce::Colour(5, 5, 10);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(0, 0, 5);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(20, 20, 30);

    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(static_cast<juce::uint8>(15), static_cast<juce::uint8>(15), static_cast<juce::uint8>(25), static_cast<juce::uint8>(220));
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(255, 0, 128);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(0, 255, 255);

    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(255, 255, 255);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(200, 200, 200);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(80, 80, 100);

    colourMap[ThemeColourRole::AccentPrimary] = juce::Colour(255, 0, 128);
    colourMap[ThemeColourRole::AccentSecondary] = juce::Colour(0, 255, 255);
    colourMap[ThemeColourRole::AccentSuccess] = juce::Colour(0, 255, 100);
    colourMap[ThemeColourRole::AccentWarning] = juce::Colour(255, 200, 0);
    colourMap[ThemeColourRole::AccentError] = juce::Colour(255, 50, 50);

    colourMap[ThemeColourRole::ButtonNormal] = juce::Colour(30, 30, 40);
    colourMap[ThemeColourRole::ButtonHover] = juce::Colour(50, 50, 60);
    colourMap[ThemeColourRole::ButtonPressed] = juce::Colour(255, 0, 128);
    colourMap[ThemeColourRole::ButtonDisabled] = juce::Colour(20, 20, 30);

    colourMap[ThemeColourRole::SliderTrack] = juce::Colour(20, 20, 30);
    colourMap[ThemeColourRole::SliderThumb] = juce::Colour(0, 255, 255);
    colourMap[ThemeColourRole::KnobBackground] = juce::Colour(15, 15, 25);
    colourMap[ThemeColourRole::KnobForeground] = juce::Colour(255, 0, 128);

    colourMap[ThemeColourRole::GlassTint] = juce::Colour(static_cast<juce::uint8>(20), static_cast<juce::uint8>(20), static_cast<juce::uint8>(40), static_cast<juce::uint8>(120));
    colourMap[ThemeColourRole::GlassBorder] = juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(0), static_cast<juce::uint8>(128), static_cast<juce::uint8>(200));
    colourMap[ThemeColourRole::Glow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(255), static_cast<juce::uint8>(255), static_cast<juce::uint8>(80));
    colourMap[ThemeColourRole::Shadow] = juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(0), static_cast<juce::uint8>(128), static_cast<juce::uint8>(100));

    // Cyberpunk-spezifische Metriken
    metrics.cornerRadiusMedium = 4.0f;
    metrics.cornerRadiusLarge = 6.0f;
    metrics.shadowBlur = 8.0f;
}

void ExtendedThemeSystem::applyMinimalTheme()
{
    // Minimal Theme - Reduziertes Design
    colourMap[ThemeColourRole::Background] = juce::Colour(250, 250, 250);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(240, 240, 240);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(255, 255, 255);

    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(255, 255, 255);
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(220, 220, 220);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(200, 200, 200);

    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(30, 30, 30);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(100, 100, 100);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(180, 180, 180);

    colourMap[ThemeColourRole::AccentPrimary] = juce::Colour(60, 100, 180);
    colourMap[ThemeColourRole::AccentSecondary] = juce::Colour(100, 60, 140);
    colourMap[ThemeColourRole::AccentSuccess] = juce::Colour(60, 140, 80);
    colourMap[ThemeColourRole::AccentWarning] = juce::Colour(180, 120, 40);
    colourMap[ThemeColourRole::AccentError] = juce::Colour(180, 60, 60);

    colourMap[ThemeColourRole::ButtonNormal] = juce::Colour(240, 240, 240);
    colourMap[ThemeColourRole::ButtonHover] = juce::Colour(230, 230, 230);
    colourMap[ThemeColourRole::ButtonPressed] = juce::Colour(220, 220, 220);
    colourMap[ThemeColourRole::ButtonDisabled] = juce::Colour(245, 245, 245);

    colourMap[ThemeColourRole::SliderTrack] = juce::Colour(230, 230, 230);
    colourMap[ThemeColourRole::SliderThumb] = juce::Colour(60, 100, 180);
    colourMap[ThemeColourRole::KnobBackground] = juce::Colour(240, 240, 240);
    colourMap[ThemeColourRole::KnobForeground] = juce::Colour(60, 100, 180);

    colourMap[ThemeColourRole::GlassTint] = juce::Colour(static_cast<juce::uint8>(200), static_cast<juce::uint8>(200), static_cast<juce::uint8>(200), static_cast<juce::uint8>(80));
    colourMap[ThemeColourRole::GlassBorder] = juce::Colour(220, 220, 220);
    colourMap[ThemeColourRole::Glow] = juce::Colour(static_cast<juce::uint8>(60), static_cast<juce::uint8>(100), static_cast<juce::uint8>(180), static_cast<juce::uint8>(30));
    colourMap[ThemeColourRole::Shadow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(80));

    // Minimal-spezifische Metriken
    metrics.cornerRadiusMedium = 6.0f;
    metrics.cornerRadiusLarge = 8.0f;
    metrics.shadowBlur = 6.0f;
}

void ExtendedThemeSystem::applyLiquidTheme()
{
    // Liquid Theme - Organische Farben
    colourMap[ThemeColourRole::Background] = juce::Colour(25, 30, 40);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(20, 25, 35);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(40, 45, 55);

    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(static_cast<juce::uint8>(50), static_cast<juce::uint8>(60), static_cast<juce::uint8>(75), static_cast<juce::uint8>(180));
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(90, 100, 115);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(110, 120, 140);

    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(245, 245, 250);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(190, 195, 200);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(110, 115, 120);

    colourMap[ThemeColourRole::AccentPrimary] = juce::Colour(120, 200, 240);
    colourMap[ThemeColourRole::AccentSecondary] = juce::Colour(180, 140, 220);
    colourMap[ThemeColourRole::AccentSuccess] = juce::Colour(120, 200, 160);
    colourMap[ThemeColourRole::AccentWarning] = juce::Colour(220, 180, 100);
    colourMap[ThemeColourRole::AccentError] = juce::Colour(220, 100, 120);

    colourMap[ThemeColourRole::ButtonNormal] = juce::Colour(70, 80, 95);
    colourMap[ThemeColourRole::ButtonHover] = juce::Colour(90, 100, 115);
    colourMap[ThemeColourRole::ButtonPressed] = juce::Colour(60, 70, 85);
    colourMap[ThemeColourRole::ButtonDisabled] = juce::Colour(static_cast<juce::uint8>(55), static_cast<juce::uint8>(65), static_cast<juce::uint8>(80), static_cast<juce::uint8>(128));

    colourMap[ThemeColourRole::SliderTrack] = juce::Colour(60, 70, 80);
    colourMap[ThemeColourRole::SliderThumb] = juce::Colour(120, 200, 240);
    colourMap[ThemeColourRole::KnobBackground] = juce::Colour(65, 75, 85);
    colourMap[ThemeColourRole::KnobForeground] = juce::Colour(100, 180, 220);

    colourMap[ThemeColourRole::GlassTint] = juce::Colour(static_cast<juce::uint8>(80), static_cast<juce::uint8>(90), static_cast<juce::uint8>(110), static_cast<juce::uint8>(90));
    colourMap[ThemeColourRole::GlassBorder] = juce::Colour(static_cast<juce::uint8>(110), static_cast<juce::uint8>(120), static_cast<juce::uint8>(140), static_cast<juce::uint8>(140));
    colourMap[ThemeColourRole::Glow] = juce::Colour(static_cast<juce::uint8>(120), static_cast<juce::uint8>(200), static_cast<juce::uint8>(240), static_cast<juce::uint8>(45));
    colourMap[ThemeColourRole::Shadow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(120));

    // Liquid-spezifische Metriken
    metrics.cornerRadiusMedium = 16.0f;
    metrics.cornerRadiusLarge = 24.0f;
    metrics.shadowBlur = 20.0f;
}

void ExtendedThemeSystem::applyNeonSakuraTheme()
{
    // NeonSakura Theme - Klassisches Neon Sakura Studio
    colourMap[ThemeColourRole::Background] = juce::Colour(15, 18, 25);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(10, 12, 18);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(30, 35, 45);

    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(static_cast<juce::uint8>(35), static_cast<juce::uint8>(40), static_cast<juce::uint8>(50), static_cast<juce::uint8>(215));
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(75, 80, 95);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(95, 100, 120);

    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(235, 240, 250);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(175, 180, 190);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(95, 100, 110);

    colourMap[ThemeColourRole::AccentPrimary] = juce::Colour(255, 105, 180); // Neon Pink
    colourMap[ThemeColourRole::AccentSecondary] = juce::Colour(100, 200, 255); // Neon Cyan
    colourMap[ThemeColourRole::AccentSuccess] = juce::Colour(80, 200, 120);
    colourMap[ThemeColourRole::AccentWarning] = juce::Colour(255, 180, 60);
    colourMap[ThemeColourRole::AccentError] = juce::Colour(255, 80, 100);

    colourMap[ThemeColourRole::ButtonNormal] = juce::Colour(55, 60, 70);
    colourMap[ThemeColourRole::ButtonHover] = juce::Colour(75, 80, 95);
    colourMap[ThemeColourRole::ButtonPressed] = juce::Colour(45, 50, 60);
    colourMap[ThemeColourRole::ButtonDisabled] = juce::Colour(static_cast<juce::uint8>(50), static_cast<juce::uint8>(55), static_cast<juce::uint8>(65), static_cast<juce::uint8>(128));

    colourMap[ThemeColourRole::SliderTrack] = juce::Colour(45, 50, 60);
    colourMap[ThemeColourRole::SliderThumb] = juce::Colour(255, 105, 180);
    colourMap[ThemeColourRole::KnobBackground] = juce::Colour(50, 55, 65);
    colourMap[ThemeColourRole::KnobForeground] = juce::Colour(100, 200, 255);

    colourMap[ThemeColourRole::GlassTint] = juce::Colour(static_cast<juce::uint8>(70), static_cast<juce::uint8>(75), static_cast<juce::uint8>(90), static_cast<juce::uint8>(100));
    colourMap[ThemeColourRole::GlassBorder] = juce::Colour(static_cast<juce::uint8>(95), static_cast<juce::uint8>(100), static_cast<juce::uint8>(120), static_cast<juce::uint8>(160));
    colourMap[ThemeColourRole::Glow] = juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(105), static_cast<juce::uint8>(180), static_cast<juce::uint8>(55));
    colourMap[ThemeColourRole::Shadow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(140));

    // NeonSakura-spezifische Metriken
    metrics.cornerRadiusMedium = 10.0f;
    metrics.cornerRadiusLarge = 14.0f;
    metrics.shadowBlur = 12.0f;
}

void ExtendedThemeSystem::applyProfessionalTheme()
{
    // Professional Theme - Subtle Farben für professionelle Nutzung
    colourMap[ThemeColourRole::Background] = juce::Colour(30, 32, 35);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(25, 27, 30);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(45, 47, 50);

    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(static_cast<juce::uint8>(45), static_cast<juce::uint8>(47), static_cast<juce::uint8>(50), static_cast<juce::uint8>(210));
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(85, 87, 90);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(105, 107, 110);

    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(230, 232, 235);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(170, 172, 175);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(90, 92, 95);

    colourMap[ThemeColourRole::AccentPrimary] = juce::Colour(80, 120, 180);
    colourMap[ThemeColourRole::AccentSecondary] = juce::Colour(140, 100, 160);
    colourMap[ThemeColourRole::AccentSuccess] = juce::Colour(80, 160, 100);
    colourMap[ThemeColourRole::AccentWarning] = juce::Colour(200, 140, 60);
    colourMap[ThemeColourRole::AccentError] = juce::Colour(200, 80, 90);

    colourMap[ThemeColourRole::ButtonNormal] = juce::Colour(60, 62, 65);
    colourMap[ThemeColourRole::ButtonHover] = juce::Colour(80, 82, 85);
    colourMap[ThemeColourRole::ButtonPressed] = juce::Colour(50, 52, 55);
    colourMap[ThemeColourRole::ButtonDisabled] = juce::Colour(static_cast<juce::uint8>(55), static_cast<juce::uint8>(57), static_cast<juce::uint8>(60), static_cast<juce::uint8>(128));

    colourMap[ThemeColourRole::SliderTrack] = juce::Colour(50, 52, 55);
    colourMap[ThemeColourRole::SliderThumb] = juce::Colour(80, 120, 180);
    colourMap[ThemeColourRole::KnobBackground] = juce::Colour(55, 57, 60);
    colourMap[ThemeColourRole::KnobForeground] = juce::Colour(80, 120, 180);

    colourMap[ThemeColourRole::GlassTint] = juce::Colour(static_cast<juce::uint8>(75), static_cast<juce::uint8>(77), static_cast<juce::uint8>(85), static_cast<juce::uint8>(95));
    colourMap[ThemeColourRole::GlassBorder] = juce::Colour(static_cast<juce::uint8>(105), static_cast<juce::uint8>(107), static_cast<juce::uint8>(115), static_cast<juce::uint8>(145));
    colourMap[ThemeColourRole::Glow] = juce::Colour(static_cast<juce::uint8>(80), static_cast<juce::uint8>(120), static_cast<juce::uint8>(180), static_cast<juce::uint8>(40));
    colourMap[ThemeColourRole::Shadow] = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(130));

    // Professional-spezifische Metriken
    metrics.cornerRadiusMedium = 8.0f;
    metrics.cornerRadiusLarge = 10.0f;
    metrics.shadowBlur = 10.0f;
}

void ExtendedThemeSystem::applyThemeColours(ExtendedThemeType themeType)
{
    switch (themeType)
    {
        case ExtendedThemeType::GlassUI:
            applyGlassUITheme();
            break;
        case ExtendedThemeType::Cyberpunk:
            applyCyberpunkTheme();
            break;
        case ExtendedThemeType::Minimal:
            applyMinimalTheme();
            break;
        case ExtendedThemeType::Liquid:
            applyLiquidTheme();
            break;
        case ExtendedThemeType::NeonSakura:
            applyNeonSakuraTheme();
            break;
        case ExtendedThemeType::Professional:
            applyProfessionalTheme();
            break;
    }
}

//==============================================================================
// Persistence
//==============================================================================
void ExtendedThemeSystem::saveTheme()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "NeonSakuraStudio";
    options.folderName = "NeonSakuraStudio";
    options.filenameSuffix = ".theme";
    options.osxLibrarySubFolder = "Application Support";

    juce::PropertiesFile props(options);
    props.setValue("themeType", getThemeTypeName(currentThemeType));
    props.saveIfNeeded();
}

void ExtendedThemeSystem::loadTheme()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "NeonSakuraStudio";
    options.folderName = "NeonSakuraStudio";
    options.filenameSuffix = ".theme";
    options.osxLibrarySubFolder = "Application Support";

    juce::PropertiesFile props(options);
    juce::String themeName = props.getValue("themeType", "Glass UI");

    ExtendedThemeType themeType = getThemeTypeFromName(themeName);
    setThemeType(themeType);
}

//==============================================================================
// Callbacks
//==============================================================================
void ExtendedThemeSystem::setThemeChangeCallback(ThemeChangeCallback callback)
{
    themeChangeCallback = std::move(callback);
}

void ExtendedThemeSystem::triggerThemeChangeCallback(ExtendedThemeType newThemeType)
{
    if (themeChangeCallback)
    {
        themeChangeCallback(newThemeType);
    }
}

//==============================================================================
// Utility
//==============================================================================
void ExtendedThemeSystem::drawGlassPanel(juce::Graphics& g, const juce::Rectangle<int>& bounds, float cornerRadius) const
{
    auto rect = bounds.toFloat();

    // Glass Background
    juce::ColourGradient gradient(
        getColour(ThemeColourRole::GlassTint),
        rect.getTopLeft(),
        getColour(ThemeColourRole::GlassTint).darker(0.2f),
        rect.getBottomRight(),
        true
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(rect, cornerRadius);

    // Glass Border
    g.setColour(getColour(ThemeColourRole::GlassBorder));
    g.drawRoundedRectangle(rect, cornerRadius, metrics.borderWidth);

    // Inner Glow
    juce::Path glowPath;
    glowPath.addRoundedRectangle(rect.reduced(metrics.borderWidth), cornerRadius - metrics.borderWidth);

    juce::ColourGradient glowGradient(
        juce::Colours::white.withAlpha(0.05f),
        rect.getCentre(),
        juce::Colours::transparentBlack,
        rect.getCentre(),
        true
    );
    g.setGradientFill(glowGradient);
    g.fillPath(glowPath);
}

void ExtendedThemeSystem::drawThemedButton(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& text,
                                        bool hovered, bool pressed, bool enabled) const
{
    auto rect = bounds.toFloat();
    float cornerRadius = metrics.cornerRadiusMedium;

    juce::Colour bgColour;
    if (!enabled)
        bgColour = getColour(ThemeColourRole::ButtonDisabled);
    else if (pressed)
        bgColour = getColour(ThemeColourRole::ButtonPressed);
    else if (hovered)
        bgColour = getColour(ThemeColourRole::ButtonHover);
    else
        bgColour = getColour(ThemeColourRole::ButtonNormal);

    // Hintergrund
    juce::ColourGradient bgGradient(
        bgColour,
        rect.getTopLeft(),
        bgColour.darker(0.1f),
        rect.getBottomRight(),
        true
    );
    g.setGradientFill(bgGradient);
    g.fillRoundedRectangle(rect, cornerRadius);

    // Border
    if (enabled)
    {
        g.setColour(getColour(ThemeColourRole::PanelBorder));
        g.drawRoundedRectangle(rect, cornerRadius, metrics.borderWidth);
    }

    // Text
    juce::Colour textColour = enabled
        ? getColour(ThemeColourRole::TextPrimary)
        : getColour(ThemeColourRole::TextDisabled);

    g.setColour(textColour);
    g.setFont(juce::Font(13.0f));
    g.drawText(text, rect, juce::Justification::centred);
}

juce::Colour ExtendedThemeSystem::blendColours(const juce::Colour& c1, const juce::Colour& c2, float ratio)
{
    ratio = juce::jlimit(0.0f, 1.0f, ratio);
    return c1.interpolatedWith(c2, ratio);
}

juce::Colour ExtendedThemeSystem::adjustBrightness(const juce::Colour& colour, float amount)
{
    if (amount > 0.0f)
        return colour.brighter(static_cast<float>(amount * 100.0f));
    else
        return colour.darker(static_cast<float>(std::abs(amount) * 100.0f));
}

//==============================================================================
// Private Methods
//==============================================================================
void ExtendedThemeSystem::initializeDefaultColours()
{
    // Standard-Fallback-Farben
    colourMap[ThemeColourRole::Background] = juce::Colour(20, 20, 25);
    colourMap[ThemeColourRole::BackgroundDark] = juce::Colour(15, 15, 20);
    colourMap[ThemeColourRole::BackgroundLight] = juce::Colour(35, 35, 45);
    colourMap[ThemeColourRole::PanelBackground] = juce::Colour(40, 40, 50);
    colourMap[ThemeColourRole::PanelBorder] = juce::Colour(80, 80, 90);
    colourMap[ThemeColourRole::PanelHighlight] = juce::Colour(100, 100, 120);
    colourMap[ThemeColourRole::TextPrimary] = juce::Colour(240, 240, 245);
    colourMap[ThemeColourRole::TextSecondary] = juce::Colour(180, 180, 190);
    colourMap[ThemeColourRole::TextDisabled] = juce::Colour(100, 100, 110);
}
