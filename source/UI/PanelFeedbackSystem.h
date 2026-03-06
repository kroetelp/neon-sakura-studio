// ============================================================================
// PanelFeedbackSystem.h - Feedback System für Panels
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "FloatingPanelEnums.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

// Forward Declarations
class juce::Component;

/**
 * FeedbackType - Typen für visuelles Feedback
 */
enum class FeedbackType
{
    None,                // Kein Feedback
    SnapPreview,         // Preview beim Hover über Snap-Zone
    DockingSuccess,      // Erfolgreiches Andocken
    DockingFail,         // Fehlgeschlagenes Andocken
    Undocking,           // Undocken
    Minimize,            // Minimieren
    Maximize,            // Maximieren
    Resize,              // Resize-Feedback
    Highlight,           // Highlight-Effekt
    Glow,                // Glow-Effekt
    Pulse,               // Pulsieren
    Shake,               // Shake bei Fehlern
    DragStart,           // Drag startet
    DragEnd,             // Drag endet
    ZoneActive,          // Zone ist aktiv
    ZoneInactive         // Zone ist inaktiv
};

/**
 * FeedbackStyle - Verschiedene Feedback-Stile
 */
enum class FeedbackStyle
{
    Subtle,              // Subtiles Feedback
    Normal,              // Normales Feedback
    Strong,              // Stärkeres Feedback
    Intense              // Intensives Feedback
};

/**
 * FeedbackAnimation - Animation für Feedback
 */
struct FeedbackAnimation
{
    FeedbackType type = FeedbackType::None;
    juce::Component* targetComponent = nullptr;
    juce::Rectangle<int> bounds;

    float opacity = 0.0f;
    float targetOpacity = 1.0f;
    float currentScale = 1.0f;
    float targetScale = 1.0f;

    juce::Colour colour;
    juce::Colour glowColour;

    int duration = 150;
    int startTime = 0;
    float progress = 0.0f;

    bool isActive = false;
    bool autoRemove = true;

    std::function<void()> onComplete;
};

/**
 * SnapZonePreview - Preview für Snap-Zone
 */
struct SnapZonePreview
{
    PanelSnapZone zone = PanelSnapZone::None;
    juce::Rectangle<int> bounds;
    bool isActive = false;
    float opacity = 0.0f;
    float targetOpacity = 0.7f;

    juce::Colour fillColour;
    juce::Colour borderColour;
    float borderThickness = 2.0f;
    float cornerRadius = 8.0f;
};

/**
 * PanelFeedbackSystem - Feedback System für Panels
 *
 * Diese Klasse verwaltet das visuelle Feedback für Panel-Operationen wie:
 * - Snap-Zone Previews beim Drag-and-Drop
 * - Docking/Undocking Feedback
 * - Highlight- und Glow-Effekte
 * - Shake-Animation bei Fehlern
 * - Pulse-Animation für aktive Zonen
 *
 * JUCE 8 Hinweis: Diese Klasse erbt von Timer für Feedback-Animationen
 */
