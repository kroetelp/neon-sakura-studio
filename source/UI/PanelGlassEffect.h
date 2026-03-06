// ============================================================================
// PanelGlassEffect.h - Glass/Blur Effects für Panels
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>

/**
 * GlassStyle - Verschiedene Glass-Effekt-Stile
 *
 * None: Kein Effekt
 * Subtle: Sanfter Blur mit leichtem Glanz
 * Medium: Mittlerer Blur mit ausgeprägtem Glanz
 * Heavy: Starker Blur mit intensivem Glanz
 * Frosted: Frost-Effekt (texturierter Glass-Look)
 */
enum class GlassStyle
{
    None,
    Subtle,
    Medium,
    Heavy,
    Frosted
};

/**
 * GlassBorderType - Border-Typ für Glass-Effekt
 */
enum class GlassBorderType
{
    None,           // Kein Border
    Solid,          // Solider Border
    Gradient,       // Gradient Border
    Glow            // Glow statt Border
};

/**
 * PanelGlassEffect - Glass/Blur Effects für Panels
 *
 * Diese Klasse implementiert Glass- und Blur-Effekte für Panels.
 * Sie verwendet die JUCE Graphics API für saubere, performante
 * Darstellung mit Transparenz und Schatten.
 *
 * Features:
 * - Verschiedene Glass-Stile (Subtle, Medium, Heavy, Frosted)
 * - Configurable Blur Radius
 * - Border/Glow Optionen
 * - Transparenz Control
 * - Corner Radius Support
 * - DropShadow Support
 * - JUCE 8 Graphics API kompatibel
 */
