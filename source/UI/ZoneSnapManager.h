// ============================================================================
// ZoneSnapManager.h - Zone Snap Manager für Floating Panels
// ============================================================================

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "FloatingPanelEnums.h"
#include <memory>
#include <functional>

// Forward Declarations
class FloatingPanelBase;

/**
 * ZoneSnapManager - Verwaltung der Snap-Zonen für Floating Panels
 *
 * Diese Klasse verwaltet die verschiedenen Zonen im UI, in die Panels
 * gesnappt werden können. Sie bietet Methoden zum Erkennen von Zonen,
 * zum Snappen von Panels und zum Aktualisieren der Zone-Bounds.
 *
 * Features:
 * - Zone Detection (Erkennt, über welcher Zone ein Panel schwebt)
 * - Panel Snapping (Snappt Panels an Zonen)
 * - Zone Bounds Management (Verwaltet die Bounds aller Zonen)
 * - Visual Feedback (Zeigt Snap-Zonen während des Draggings)
 * - Zone Resize (Passt Zonen bei Window-Resize an)
 */
class ZoneSnapManager
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    ZoneSnapManager();
    ~ZoneSnapManager();

    //==============================================================================
    // Zone Bounds Management
    //==============================================================================
    /**
     * Aktualisiert die Bounds aller Snap-Zonen basierend auf der angegebenen Bounds
     * @param containerBounds Die Bounds des Container-Komponente
     */
    void updateZoneBounds(const juce::Rectangle<int>& containerBounds);

    /**
     * Aktualisiert die Zone-Bounds für eine spezifische Zone
     * @param zone Die zu aktualisierende Zone
     * @param bounds Die neuen Bounds für die Zone
     */
    void updateZoneBounds(PanelSnapZone zone, const juce::Rectangle<int>& bounds);

    /**
     * Gibt die Bounds einer spezifischen Zone zurück
     * @param zone Die Zone
     * @return Die Bounds der Zone
     */
    juce::Rectangle<int> getZoneBounds(PanelSnapZone zone) const;

    /**
     * Gibt alle Zone-Bounds zurück
     */
    std::unordered_map<PanelSnapZone, juce::Rectangle<int>> getAllZoneBounds() const { return zoneBounds; }

    //==============================================================================
    // Zone Detection
    //==============================================================================
    /**
     * Erkennt, über welcher Zone der gegebene Punkt liegt
     * @param point Der zu prüfende Punkt
     * @return Die Zone oder PanelSnapZone::None
     */
    PanelSnapZone detectZone(const juce::Point<int>& point) const;

    /**
     * Erkennt, über welcher Zone die gegebene Component liegt
     * @param component Die zu prüfende Component
     * @return Die Zone oder PanelSnapZone::None
     */
    PanelSnapZone detectZoneForComponent(const juce::Component& component) const;

    /**
     * Prüft, ob ein Punkt innerhalb einer Zone liegt
     * @param point Der zu prüfende Punkt
     * @param zone Die Zone
     * @return true wenn der Punkt in der Zone liegt
     */
    bool isPointInZone(const juce::Point<int>& point, PanelSnapZone zone) const;

    //==============================================================================
    // Snapping
    //==============================================================================
    /**
     * Snapped ein Panel an eine Zone
     * @param panel Das Panel
     * @param zone Die Ziel-Zone
     * @param animate Ob das Snappen animiert werden soll
     * @return true wenn das Panel erfolgreich gesnappt wurde
     */
    bool snapPanelToZone(FloatingPanelBase* panel, PanelSnapZone zone, bool animate = true);

    /**
     * Entfernt ein Panel aus einer Zone (Unsnap)
     * @param panel Das Panel
     * @param animate Ob das Unsnap animiert werden soll
     * @return true wenn das Panel erfolgreich unsnapped wurde
     */
    bool unsnapPanel(FloatingPanelBase* panel, bool animate = true);

    /**
     * Gibt die Zone zurück, in der ein Panel aktuell gesnappt ist
     * @param panel Das Panel
     * @return Die Zone oder PanelSnapZone::None
     */
    PanelSnapZone getPanelZone(const FloatingPanelBase* panel) const;

    //==============================================================================
    // Zone Configuration
    //==============================================================================
    /**
     * Aktiviert oder deaktiviert eine Zone
     * @param zone Die Zone
     * @param enabled true um die Zone zu aktivieren
     */
    void setZoneEnabled(PanelSnapZone zone, bool enabled);

    /**
     * Prüft, ob eine Zone aktiviert ist
     * @param zone Die Zone
     * @return true wenn die Zone aktiviert ist
     */
    bool isZoneEnabled(PanelSnapZone zone) const;

    /**
     * Setzt die Sensitivität für Zone-Detection (in Pixel)
     * @param sensitivity Die Sensitivität (Standard: 20)
     */
    void setSnapSensitivity(int sensitivity);

    /**
     * Gibt die aktuelle Sensitivität zurück
     */
    int getSnapSensitivity() const { return snapSensitivity; }

    //==============================================================================
    // Visual Feedback
    //==============================================================================
    /**
     * Aktiviert oder deaktiviert Visual Feedback
     * @param enabled true um Visual Feedback zu aktivieren
     */
    void setVisualFeedbackEnabled(bool enabled);

    /**
     * Prüft, ob Visual Feedback aktiviert ist
     */
    bool isVisualFeedbackEnabled() const { return visualFeedbackEnabled; }

    /**
     * Zeichnet die Snap-Zonen für Visual Feedback
     * @param g Graphics Context
     */
    void paintZoneFeedback(juce::Graphics& g) const;

    /**
     * Setzt die aktuell hervorgehobene Zone
     * @param zone Die Zone oder PanelSnapZone::None
     */
    void setHighlightedZone(PanelSnapZone zone);

    /**
     * Gibt die aktuell hervorgehobene Zone zurück
     */
    PanelSnapZone getHighlightedZone() const { return highlightedZone; }

    //==============================================================================
    // Zone Padding & Margins
    //==============================================================================
    /**
     * Setzt den Padding für alle Zonen
     * @param padding Der Padding in Pixel
     */
    void setZonePadding(int padding);

    /**
     * Setzt den Padding für eine spezifische Zone
     * @param zone Die Zone
     * @param padding Der Padding in Pixel
     */
    void setZonePadding(PanelSnapZone zone, int padding);

    /**
     * Gibt den Padding einer Zone zurück
     */
    int getZonePadding(PanelSnapZone zone) const;

    //==============================================================================
    // Callbacks
    //==============================================================================
    /**
     * Callback-Typ für Zone-Änderungen
     */
    using ZoneChangeCallback = std::function<void(FloatingPanelBase*, PanelSnapZone, PanelSnapZone)>;

    /**
     * Setzt einen Callback, der aufgerufen wird, wenn ein Panel zwischen Zonen wechselt
     */
    void setZoneChangeCallback(ZoneChangeCallback callback);

    //==============================================================================
    // Utility
    //==============================================================================
    /**
     * Gibt den Namen einer Zone als String zurück
     */
    static juce::String getZoneName(PanelSnapZone zone);

    /**
     * Gibt eine Zone anhand ihres Namens zurück
     */
    static PanelSnapZone getZoneFromName(const juce::String& name);

    /**
     * Löscht alle gespeicherten Panel-Zuordnungen
     */
    void clearPanelMappings();

private:
    //==============================================================================
    // Private Member
    //==============================================================================
    // Zone Bounds
    std::unordered_map<PanelSnapZone, juce::Rectangle<int>> zoneBounds;
    std::unordered_map<PanelSnapZone, int> zonePadding;
    std::unordered_map<PanelSnapZone, bool> zoneEnabled;

    // Panel-Zuordnung
    std::unordered_map<const FloatingPanelBase*, PanelSnapZone> panelZoneMap;

    // Konfiguration
    int snapSensitivity = 20;
    bool visualFeedbackEnabled = true;
    PanelSnapZone highlightedZone = PanelSnapZone::None;

    // Callback
    ZoneChangeCallback zoneChangeCallback;

    //==============================================================================
    // Private Methods
    //==============================================================================
    void initializeZonePaddings();
    void initializeZoneEnabled();
    void calculateZoneBounds(const juce::Rectangle<int>& containerBounds);
    void triggerZoneChangeCallback(FloatingPanelBase* panel, PanelSnapZone oldZone, PanelSnapZone newZone);

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZoneSnapManager)
};
