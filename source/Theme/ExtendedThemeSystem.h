// ============================================================================
// ExtendedThemeSystem.h - Erweitertes Theme System
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include <unordered_map>

// Forward Declarations
class ThemeManager;

/**
 * ExtendedThemeType - Neue Themes für Neon Sakura Studio
 *
 * GlassUI: Moderner Glass-Morphismus Look mit Blur und Transparenz
 * Cyberpunk: Neon-Farben, harte Kanten, futuristisches Design
 * Minimal: Reduziertes Design, klare Typografie, viel Weißraum
 * Liquid: Flüssige Formen, organische Farben, weiche Übergänge
 * NeonSakura: Klassisches Neon Sakura Studio Theme
 * Professional: Subtile Farben, konsistentes UI für professionelle Nutzung
 */
enum class ExtendedThemeType
{
    GlassUI,
    Cyberpunk,
    Minimal,
    Liquid,
    NeonSakura,
    Professional
};

/**
 * ThemeColourRole - Rollen für Farben im Theme
 *
 * Jedes Theme definiert Farben für diese Rollen
 */
enum class ThemeColourRole
{
    // Grundfarben
    Background,         // Haupt-Hintergrund
    BackgroundDark,     // Dunklerer Hintergrund (für Panels)
    BackgroundLight,     // Hellerer Hintergrund (für Highlights)

    // Panel-Farben
    PanelBackground,     // Panel Hintergrund
    PanelBorder,        // Panel Border
    PanelHighlight,      // Panel Highlight/Hover

    // Text-Farben
    TextPrimary,        // Haupt-Text
    TextSecondary,      // Sekundärer Text
    TextDisabled,       // Deaktivierter Text

    // Akzent-Farben
    AccentPrimary,      // Haupt-Akzent
    AccentSecondary,    // Sekundärer Akzent
    AccentSuccess,      // Erfolg (Grün)
    AccentWarning,      // Warnung (Gelb/Orange)
    AccentError,        // Fehler (Rot)

    // Interaktiv
    ButtonNormal,       // Normaler Button
    ButtonHover,        // Hover Button
    ButtonPressed,      // Gedrückter Button
    ButtonDisabled,     // Deaktivierter Button

    // Slider/Knob
    SliderTrack,        // Slider Track
    SliderThumb,        // Slider Thumb
    KnobBackground,     // Knob Hintergrund
    KnobForeground,     // Knob Vordergrund

    // Special
    GlassTint,         // Glass-Effekt Tint
    GlassBorder,        // Glass-Effekt Border
    Glow,              // Glow-Effekt
    Shadow             // Shadow Farbe
};

/**
 * ThemeMetrics - Metriken für das Theme
 *
 * Corner Radiuses, Padding, Abstände, etc.
 */
struct ThemeMetrics
{
    float cornerRadiusSmall = 4.0f;
    float cornerRadiusMedium = 8.0f;
    float cornerRadiusLarge = 12.0f;
    float cornerRadiusXL = 16.0f;

    float paddingTiny = 2.0f;
    float paddingSmall = 4.0f;
    float paddingMedium = 8.0f;
    float paddingLarge = 16.0f;
    float paddingXL = 24.0f;

    float spacingTiny = 2.0f;
    float spacingSmall = 4.0f;
    float spacingMedium = 8.0f;
    float spacingLarge = 16.0f;
    float spacingXL = 32.0f;

    float borderWidth = 1.0f;
    float shadowBlur = 10.0f;
    float shadowOffset = 4.0f;

    // Enums für konsistente Nutzung
    enum CornerSize { Tiny, Small, Medium, Large, XL };
    enum PaddingSize { PTiny, PSmall, PMedium, PLarge, PXL };
    enum SpacingSize { STiny, SSmall, SMedium, SLarge, SXL };
};

/**
 * ExtendedThemeSystem - Erweitertes Theme System
 *
 * Diese Klasse verwaltet das Theme-System für Neon Sakura Studio.
 * Sie bietet:
 * - Mehrere Theme-Typen (GlassUI, Cyberpunk, etc.)
 * - Farbps für alle UI-Elemente
 * - Theme-Metriken (Corner Radius, Padding, etc.)
 * - Theme-Wechsel mit Animation
 * - Persistenz der Theme-Auswahl
 *
 * Features:
 * - JUCE 8 Graphics API kompatibel
 * - Gradient Unterstützung
 * - Glass-Effekt Tints
 * - Callbacks für Theme-Änderungen
 */
