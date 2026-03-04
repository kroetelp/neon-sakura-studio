#pragma once

/**
 * ProfessionalTheme - FL Studio / Ableton Style Theme for Neon Sakura Studio
 *
 * Eine professionelle Farbpalette für ernsthafte Musikproduktion.
 * Inspiriert von FL Studio (Orange) und Ableton Live (Blau).
 */

#include <juce_gui_basics/juce_gui_basics.h>

namespace ProfessionalTheme
{
    // ========================================================================
    // Color Schemes
    // ========================================================================

    enum class ColorScheme
    {
        DarkOrange,     // FL Studio inspired
        DarkBlue,       // Ableton inspired
        DarkGray        // Neutral professional
    };

    // ========================================================================
    // Dark Orange (FL Studio Style)
    // ========================================================================
    namespace DarkOrange
    {
        // Background colors
        constexpr juce::uint32 Background        = 0xFF1C1C1E;  // Main background
        constexpr juce::uint32 BackgroundLighter = 0xFF2D2D30;  // Elevated surfaces
        constexpr juce::uint32 BackgroundDarker  = 0xFF121214;  // Deeper background

        // Panel colors
        constexpr juce::uint32 PanelBackground   = 0xFF252527;  // Panel backgrounds
        constexpr juce::uint32 PanelHeader       = 0xFF2D2D30;  // Panel headers
        constexpr juce::uint32 PanelBorder       = 0xFF3F3F46;  // Panel borders

        // Accent colors (FL Orange)
        constexpr juce::uint32 AccentPrimary     = 0xFFFF7E00;  // Primary accent (orange)
        constexpr juce::uint32 AccentSecondary   = 0xFFFF9933;  // Lighter orange
        constexpr juce::uint32 AccentMuted       = 0xFFB35900;  // Muted orange

        // Text colors
        constexpr juce::uint32 TextPrimary       = 0xFFDCDCDC;  // Primary text
        constexpr juce::uint32 TextSecondary     = 0xFF9D9D9D;  // Secondary text
        constexpr juce::uint32 TextMuted         = 0xFF6B6B6B;  // Muted text
        constexpr juce::uint32 TextBright        = 0xFFFFFFFF;  // Bright text (on dark)

        // Track colors (for mixer strips)
        constexpr juce::uint32 TrackDefault      = 0xFF3D3D40;
        constexpr juce::uint32 TrackSelected     = 0xFF4A4A4F;
        constexpr juce::uint32 TrackMuted        = 0xFF2A2A2D;

        // Control colors
        constexpr juce::uint32 ButtonNormal      = 0xFF3D3D42;
        constexpr juce::uint32 ButtonHover       = 0xFF4A4A50;
        constexpr juce::uint32 ButtonPressed     = 0xFF2D2D32;
        constexpr juce::uint32 ButtonActive      = AccentPrimary;

        constexpr juce::uint32 SliderTrack       = 0xFF3D3D42;
        constexpr juce::uint32 SliderThumb       = AccentPrimary;
        constexpr juce::uint32 SliderFill        = 0xFFFF7E00;

        constexpr juce::uint32 MeterBackground   = 0xFF1C1C1E;
        constexpr juce::uint32 MeterGreen        = 0xFF22C55E;
        constexpr juce::uint32 MeterYellow       = 0xFFEAB308;
        constexpr juce::uint32 MeterRed          = 0xFFEF4444;

        // Waveform / MIDI colors
        constexpr juce::uint32 WaveformOutline   = AccentPrimary;
        constexpr juce::uint32 WaveformFill      = 0x33FF7E00;
        constexpr juce::uint32 MIDINote          = AccentPrimary;
        constexpr juce::uint32 MIDISelected      = 0xFFFFFFFF;

        // Playhead / Timeline
        constexpr juce::uint32 Playhead          = 0xFFFFFFFF;
        constexpr juce::uint32 LoopRegion        = 0x33FF7E00;
        constexpr juce::uint32 GridLine          = 0xFF2D2D30;
        constexpr juce::uint32 GridLineBeat      = 0xFF3F3F46;

