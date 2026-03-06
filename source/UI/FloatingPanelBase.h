// ============================================================================
// FloatingPanelBase.h - Erweiterte Basis für Floating Panels
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include "FloatingPanelEnums.h"

// Forward Declarations
class DockingManager;
class ZoneSnapManager;
class ThemeManager;

/**
 * FloatingPanelBase - Erweiterte Basis für Floating Panels
 *
 * Diese Klasse bietet die grundlegende Funktionalität für alle Panels,
 * die gefloatet, gedockt oder minimiert werden können.
 *
 * Features:
 * - Panel State Management (Floating, Snapped, Minimized, Hidden)
 * - Size Mode Management (Compact, Standard, Expanded, Full)
 * - Animation Unterstützung
 * - Theme Awareness
 * - Snap Zone Management
 */
class FloatingPanelBase : public juce::Component, private juce::Timer
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    FloatingPanelBase();
    ~FloatingPanelBase() override;

    //==============================================================================
    // Panel State Management
    //==============================================================================
    /**
     * Setzt den aktuellen Panel State
     * @param newState Der neue PanelState (Floating, Snapped, Minimized, etc.)
     * @param animate Ob der Übergang animiert werden soll
     */
    void setPanelState(PanelState newState, bool animate = true);

    /**
     * Gibt den aktuellen Panel State zurück
     */
    PanelState getPanelState() const { return panelState; }

    /**
     * Prüft, ob das Panel aktuell gefloatet ist
     */
    bool isFloating() const { return panelState == PanelState::Floating; }

    /**
     * Prüft, ob das Panel gesnappt ist
     */
    bool isSnapped() const { return panelState == PanelState::Snapped; }

    /**
     * Prüft, ob das Panel minimiert ist
     */
    bool isMinimized() const { return panelState == PanelState::Minimized; }

    /**
     * Prüft, ob das Panel versteckt ist
     */
    bool isHidden() const { return panelState == PanelState::Hidden; }

    //==============================================================================
    // Size Mode Management
    //==============================================================================
    /**
     * Setzt den aktuellen Size Mode
     * @param newMode Der neue PanelSizeMode (Compact, Standard, Expanded, Full)
     * @param animate Ob der Übergang animiert werden soll
     */
    void setSizeMode(PanelSizeMode newMode, bool animate = true);

    /**
     * Gibt den aktuellen Size Mode zurück
     */
    PanelSizeMode getSizeMode() const { return sizeMode; }

    /**
     * Prüft, ob das Panel im Compact Mode ist
     */
    bool isCompact() const { return sizeMode == PanelSizeMode::Compact; }

    /**
     * Prüft, ob das Panel im Standard Mode ist
     */
    bool isStandard() const { return sizeMode == PanelSizeMode::Standard; }

    /**
     * Prüft, ob das Panel im Expanded Mode ist
     */
    bool isExpanded() const { return sizeMode == PanelSizeMode::Expanded; }

    //==============================================================================
    // Snap Zone Management
    //==============================================================================
    /**
     * Setzt die Snap Zone für dieses Panel
     * @param zone Die neue PanelSnapZone
     */
    void setSnapZone(PanelSnapZone zone);

    /**
     * Gibt die aktuelle Snap Zone zurück
     */
    PanelSnapZone getSnapZone() const { return snapZone; }

    /**
     * Prüft, ob das Panel in einer bestimmten Zone gesnappt ist
     */
    bool isInZone(PanelSnapZone zone) const { return snapZone == zone; }

    //==============================================================================
    // Animation
    //==============================================================================
    /**
     * Startet eine Fade-In Animation
     * @param duration Dauer in Millisekunden
     */
    void fadeIn(int duration = 200);

    /**
     * Startet eine Fade-Out Animation
     * @param duration Dauer in Millisekunden
     */
    void fadeOut(int duration = 200);

    /**
     * Setzt die Opacity direkt (ohne Animation)
     */
    void setOpacity(float opacity);

    /**
     * Gibt die aktuelle Opacity zurück
     */
    float getOpacity() const { return currentOpacity; }

    //==============================================================================
    // Visibility Management
    //==============================================================================
    /**
     * Zeigt das Panel an
     */
    void showPanel();

    /**
     * Versteckt das Panel
     */
    void hidePanel();

    /**
     * Toggle zwischen visible und hidden
     */
    void toggleVisibility();

    //==============================================================================
    // Docking Support
    //==============================================================================
    /**
     * Setzt den DockingManager für dieses Panel
     */
    void setDockingManager(DockingManager* manager);

    /**
     * Gibt den DockingManager zurück
     */
    DockingManager* getDockingManager() const { return dockingManager; }

    /**
     * Prüft, ob das Panel aktuell gedockt ist
     */
    bool isDocked() const { return dockingManager != nullptr && isSnapped(); }

    //==============================================================================
    // Panel ID
    //==============================================================================
    /**
     * Setzt eine eindeutige ID für dieses Panel
     */
    void setPanelID(const juce::String& id);

    /**
     * Gibt die Panel ID zurück
     */
    juce::String getPanelID() const { return panelID; }

    //==============================================================================
    // Panel Title
    //==============================================================================
    /**
     * Setzt den Panel Titel
     */
    void setPanelTitle(const juce::String& title);

    /**
     * Gibt den Panel Titel zurück
     */
    juce::String getPanelTitle() const { return panelTitle; }

    //==============================================================================
    // Preferred Size
    //==============================================================================
    /**
     * Setzt die bevorzugte Größe für dieses Panel
     */
    void setPreferredSize(int width, int height);

    /**
     * Setzt die bevorzugte Größe für dieses Panel
     */
    void setPreferredSize(const juce::Rectangle<int>& size);

    /**
     * Gibt die bevorzugte Größe zurück
     */
    juce::Rectangle<int> getPreferredSize() const { return preferredSize; }

    /**
     * Setzt die minimale Größe für dieses Panel
     */
    void setMinSize(int width, int height);

    /**
     * Setzt die minimale Größe für dieses Panel
     */
    void setMinSize(const juce::Rectangle<int>& size);

    /**
     * Gibt die minimale Größe zurück
     */
    juce::Rectangle<int> getMinSize() const { return minSize; }

    /**
     * Setzt die maximale Größe für dieses Panel
     */
    void setMaxSize(int width, int height);

    /**
     * Setzt die maximale Größe für dieses Panel
     */
    void setMaxSize(const juce::Rectangle<int>& size);

    /**
     * Gibt die maximale Größe zurück
     */
    juce::Rectangle<int> getMaxSize() const { return maxSize; }

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Callback-Typ für Panel-Änderungen
     */
    using PanelChangeCallback = std::function<void(FloatingPanelBase*, PanelState, PanelSizeMode)>;

    /**
     * Setzt einen Callback, der bei Panel-Änderungen aufgerufen wird
     */
    void setPanelChangeCallback(PanelChangeCallback callback);

    //==============================================================================
    // JUCE Component Overrides
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