class ExtendedThemeSystem
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    ExtendedThemeSystem();
    ~ExtendedThemeSystem();

    //==============================================================================
    // Theme Management
    //==============================================================================
    /**
     * Setzt den aktuellen Theme-Typ
     * @param themeType Der neue Theme-Typ
     */
    void setThemeType(ExtendedThemeType themeType);

    /**
     * Gibt den aktuellen Theme-Typ zurück
     */
    ExtendedThemeType getThemeType() const { return currentThemeType; }

    /**
     * Gibt den Namen eines Theme-Typs als String zurück
     */
    static juce::String getThemeTypeName(ExtendedThemeType themeType);

    /**
     * Gibt einen Theme-Typ anhand des Namens zurück
     */
    static ExtendedThemeType getThemeTypeFromName(const juce::String& name);

    //==============================================================================
    // Farbps Management
    //==============================================================================
    /**
     * Setzt eine Farbe für eine Rolle
     * @param role Die Farb-Rolle
     * @param colour Die Farbe
     */
    void setColour(ThemeColourRole role, const juce::Colour& colour);

    /**
     * Gibt eine Farbe für eine Rolle zurück
     * @param role Die Farb-Rolle
     * @return Die Farbe oder TransparentBlack wenn nicht definiert
     */
    juce::Colour getColour(ThemeColourRole role) const;

    /**
     * Gibt eine Farbe für eine Rolle mit angegebener Opacity zurück
     * @param role Die Farb-Rolle
     * @param opacity Die Opacity (0.0 - 1.0)
     * @return Die Farbe mit Opacity
     */
    juce::Colour getColourWithOpacity(ThemeColourRole role, float opacity) const;

    /**
     * Gibt einen Gradient für eine Rolle zurück
     * @param role Die Farb-Rolle
     * @param bounds Die Bounds für den Gradient
     * @param vertical Ob der Gradient vertikal sein soll
     * @return Der ColourGradient
     */
    juce::ColourGradient getGradient(ThemeColourRole role, const juce::Rectangle<int>& bounds, bool vertical = true) const;

    /**
     * Setzt einen Gradient für eine Rolle
     * @param role Die Farb-Rolle
     * @param gradient Der Gradient
     */
    void setGradient(ThemeColourRole role, const juce::ColourGradient& gradient);

    //==============================================================================
    // Theme Metrics
    //==============================================================================
    /**
     * Gibt die Theme-Metriken zurück
     */
    const ThemeMetrics& getMetrics() const { return metrics; }

    /**
     * Setzt die Theme-Metriken
     */
    void setMetrics(const ThemeMetrics& newMetrics);

    /**
     * Gibt einen Corner Radius zurück
     */
    float getCornerRadius(ThemeMetrics::CornerSize size = ThemeMetrics::Medium) const;

    /**
     * Gibt einen Padding-Wert zurück
     */
    float getPadding(ThemeMetrics::PaddingSize size = ThemeMetrics::PMedium) const;

    /**
     * Gibt einen Spacing-Wert zurück
     */
    float getSpacing(ThemeMetrics::SpacingSize size = ThemeMetrics::SMedium) const;

    //==============================================================================
    // Preset Themes
    //==============================================================================
    /**
     * Aktiviert das GlassUI Theme
     */
    void applyGlassUITheme();

    /**
     * Aktiviert das Cyberpunk Theme
     */
    void applyCyberpunkTheme();

    /**
     * Aktiviert das Minimal Theme
     */
    void applyMinimalTheme();

    /**
     * Aktiviert das Liquid Theme
     */
    void applyLiquidTheme();

    /**
     * Aktiviert das NeonSakura Theme
     */
    void applyNeonSakuraTheme();

    /**
     * Aktiviert das Professional Theme
     */
    void applyProfessionalTheme();

    //==============================================================================
    // Persistence
    //==============================================================================
    /**
     * Speichert das aktuelle Theme
     */
    void saveTheme();

    /**
     * Lädt das gespeicherte Theme
     */
    void loadTheme();

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Callback-Typ für Theme-Änderungen
     */
    using ThemeChangeCallback = std::function<void(ExtendedThemeType)>;

    /**
     * Setzt einen Callback, der bei Theme-Änderungen aufgerufen wird
     */
    void setThemeChangeCallback(ThemeChangeCallback callback);

    //==============================================================================
    // Utility
    //==============================================================================
    /**
     * Erzeugt ein Glass-Panel mit dem aktuellen Theme
     * @param g Graphics Context
     * @param bounds Die Bounds des Panels
     * @param cornerRadius Der Corner Radius
     */
    void drawGlassPanel(juce::Graphics& g, const juce::Rectangle<int>& bounds, float cornerRadius) const;

    /**
     * Erzeugt einen Button mit dem aktuellen Theme
     * @param g Graphics Context
     * @param bounds Die Bounds des Buttons
     * @param text Der Button-Text
     * @param hovered Ob gehovert wird
     * @param pressed Ob gedrückt wird
     * @param enabled Ob aktiviert ist
     */
    void drawThemedButton(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& text,
                         bool hovered, bool pressed, bool enabled = true) const;

    /**
     * Misch zwei Farben
     */
    static juce::Colour blendColours(const juce::Colour& c1, const juce::Colour& c2, float ratio);

    /**
     * Helligkeit einer Farbe anpassen
     */
    static juce::Colour adjustBrightness(const juce::Colour& colour, float amount);

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Aktuelles Theme
    ExtendedThemeType currentThemeType = ExtendedThemeType::GlassUI;

    // Farbps
    std::unordered_map<ThemeColourRole, juce::Colour> colourMap;
    std::unordered_map<ThemeColourRole, juce::ColourGradient> gradientMap;

    // Metriken
    ThemeMetrics metrics;

    // Callback
    ThemeChangeCallback themeChangeCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void initializeDefaultColours();
    void applyThemeColours(ExtendedThemeType themeType);
    void triggerThemeChangeCallback(ExtendedThemeType newThemeType);

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExtendedThemeSystem)
};