        // Status colors
        constexpr juce::uint32 Success           = 0xFF22C55E;
        constexpr juce::uint32 Warning           = 0xFFEAB308;
        constexpr juce::uint32 Error             = 0xFFEF4444;
        constexpr juce::uint32 Info              = 0xFF3B82F6;
    }

    // ========================================================================
    // Dark Blue (Ableton Style)
    // ========================================================================
    namespace DarkBlue
    {
        // Background colors
        constexpr juce::uint32 Background        = 0xFF191A1D;
        constexpr juce::uint32 BackgroundLighter = 0xFF26282C;
        constexpr juce::uint32 BackgroundDarker  = 0xFF0F1012;

        // Panel colors
        constexpr juce::uint32 PanelBackground   = 0xFF1E2023;
        constexpr juce::uint32 PanelHeader       = 0xFF26282C;
        constexpr juce::uint32 PanelBorder       = 0xFF3A3D42;

        // Accent colors (Ableton Blue/Teal)
        constexpr juce::uint32 AccentPrimary     = 0xFF0087AF;  // Ableton teal
        constexpr juce::uint32 AccentSecondary   = 0xFF00A5D4;
        constexpr juce::uint32 AccentMuted       = 0xFF006688;

        // Text colors
        constexpr juce::uint32 TextPrimary       = 0xFFE0E0E0;
        constexpr juce::uint32 TextSecondary     = 0xFFA0A0A0;
        constexpr juce::uint32 TextMuted         = 0xFF707070;
        constexpr juce::uint32 TextBright        = 0xFFFFFFFF;

        // Control colors
        constexpr juce::uint32 ButtonNormal      = 0xFF2D3033;
        constexpr juce::uint32 ButtonHover       = 0xFF3A3D42;
        constexpr juce::uint32 ButtonPressed     = 0xFF1E2023;
        constexpr juce::uint32 ButtonActive      = AccentPrimary;

        constexpr juce::uint32 SliderTrack       = 0xFF2D3033;
        constexpr juce::uint32 SliderThumb       = AccentPrimary;
        constexpr juce::uint32 SliderFill        = 0xFF0087AF;

        constexpr juce::uint32 MeterBackground   = 0xFF191A1D;
        constexpr juce::uint32 MeterGreen        = 0xFF22C55E;
        constexpr juce::uint32 MeterYellow       = 0xFFEAB308;
        constexpr juce::uint32 MeterRed          = 0xFFEF4444;

        // Status colors
        constexpr juce::uint32 Success           = 0xFF22C55E;
        constexpr juce::uint32 Warning           = 0xFFEAB308;
        constexpr juce::uint32 Error             = 0xFFEF4444;
        constexpr juce::uint32 Info              = 0xFF0087AF;

        // Waveform / MIDI colors
        constexpr juce::uint32 WaveformOutline   = AccentPrimary;
        constexpr juce::uint32 WaveformFill      = 0x330087AF;
        constexpr juce::uint32 MIDINote          = AccentPrimary;

        // Playhead / Timeline
        constexpr juce::uint32 Playhead          = 0xFFFFFFFF;
        constexpr juce::uint32 GridLine          = 0xFF26282C;
    }

    // ========================================================================
    // Dark Gray (Neutral Professional)
    // ========================================================================
    namespace DarkGray
    {
        // Background colors
        constexpr juce::uint32 Background        = 0xFF1F1F1F;  // Main background
        constexpr juce::uint32 BackgroundLighter = 0xFF2A2A2A;  // Elevated surfaces
        constexpr juce::uint32 BackgroundDarker  = 0xFF141414;  // Deeper background

        // Panel colors
        constexpr juce::uint32 PanelBackground   = 0xFF262626;  // Panel backgrounds
        constexpr juce::uint32 PanelHeader       = 0xFF2E2E2E;  // Panel headers
        constexpr juce::uint32 PanelBorder       = 0xFF3A3A3A;  // Panel borders

