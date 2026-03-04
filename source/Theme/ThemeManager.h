#pragma once

/**
 * ThemeManager - Zentrales Theme-Management für Neon Sakura Studio
 *
 * Ermöglicht das Umschalten zwischen verschiedenen Themes
 * und speziell Plugin-Management für konsistente Farbdarstellung.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include "ProfessionalTheme.h"

class ThemeManager
{
public:
    static ThemeManager& getInstance();

    ThemeManager();
    ~ThemeManager();

    // ============================================================
    // Theme Types
    // ============================================================

    enum class ThemeType
    {
        NeonSakura,    // Classic neon theme
        Professional,  // FL/Ableton style
        Custom          // User-defined custom theme
    };

    // ============================================================
    // Current Theme
    // ============================================================

    ThemeType getCurrentTheme() const { return currentTheme; }
    void setCurrentTheme(ThemeType type);

    void setCustomThemeColors(const juce::Colour& background,
                              const juce::Colour& backgroundLighter,
                              const juce::Colour& accent,
                              const juce::Colour& textPrimary,
                              const juce::Colour& textSecondary,
                              const juce::Colour& panelBackground,
                              const juce::Colour& panelHeader,
                              const juce::Colour& panelBorder,
                              const juce::Colour& button,
                              const juce::Colour& buttonHover,
                              const juce::Colour& buttonDown,
                              const juce::Colour& slider,
                              const juce::Colour& sliderTrack,
                              const juce::Colour& sliderThumb,
                              const juce::Colour& waveformFill,
                              const juce::Colour& waveformOutline,
                              const juce::Colour& midiNote,
                              const juce::Colour& playhead,
                              const juce::Colour& gridLine,
                              const juce::Colour& success,
                              const juce::Colour& warning,
                              const juce::Colour& error,
                              const juce::Colour& info);

    // ============================================================
    // Color Accessors (Automatically switch based on current theme)
    // ============================================================

    juce::Colour getBackgroundColor() const;
    juce::Colour getBackgroundLighterColor() const;
    juce::Colour getAccentColor() const;
    juce::Colour getTextPrimaryColor() const;
    juce::Colour getTextSecondaryColor() const;
    juce::Colour getPanelBackgroundColor() const;
    juce::Colour getPanelHeaderColor() const;
    juce::Colour getPanelBorderColor() const;
    juce::Colour getButtonColor() const;
    juce::Colour getButtonHoverColor() const;
    juce::Colour getButtonDownColor() const;
    juce::Colour getSliderColor() const;
    juce::Colour getSliderTrackColor() const;
    juce::Colour getSliderThumbColor() const;
    juce::Colour getWaveformFillColor() const;
    juce::Colour getWaveformOutlineColor() const;
    juce::Colour getMidiNoteColor() const;
    juce::Colour getPlayheadColor() const;
    juce::Colour getGridLineColor() const;
    juce::Colour getSuccessColor() const;
    juce::Colour getWarningColor() const;
    juce::Colour getErrorColor() const;
    juce::Colour getInfoColor() const;

    // Additional convenience colors
    juce::Colour getPanelBackgroundLighterColor() const { return backgroundLighterColor; }

    // ============================================================
    // Color Scheme
    // ============================================================

    ProfessionalTheme::ColorScheme getColorScheme() const;
    void setColorScheme(ProfessionalTheme::ColorScheme scheme);

    void cycleColorScheme();  // Cycle through: DarkOrange -> DarkBlue -> DarkGray

 ProfessionalTheme::ColorScheme getNextColorScheme() const;

    // ============================================================
    // Theme Change Callback
    // ============================================================

    std::function<void(ThemeType)> onThemeChanged;

    // ============================================================
    // Apply Theme to LookAndFeel
    // ============================================================

    void applyToLookAndFeel(juce::LookAndFeel& lf);
    void applyToComponent(juce::Component& component);

private:
    ThemeType currentTheme = ThemeType::NeonSakura;
    ProfessionalTheme::ColorScheme currentColorScheme = ProfessionalTheme::ColorScheme::DarkOrange;

    juce::Colour backgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Background);
    juce::Colour backgroundLighterColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::BackgroundLighter);
    juce::Colour accentColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::AccentPrimary);
    juce::Colour textPrimaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::TextPrimary);
    juce::Colour textSecondaryColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::TextSecondary);
    juce::Colour panelBackgroundColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::PanelBackground);
    juce::Colour panelHeaderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::PanelHeader);
    juce::Colour panelBorderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::PanelBorder);
    juce::Colour buttonColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::ButtonNormal);
    juce::Colour buttonHoverColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::ButtonHover);
    juce::Colour buttonDownColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::ButtonPressed);
    juce::Colour sliderColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::SliderFill);
    juce::Colour sliderTrackColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::SliderTrack);
    juce::Colour sliderThumbColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::SliderThumb);
    juce::Colour waveformFillColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::WaveformFill);
    juce::Colour waveformOutlineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::WaveformOutline);
    juce::Colour midiNoteColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::MIDINote);
    juce::Colour playheadColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Playhead);
    juce::Colour gridLineColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::GridLine);
    juce::Colour successColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Success);
    juce::Colour warningColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Warning);
    juce::Colour errorColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Error);
    juce::Colour infoColor = ProfessionalTheme::colour(ProfessionalTheme::DarkOrange::Info);
};