class PanelGlassEffect
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    PanelGlassEffect();
    explicit PanelGlassEffect(GlassStyle style);
    ~PanelGlassEffect();

    //==============================================================================
    // Glass Style
    //==============================================================================
    /**
     * Setzt den Glass-Stil
     * @param style Der neue Glass-Stil
     */
    void setGlassStyle(GlassStyle style);

    /**
     * Gibt den aktuellen Glass-Stil zurück
     */
    GlassStyle getGlassStyle() const { return glassStyle; }

    //==============================================================================
    // Blur Configuration
    //==============================================================================
    /**
     * Setzt den Blur-Radius
     * @param radius Der Blur-Radius in Pixel (0-50)
     */
    void setBlurRadius(float radius);

    /**
     * Gibt den aktuellen Blur-Radius zurück
     */
    float getBlurRadius() const { return blurRadius; }

    /**
     * Aktiviert oder deaktiviert den Blur-Effekt
     * @param enabled true um Blur zu aktivieren
     */
    void setBlurEnabled(bool enabled);

    /**
     * Prüft, ob Blur aktiviert ist
     */
    bool isBlurEnabled() const { return blurEnabled; }

    //==============================================================================
    // Border Configuration
    //==============================================================================
    /**
     * Setzt den Border-Typ
     * @param type Der Border-Typ
     */
    void setBorderType(GlassBorderType type);

    /**
     * Gibt den aktuellen Border-Typ zurück
     */
    GlassBorderType getBorderType() const { return borderType; }

    /**
     * Setzt die Border-Breite
     * @param width Die Border-Breite in Pixel
     */
    void setBorderWidth(float width);

    /**
     * Gibt die aktuelle Border-Breite zurück
     */
    float getBorderWidth() const { return borderWidth; }

    /**
     * Setzt die Border-Farbe
     * @param colour Die Border-Farbe
     */
    void setBorderColour(const juce::Colour& colour);

    /**
     * Gibt die aktuelle Border-Farbe zurück
     */
    juce::Colour getBorderColour() const { return borderColour; }

    //==============================================================================
    // Corner Radius
    //==============================================================================
    /**
     * Setzt den Corner Radius
     * @param radius Der Corner Radius in Pixel
     */
    void setCornerRadius(float radius);

    /**
     * Gibt den aktuellen Corner Radius zurück
     */
    float getCornerRadius() const { return cornerRadius; }

    //==============================================================================
    // Opacity / Transparency
    //==============================================================================
    /**
     * Setzt die Opacity des Glass-Effekts
     * @param opacity Die Opacity (0.0 = transparent, 1.0 = undurchsichtig)
     */
    void setOpacity(float opacity);

    /**
     * Gibt die aktuelle Opacity zurück
     */
    float getOpacity() const { return opacity; }

    /**
     * Setzt die Background-Transparenz
     * @param transparent true für transparenten Hintergrund
     */
    void setBackgroundTransparent(bool transparent);

    /**
     * Prüft, ob der Hintergrund transparent ist
     */
    bool isBackgroundTransparent() const { return backgroundTransparent; }

    //==============================================================================
    // Shadow Configuration
    //==============================================================================
    /**
     * Aktiviert oder deaktiviert den DropShadow
     * @param enabled true um Shadow zu aktivieren
     */
    void setShadowEnabled(bool enabled);

    /**
     * Prüft, ob Shadow aktiviert ist
     */
    bool isShadowEnabled() const { return shadowEnabled; }

    /**
     * Setzt die Shadow-Farbe
     * @param colour Die Shadow-Farbe
     */
    void setShadowColour(const juce::Colour& colour);

    /**
     * Gibt die aktuelle Shadow-Farbe zurück
     */
    juce::Colour getShadowColour() const { return shadowColour; }

    /**
     * Setzt den Shadow-Offset
     * @param offset Der Offset in Pixel (x, y)
     */
    void setShadowOffset(const juce::Point<int>& offset);

    /**
     * Setzt den Shadow-Offset
     * @param x X-Offset
     * @param y Y-Offset
     */
    void setShadowOffset(int x, int y);

    /**
     * Gibt den aktuellen Shadow-Offset zurück
     */
    juce::Point<int> getShadowOffset() const { return shadowOffset; }

    /**
     * Setzt die Shadow-Ausdehnung (Spread)
     * @param spread Die Spread in Pixel
     */
    void setShadowSpread(float spread);

    /**
     * Gibt die aktuelle Shadow-Ausdehnung zurück
     */
    float getShadowSpread() const { return shadowSpread; }

    //==============================================================================
    // Presets
    //==============================================================================
    /**
     * Aktiviert das Subtle Glass Preset
     */
    void applySubtlePreset();

    /**
     * Aktiviert das Medium Glass Preset
     */
    void applyMediumPreset();

    /**
     * Aktiviert das Heavy Glass Preset
     */
    void applyHeavyPreset();

    /**
     * Aktiviert das Frosted Glass Preset
     */
    void applyFrostedPreset();

    //==============================================================================
    // Drawing
    //==============================================================================
    /**
     * Zeichnet den Glass-Effekt auf den angegebenen Bereich
     * @param g Graphics Context
     * @param bounds Der Bereich in dem gezeichnet werden soll
     */
    void drawGlassEffect(juce::Graphics& g, const juce::Rectangle<int>& bounds) const;

    /**
     * Zeichnet den Glass-Effekt mit angegebener Background-Farbe
     * @param g Graphics Context
     * @param bounds Der Bereich in dem gezeichnet werden soll
     * @param bgColour Die Background-Farbe
     */
    void drawGlassEffect(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const;

    /**
     * Erzeugt eine Path mit Corner Radius für den Glass-Effekt
     * @param bounds Die Bounds
     * @return Die Path
     */
    juce::Path createGlassPath(const juce::Rectangle<int>& bounds) const;

    //==============================================================================
    // Utility
    //==============================================================================
    /**
     * Gibt die Standardwerte für einen Glass-Stil zurück
     */
    struct GlassPresetValues
    {
        float blurRadius;
        float opacity;
        float cornerRadius;
        float borderWidth;
        GlassBorderType borderType;
        juce::Colour borderColour;
    };

    /**
     * Gibt die Preset-Werte für einen Stil zurück
     */
    static GlassPresetValues getPresetValues(GlassStyle style);

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Style
    GlassStyle glassStyle = GlassStyle::Medium;

    // Blur
    float blurRadius = 8.0f;
    bool blurEnabled = true;

    // Border
    GlassBorderType borderType = GlassBorderType::Solid;
    float borderWidth = 1.0f;
    juce::Colour borderColour = juce::Colour(100, 100, 120);

    // Corner Radius
    float cornerRadius = 8.0f;

    // Opacity
    float opacity = 0.85f;
    bool backgroundTransparent = false;

    // Shadow
    bool shadowEnabled = true;
    juce::Colour shadowColour = juce::Colour(static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(0), static_cast<juce::uint8>(100));
    juce::Point<int> shadowOffset = juce::Point<int>(0, 2);
    float shadowSpread = 4.0f;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void drawSubtleGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const;
    void drawMediumGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const;
    void drawHeavyGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const;
    void drawFrostedGlass(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::Colour& bgColour) const;

    void drawShadow(juce::Graphics& g, const juce::Rectangle<int>& bounds) const;
    void drawBorder(juce::Graphics& g, const juce::Rectangle<int>& bounds) const;
    void drawGlow(juce::Graphics& g, const juce::Rectangle<int>& bounds) const;

    juce::ColourGradient createGradientForBounds(const juce::Rectangle<int>& bounds, const juce::Colour& colour) const;

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelGlassEffect)
};