class PanelFeedbackSystem : public juce::Component, private juce::Timer
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    PanelFeedbackSystem();
    ~PanelFeedbackSystem() override;

    //==============================================================================
    // Snap-Zone Management
    //==============================================================================
    /**
     * Registriert eine Snap-Zone
     * @param zone Die Snap-Zone
     * @param bounds Die Bounds der Zone
     * @param cornerRadius Der Corner Radius
     */
    void registerSnapZone(PanelSnapZone zone, const juce::Rectangle<int>& bounds, float cornerRadius = 8.0f);

    /**
     * Entfernt eine Snap-Zone
     * @param zone Die zu entfernende Zone
     */
    void unregisterSnapZone(PanelSnapZone zone);

    /**
     * Setzt die Bounds einer Snap-Zone
     * @param zone Die Snap-Zone
     * @param bounds Die neuen Bounds
     */
    void setSnapZoneBounds(PanelSnapZone zone, const juce::Rectangle<int>& bounds);

    /**
     * Gibt die Bounds einer Snap-Zone zurück
     */
    juce::Rectangle<int> getSnapZoneBounds(PanelSnapZone zone) const;

    /**
     * Aktiviert eine Snap-Zone (zeigt Preview an)
     * @param zone Die Snap-Zone
     */
    void activateSnapZone(PanelSnapZone zone);

    /**
     * Deaktiviert eine Snap-Zone (versteckt Preview)
     * @param zone Die Snap-Zone
     */
    void deactivateSnapZone(PanelSnapZone zone);

    /**
     * Deaktiviert alle Snap-Zonen
     */
    void deactivateAllSnapZones();

    /**
     * Prüft, ob eine Snap-Zone aktiv ist
     */
    bool isSnapZoneActive(PanelSnapZone zone) const;

    //==============================================================================
    // Feedback auslösen
    //==============================================================================
    /**
     * Zeigt Docking-Success Feedback an
     * @param component Die Ziel-Komponente
     * @param duration Dauer des Feedbacks in Millisekunden
     */
    void showDockingSuccess(juce::Component* component, int duration = 300);

    /**
     * Zeigt Docking-Fail Feedback an
     * @param component Die Ziel-Komponente
     * @param duration Dauer des Feedbacks in Millisekunden
     */
    void showDockingFail(juce::Component* component, int duration = 400);

    /**
     * Zeigt Undocking Feedback an
     * @param component Die Ziel-Komponente
     * @param duration Dauer des Feedbacks in Millisekunden
     */
    void showUndocking(juce::Component* component, int duration = 200);

    /**
     * Zeigt Minimize Feedback an
     * @param component Die Ziel-Komponente
     */
    void showMinimizeFeedback(juce::Component* component);

    /**
     * Zeigt Maximize Feedback an
     * @param component Die Ziel-Komponente
     */
    void showMaximizeFeedback(juce::Component* component);

    /**
     * Zeigt Resize Feedback an
     * @param component Die Ziel-Komponente
     * @param newBounds Die neuen Bounds
     */
    void showResizeFeedback(juce::Component* component, const juce::Rectangle<int>& newBounds);

    /**
     * Zeigt Highlight Feedback an
     * @param component Die Ziel-Komponente
     * @param bounds Die Bounds die gehighlighted werden sollen
     * @param duration Dauer des Feedbacks
     */
    void showHighlight(juce::Component* component, const juce::Rectangle<int>& bounds, int duration = 200);

    /**
     * Zeigt Glow Feedback an
     * @param component Die Ziel-Komponente
     * @param bounds Die Bounds die glowen sollen
     * @param duration Dauer des Feedbacks
     */
    void showGlow(juce::Component* component, const juce::Rectangle<int>& bounds, int duration = 300);

    /**
     * Zeigt Pulse Feedback an
     * @param component Die Ziel-Komponente
     * @param pulseCount Anzahl der Pulses
     */
    void showPulse(juce::Component* component, int pulseCount = 3);

    /**
     * Zeigt Shake Feedback an (bei Fehlern)
     * @param component Die Ziel-Komponente
     * @param intensity Shake-Intensität
     */
    void showShake(juce::Component* component, int intensity = 10);

    /**
     * Zeigt Drag-Start Feedback an
     * @param component Die gezogene Komponente
     */
    void showDragStart(juce::Component* component);

    /**
     * Zeigt Drag-End Feedback an
     * @param component Die gezogene Komponente
     */
    void showDragEnd(juce::Component* component);

    //==============================================================================
    // Benutzerdefiniertes Feedback
    //==============================================================================
    /**
     * Zeigt benutzerdefiniertes Feedback an
     * @param type Der Feedback-Typ
     * @param component Die Ziel-Komponente
     * @param bounds Die Bounds für das Feedback
     * @param duration Dauer des Feedbacks
     * @param style Der Feedback-Stil
     */
    void showFeedback(FeedbackType type, juce::Component* component,
                      const juce::Rectangle<int>& bounds, int duration = 200,
                      FeedbackStyle style = FeedbackStyle::Normal);

    /**
     * Fügt eine benutzerdefinierte Feedback-Animation hinzu
     * @param animation Die Animations-Konfiguration
     */
    void addFeedbackAnimation(const FeedbackAnimation& animation);

    //==============================================================================
    // Feedback-Steuerung
    //==============================================================================
    /**
     * Stoppt alle Feedback-Animationen für eine Komponente
     * @param component Die Ziel-Komponente
     */
    void stopFeedbackForComponent(juce::Component* component);

    /**
     * Stoppt alle Feedback-Animationen eines bestimmten Typs
     * @param type Der Feedback-Typ
     */
    void stopFeedbackType(FeedbackType type);

    /**
     * Stoppt alle Feedback-Animationen
     */
    void stopAllFeedback();

    /**
     * Löscht alle Snap-Zonen
     */
    void clearAllSnapZones();

    //==============================================================================
    // Feedback-Status
    //==============================================================================
    /**
     * Prüft, ob Feedback für eine Komponente aktiv ist
     */
    bool hasFeedbackForComponent(juce::Component* component) const;

    /**
     * Prüft, ob Feedback eines bestimmten Typs aktiv ist
     */
    bool hasFeedbackType(FeedbackType type) const;

    /**
     * Gibt die Anzahl der aktiven Feedback-Animationen zurück
     */
    int getActiveFeedbackCount() const;

    //==============================================================================
    // Appearance
    //==============================================================================
    /**
     * Setzt die Standard-Farbe für Snap-Zone Previews
     */
    void setSnapZonePreviewColour(const juce::Colour& colour);

    /**
     * Gibt die Standard-Farbe für Snap-Zone Previews zurück
     */
    juce::Colour getSnapZonePreviewColour() const { return snapZonePreviewColour; }

    /**
     * Setzt die Border-Farbe für Snap-Zone Previews
     */
    void setSnapZoneBorderColour(const juce::Colour& colour);

    /**
     * Gibt die Border-Farbe für Snap-Zone Previews zurück
     */
    juce::Colour getSnapZoneBorderColour() const { return snapZoneBorderColour; }

    /**
     * Setzt die Glow-Farbe für Highlight-Feedback
     */
    void setGlowColour(const juce::Colour& colour);

    /**
     * Gibt die Glow-Farbe zurück
     */
    juce::Colour getGlowColour() const { return glowColour; }

    /**
     * Setzt die Border-Thickness für Snap-Zone Previews
     */
    void setSnapZoneBorderThickness(float thickness);

    /**
     * Gibt die Border-Thickness zurück
     */
    float getSnapZoneBorderThickness() const { return snapZoneBorderThickness; }

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Callback-Typ für Snap-Zone-Aktivierung
     */
    using SnapZoneCallback = std::function<void(PanelSnapZone)>;

    /**
     * Setzt einen Callback, der bei Snap-Zone-Aktivierung aufgerufen wird
     */
    void setSnapZoneActivationCallback(SnapZoneCallback callback);

    /**
     * Callback-Typ für Feedback-Start
     */
    using FeedbackStartCallback = std::function<void(FeedbackType, juce::Component*)>;

    /**
     * Setzt einen Callback, der bei Feedback-Start aufgerufen wird
     */
    void setFeedbackStartCallback(FeedbackStartCallback callback);

    /**
     * Callback-Typ für Feedback-Ende
     */
    using FeedbackEndCallback = std::function<void(FeedbackType, juce::Component*)>;

    /**
     * Setzt einen Callback, der bei Feedback-Ende aufgerufen wird
     */
    void setFeedbackEndCallback(FeedbackEndCallback callback);

    //==============================================================================
    // JUCE Component Overrides
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Snap-Zonen
    std::unordered_map<PanelSnapZone, SnapZonePreview> snapZones;

    // Feedback-Animationen
    std::vector<std::unique_ptr<FeedbackAnimation>> feedbackAnimations;

    // Appearance
    juce::Colour snapZonePreviewColour = juce::Colour(0xFF4A90E2);
    juce::Colour snapZoneBorderColour = juce::Colour(0xFF5BA3F5);
    juce::Colour glowColour = juce::Colour(0xFFFFA500);
    float snapZoneBorderThickness = 2.0f;

    // Callbacks
    SnapZoneCallback snapZoneActivationCallback;
    FeedbackStartCallback feedbackStartCallback;
    FeedbackEndCallback feedbackEndCallback;

    // Active Snap Zone für Hover
    PanelSnapZone activeSnapZone = PanelSnapZone::None;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void timerCallback() override;

    void updateFeedbackAnimations();
    void applyFeedbackAnimation(FeedbackAnimation* animation);

    juce::Colour getFeedbackColour(FeedbackType type, FeedbackStyle style) const;
    juce::Colour getFeedbackGlowColour(FeedbackType type, FeedbackStyle style) const;

    void triggerSnapZoneActivationCallback(PanelSnapZone zone);
    void triggerFeedbackStartCallback(FeedbackType type, juce::Component* component);
    void triggerFeedbackEndCallback(FeedbackType type, juce::Component* component);

    void drawSnapZonePreview(juce::Graphics& g, const SnapZonePreview& preview);
    void drawFeedbackAnimation(juce::Graphics& g, const FeedbackAnimation& animation);
    void drawResizeIndicators(juce::Graphics& g, const juce::Rectangle<float>& bounds);

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelFeedbackSystem)
};
