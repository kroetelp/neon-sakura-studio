// ============================================================================
// EnhancedPanelHeader.h - Erweiterter Panel-Header mit Drag & Drop
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include "FloatingPanelEnums.h"

// Forward Declarations
class FloatingPanelBase;
class ThemeManager;

/**
 * EnhancedPanelHeader - Erweiterter Panel-Header mit Drag & Drop
 *
 * Diese Klasse implementiert einen Panel-Header mit den folgenden Features:
 * - Drag & Drop für Floating Panels
 * - Panel-Titel Anzeige
 * - Close Button
 * - Minimize/Expand Buttons
 * - Pin/Floating Toggle
 * - Visual Feedback
 * - JUCE 8 Timer Kompatibilität (für Animationen)
 */
class EnhancedPanelHeader : public juce::Component, private juce::Timer
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    /**
     * Konstruktor mit Panel-Titel
     * @param title Der Titel des Panels
     */
    explicit EnhancedPanelHeader(const juce::String& title);

    ~EnhancedPanelHeader() override;

    //==============================================================================
    // Header Actions
    //==============================================================================
    /**
     * Setzt den Panel-Titel
     */
    void setTitle(const juce::String& title);

    /**
     * Gibt den aktuellen Panel-Titel zurück
     */
    juce::String getTitle() const { return title; }

    //==============================================================================
    // Panel Actions
    //==============================================================================
    /**
     * Aktiviert oder deaktiviert das Panel-Close
     */
    void setCloseEnabled(bool enabled);

    /**
     * Prüft, ob Panel-Close aktiviert ist
     */
    bool isCloseEnabled() const { return closeEnabled; }

    /**
     * Aktiviert oder deaktiviert Panel-Minimize
     */
    void setMinimizeEnabled(bool enabled);

    /**
     * Prüft, ob Panel-Minimize aktiviert ist
     */
    bool isMinimizeEnabled() const { return minimizeEnabled; }

    /**
     * Aktiviert oder deaktiviert Panel-Maximize
     */
    void setMaximizeEnabled(bool enabled);

    /**
     * Prüft, ob Panel-Maximize aktiviert ist
     */
    bool isMaximizeEnabled() const { return maximizeEnabled; }

    /**
     * Setzt den Panel State
     */
    void setPanelState(PanelState state);

    /**
     * Gibt den aktuellen Panel State zurück
     */
    PanelState getPanelState() const { return panelState; }

    //==============================================================================
    // Drag & Drop
    //==============================================================================
    /**
     * Aktiviert oder deaktiviert Drag & Drop
     */
    void setDragEnabled(bool enabled);

    /**
     * Prüft, ob Drag & Drop aktiviert ist
     */
    bool isDragEnabled() const { return dragEnabled; }

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Callback-Typ für Header-Aktionen
     */
    enum class HeaderAction
    {
        Close,           // Close Button geklickt
        Minimize,        // Minimize Button geklickt
        Maximize,        // Maximize Button geklickt
        DragStart,       // Drag gestartet
        DragEnd,         // Drag beendet
        DoubleClick      // Header wurde gedoppelklickt
    };

    /**
     * Callback-Typ für Header-Aktionen
     */
    using HeaderActionCallback = std::function<void(HeaderAction)>;

    /**
     * Setzt einen Callback, der bei Header-Aktionen aufgerufen wird
     */
    void setHeaderActionCallback(HeaderActionCallback callback);

    //==============================================================================
    // Visual Appearance
    //==============================================================================
    /**
     * Setzt die Header-Höhe
     */
    void setHeaderHeight(int height);

    /**
     * Gibt die aktuelle Header-Höhe zurück
     */
    int getHeaderHeight() const { return headerHeight; }

    /**
     * Setzt den Hintergrundstil (Solid/Gradient)
     */
    void setUseGradientBackground(bool useGradient);

    /**
     * Prüft, ob Gradient-Hintergrund aktiviert ist
     */
    bool isUsingGradientBackground() const { return useGradientBackground; }

    //==============================================================================
    // Button Visibility
    //==============================================================================
    /**
     * Zeigt oder versteckt den Close Button
     */
    void setCloseButtonVisible(bool visible);

    /**
     * Zeigt oder versteckt den Minimize Button
     */
    void setMinimizeButtonVisible(bool visible);

    /**
     * Zeigt oder versteckt den Maximize Button
     */
    void setMaximizeButtonVisible(bool visible);

    //==============================================================================
    // JUCE Component Overrides
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

protected:
    //==============================================================================
    // Override diese Methoden in abgeleiteten Klassen
    //==============================================================================
    /**
     * Wird aufgerufen, wenn das Panel-State sich ändert
     */
    virtual void onPanelStateChanged(PanelState newState);

    /**
     * Wird aufgerufen, wenn ein Header-Button geklickt wird
     */
    virtual void onHeaderAction(HeaderAction action);

    //==============================================================================
    // Protected Member für abgeleitete Klassen
    //==============================================================================
    ThemeManager* themeManager = nullptr;

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Header Properties
    juce::String title;
    int headerHeight = 40;
    PanelState panelState = PanelState::Floating;
    bool useGradientBackground = true;

    // Drag & Drop
    bool dragEnabled = true;
    bool isDragging = false;
    juce::Point<int> dragStartPos;

    // Button States
    bool closeEnabled = true;
    bool minimizeEnabled = true;
    bool maximizeEnabled = true;

    bool closeButtonVisible = true;
    bool minimizeButtonVisible = true;
    bool maximizeButtonVisible = true;

    // Button Bounds
    juce::Rectangle<int> closeButtonBounds;
    juce::Rectangle<int> minimizeButtonBounds;
    juce::Rectangle<int> maximizeButtonBounds;
    juce::Rectangle<int> titleBounds;

    // Hover States
    bool closeButtonHovered = false;
    bool minimizeButtonHovered = false;
    bool maximizeButtonHovered = false;
    bool headerHovered = false;

    // Animation
    float hoverAlpha = 0.0f;
    bool hovering = false;

    // Callback
    HeaderActionCallback headerActionCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void layoutButtons();
    juce::Rectangle<int> getButtonArea() const;
    juce::Rectangle<int> getTitleArea() const;
    void updateButtonHoverStates(const juce::Point<int>& mousePos);
    void triggerHeaderAction(HeaderAction action);

    // Paint Helpers
    void paintBackground(juce::Graphics& g);
    void paintTitle(juce::Graphics& g);
    void paintButton(juce::Graphics& g, const juce::Rectangle<int>& bounds, const juce::String& symbol, bool hovered);

    //==============================================================================
    // JUCE Timer Callback
    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnhancedPanelHeader)
};