protected:
    //==============================================================================
    // Override diese Methoden in abgeleiteten Klassen
    //==============================================================================
    /**
     * Wird aufgerufen, wenn der Panel State sich ändert
     */
    virtual void onPanelStateChanged(PanelState oldState, PanelState newState);

    /**
     * Wird aufgerufen, wenn der Size Mode sich ändert
     */
    virtual void onSizeModeChanged(PanelSizeMode oldMode, PanelSizeMode newMode);

    /**
     * Wird aufgerufen, wenn das Panel resized wird
     * Abgeleitete Klassen können hier ihre Children anordnen
     */
    virtual void layoutChildren() = 0;

    /**
     * Wird aufgerufen, um das Hintergrund-Panel zu zeichnen
     */
    virtual void paintPanelBackground(juce::Graphics& g);

    //==============================================================================
    // Protected Member für abgeleitete Klassen
    //==============================================================================
    ThemeManager* themeManager = nullptr;

    //==============================================================================
    // Animations-Helper
    //==============================================================================
    bool isAnimating() const { return animating; }
    float getAnimationProgress() const { return animationProgress; }

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Panel State & Size Mode
    PanelState panelState = PanelState::Floating;
    PanelSizeMode sizeMode = PanelSizeMode::Standard;
    PanelSnapZone snapZone = PanelSnapZone::None;

    // Identification
    juce::String panelID;
    juce::String panelTitle = "Panel";

    // Docking
    DockingManager* dockingManager = nullptr;

    // Size Constraints
    juce::Rectangle<int> preferredSize = juce::Rectangle<int>(0, 0, 400, 300);
    juce::Rectangle<int> minSize = juce::Rectangle<int>(0, 0, 200, 150);
    juce::Rectangle<int> maxSize = juce::Rectangle<int>(0, 0, 1920, 1080);

    // Animation State
    float currentOpacity = 1.0f;
    float targetOpacity = 1.0f;
    float animationProgress = 0.0f;
    bool animating = false;
    int animationDuration = 0;
    int animationStartTime = 0;

    // Callback
    PanelChangeCallback panelChangeCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void startAnimation(int duration);
    void stopAnimation();
    void updateAnimation();
    void triggerPanelChangeCallback(PanelState newState, PanelSizeMode newMode);

    //==============================================================================
    // JUCE Timer Callback
    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingPanelBase)
};