        // Accent colors (Neutral Blue-Gray)
        constexpr juce::uint32 AccentPrimary     = 0xFF6B7280;  // Primary accent (blue-gray)
        constexpr juce::uint32 AccentSecondary   = 0xFF9CA3AF;  // Lighter accent
        constexpr juce::uint32 AccentMuted       = 0xFF4B5563;  // Muted accent

        // Text colors
        constexpr juce::uint32 TextPrimary       = 0xFFE5E5E5;  // Primary text
        constexpr juce::uint32 TextSecondary     = 0xFFA3A3A3;  // Secondary text
        constexpr juce::uint32 TextMuted         = 0xFF737373;  // Muted text
        constexpr juce::uint32 TextBright        = 0xFFFFFFFF;  // Bright text

        // Control colors
        constexpr juce::uint32 ButtonNormal      = 0xFF3A3A3A;
        constexpr juce::uint32 ButtonHover       = 0xFF474747;
        constexpr juce::uint32 ButtonPressed     = 0xFF2D2D2D;
        constexpr juce::uint32 ButtonActive      = AccentPrimary;

        constexpr juce::uint32 SliderTrack       = 0xFF3A3A3A;
        constexpr juce::uint32 SliderThumb       = AccentPrimary;
        constexpr juce::uint32 SliderFill        = 0xFF6B7280;

        constexpr juce::uint32 MeterBackground   = 0xFF1F1F1F;
        constexpr juce::uint32 MeterGreen        = 0xFF22C55E;
        constexpr juce::uint32 MeterYellow       = 0xFFEAB308;
        constexpr juce::uint32 MeterRed          = 0xFFEF4444;

        // Waveform / MIDI colors
        constexpr juce::uint32 WaveformOutline   = AccentPrimary;
        constexpr juce::uint32 WaveformFill      = 0x336B7280;
        constexpr juce::uint32 MIDINote          = AccentPrimary;
        constexpr juce::uint32 MIDISelected      = 0xFFFFFFFF;

        // Playhead / Timeline
        constexpr juce::uint32 Playhead          = 0xFFFFFFFF;
        constexpr juce::uint32 LoopRegion        = 0x336B7280;
        constexpr juce::uint32 GridLine          = 0xFF2A2A2A;
        constexpr juce::uint32 GridLineBeat      = 0xFF3A3A3A;

        // Status colors
        constexpr juce::uint32 Success           = 0xFF22C55E;
        constexpr juce::uint32 Warning           = 0xFFEAB308;
        constexpr juce::uint32 Error             = 0xFFEF4444;
        constexpr juce::uint32 Info              = 0xFF6B7280;
    }

    // ========================================================================
    // Helper Functions
    // ========================================================================

    /** Convert uint32 to juce::Colour */
    inline juce::Colour colour(juce::uint32 argb)
    {
        return juce::Colour(argb);
    }

    /** Get a color scheme's background color */
    inline juce::Colour getBackground(ColorScheme scheme)
    {
        switch (scheme)
        {
            case ColorScheme::DarkOrange: return colour(DarkOrange::Background);
            case ColorScheme::DarkBlue:   return colour(DarkBlue::Background);
            case ColorScheme::DarkGray:   return colour(DarkGray::Background);
            default: return colour(DarkOrange::Background);
        }
    }

    /** Get a color scheme's primary accent color */
    inline juce::Colour getAccent(ColorScheme scheme)
    {
        switch (scheme)
        {
            case ColorScheme::DarkOrange: return colour(DarkOrange::AccentPrimary);
            case ColorScheme::DarkBlue:   return colour(DarkBlue::AccentPrimary);
            case ColorScheme::DarkGray:   return colour(DarkGray::AccentPrimary);
            default: return colour(DarkOrange::AccentPrimary);
        }
    }

    /** Get a color scheme's text color */
    inline juce::Colour getText(ColorScheme scheme)
    {
        switch (scheme)
        {
            case ColorScheme::DarkOrange: return colour(DarkOrange::TextPrimary);
            case ColorScheme::DarkBlue:   return colour(DarkBlue::TextPrimary);
            case ColorScheme::DarkGray:   return colour(DarkGray::TextPrimary);
            default: return colour(DarkOrange::TextPrimary);
        }
    }
}
