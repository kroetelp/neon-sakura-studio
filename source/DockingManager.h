// ============================================================================
// DockingManager.h - Zentrale Verwaltung aller Panels und deren Status
// ============================================================================
//
// Der DockingManager ist das Herzstück des Docking-Systems.
//
// Verantwortlichkeiten:
// - Besitzt ALLE Panel-Instanzen (unique_ptr) - Panels werden NIE neu erstellt
// - Verwaltet Dock-Status und Floating-Windows
// - Koordiniert Layout-Updates in MainComponent
// - State-Persistenz (Layout speichern/laden)
//
// WICHTIG: Panels werden beim Undocken NICHT zerstört, sondern nur
// aus dem Parent entfernt und in ein FloatingWindow verschoben.

#pragma once

#include "DockablePanel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

// Forward declarations für Dependencies
class MainComponent;
class TimelineData;
class RecordingManager;
class WavetableEngine;
class WavetableParams;
class WavetableData;
class WavetableSynthEditor;
class FloatingPanelBase;
class ZoneSnapManager;

// Include für Enums (definiert in FloatingPanelEnums.h)
#include "UI/FloatingPanelEnums.h"
#include "UI/FloatingPanelBase.h"

// ============================================================================
/**
 * FloatingWindowContainer - Custom DocumentWindow für ausgedockte Panels
 *
 * Dieses Fenster:
 * - Hält eine NON-OWNING Referenz auf das DockablePanel
 * - Fängt close-Events und informiert den DockingManager
 * - Re-dockt das Panel standardmäßig statt es zu zerstören
 * - Speichert/Restoret Fensterposition
 */
class FloatingWindowContainer : public juce::DocumentWindow
{
public:
    FloatingWindowContainer(const juce::String& windowTitle,
                            DockablePanel* panelToContain,
                            bool shouldReDockOnClose = true);
    ~FloatingWindowContainer() override;

    // === juce::DocumentWindow Overrides ===
    void closeButtonPressed() override;
    void moved() override;
    void resized() override;

    // === Accessors ===
    DockablePanel* getPanel() const { return panel; }
    bool shouldReDockOnClose() const { return reDockOnClose; }
    void setReDockOnClose(bool shouldReDock) { reDockOnClose = shouldReDock; }

    // === Callbacks ===
    // Diese werden vom DockingManager gesetzt
    std::function<void(FloatingWindowContainer*)> onWindowCloseRequested;
    std::function<void(FloatingWindowContainer*)> onWindowMoved;
    std::function<void(FloatingWindowContainer*)> onWindowResized;

    // === Window State ===
    juce::ValueTree saveWindowState() const;
    void restoreWindowState(const juce::ValueTree& state);

    // === Look & Feel ===
    static constexpr int defaultTitleBarHeight = 28;

private:
    DockablePanel* panel;           // NON-OWNING! Panel gehört DockingManager
    bool reDockOnClose;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingWindowContainer)
};

// ============================================================================
/**
 * DockLayout - Beschreibt das aktuelle Layout der angedockten Panels
 */
struct DockLayout
{
    // Panels in verschiedenen Bereichen (geordnet nach Priorität)
    std::vector<PanelType> leftPanels;
    std::vector<PanelType> rightPanels;
    std::vector<PanelType> bottomPanels;
    std::vector<PanelType> centerTabs;

    // Aktuell sichtbarer Tab im Center
    PanelType activeCenterTab = PanelType::Timeline;

    // Größen der Bereiche (in Pixeln)
    int leftWidth = 0;
    int rightWidth = 320;
    int bottomHeight = 0;
    int centerMinWidth = 400;

    // Minimale Größen
    static constexpr int minSidebarWidth = 200;
    static constexpr int maxSidebarWidth = 600;
    static constexpr int minBottomHeight = 100;
    static constexpr int maxBottomHeight = 400;

    // === Serialization ===
    juce::ValueTree saveState() const;
    void restoreState(const juce::ValueTree& state);

    // === Helper ===
    bool hasLeftPanels() const { return !leftPanels.empty(); }
    bool hasRightPanels() const { return !rightPanels.empty(); }
    bool hasBottomPanels() const { return !bottomPanels.empty(); }
    bool hasCenterPanels() const { return !centerTabs.empty(); }
    void clear();
};

// ============================================================================
/**
 * DockingManager - Zentrale Verwaltungsklasse
 *
 * Usage:
 *   1. DockingManager erstellen
 *   2. Panels registrieren mit registerPanel()
 *   3. setMainComponent() aufrufen
 *   4. Panel-Dependencies setzen (z.B. setTimelineDependencies)
 *   5. Layout restoren oder Default-Layout anwenden
 */
class DockingManager
{
public:
    DockingManager();
    ~DockingManager();

    // === Panel Registration ===

    // Registriert ein Panel. Der DockingManager übernimmt Ownership.
    // Wird typischerweise beim Initialisieren aufgerufen.
    void registerPanel(std::unique_ptr<DockablePanel> panel);

    // Panel abfragen
    DockablePanel* getPanel(PanelType type) const;
    bool hasPanel(PanelType type) const;

    // Typ-sicherer Cast
    template<typename T>
    T* getPanelAs(PanelType type) const
    {
        return dynamic_cast<T*>(getPanel(type));
    }

    // Alle registrierten PanelTypes abrufen
    std::vector<PanelType> getRegisteredPanelTypes() const;

    // === Visibility & Dock State ===

    // Panel sichtbar/unsichtbar schalten
    void setPanelVisible(PanelType type, bool visible);
    bool isPanelVisible(PanelType type) const;

    // Panel andocken (Floating -> Docked)
    void dockPanel(PanelType type, DockPosition position = DockPosition::Right);

    // Panel ausdocken (Docked -> Floating)
    void undockPanel(PanelType type);

    // Panel zwischen Docked/Floating togglen
    void togglePanelDockState(PanelType type);

    // Panel komplett schließen (Hidden)
    void closePanel(PanelType type);

    // === Layout Management ===

    // Aktuelles Layout abrufen
    const DockLayout& getCurrentLayout() const { return currentLayout; }
    DockLayout& getCurrentLayout() { return currentLayout; }

    // === NEU: Layout Update für MainComponent ===
    // Wird von MainComponent::resized() aufgerufen
    // Setzt die Bounds für alle angedockten Panels basierend auf dem availableArea
    void updateDockedLayout(const juce::Rectangle<int>& availableArea);

    // Berechnet die Bounds für ein spezifisches Panel
    juce::Rectangle<int> calculatePanelBounds(PanelType type, const juce::Rectangle<int>& totalArea) const;

    // Layout manuell ändern
    void setLeftWidth(int width);
    void setRightWidth(int width);
    void setBottomHeight(int height);

    // Panel zu einem Bereich hinzufügen/entfernen
    void addPanelToLayout(PanelType type, DockPosition position);
    void removePanelFromLayout(PanelType type);

    // === Center Tab Management ===
    void setActiveCenterTab(PanelType type);
    PanelType getActiveCenterTab() const { return currentLayout.activeCenterTab; }
    bool isCenterTabActive(PanelType type) const;

    // === MainComponent Integration ===

    // Setzt die MainComponent-Referenz (required für Layout-Updates)
    void setMainComponent(MainComponent* mainComp);
    MainComponent* getMainComponent() const { return mainComponent; }

    // Löst ein Layout-Update in der MainComponent aus
    void triggerLayoutUpdate();

    // ============================================================
    // NEU: Floating Panel Features
    // ============================================================

    // ZoneSnapManager Integration
    void setZoneSnapManager(ZoneSnapManager* manager);
    ZoneSnapManager* getZoneSnapManager() const { return zoneSnapManager; }

    // Panel State (Floating, Snapped, Minimized, Collapsed, Hidden)
    void setPanelState(PanelType type, PanelState state);
    PanelState getPanelState(PanelType type) const;

    // Panel Size Mode (Compact, Standard, Expanded, Full)
    void setPanelSizeMode(PanelType type, PanelSizeMode mode);
    PanelSizeMode getPanelSizeMode(PanelType type) const;

    // === State Persistence ===
    juce::ValueTree saveLayoutState() const;
    void restoreLayoutState(const juce::ValueTree& state);

    // === Bulk Operations ===
    void closeAllFloatingWindows();
    void hideAllPanels();
    void resetToDefaultLayout();

    // === Dependencies für spezifische Panels ===
    // Diese werden von der MainComponent gesetzt, bevor Panels angezeigt werden

    // Timeline
    void setTimelineDependencies(TimelineData* data, RecordingManager* recorder);

    // Wavetable
    void setWavetableEngine(WavetableEngine* engine);
    void openTrackWavetableEditor(int trackIndex,
                                   std::shared_ptr<WavetableParams> params,
                                   std::shared_ptr<WavetableData> wavetableData);
    void closeTrackWavetableEditor();
    bool isTrackWavetableEditorOpen() const;
    int getCurrentEditingTrack() const;

    // Context Update - Aktualisiert Panels basierend auf Track-Auswahl
    void updatePanelsForTrack(int trackIndex);

    // === Floating Window Management ===
    FloatingWindowContainer* getFloatingWindow(PanelType type) const;
    bool hasFloatingWindow(PanelType type) const;
    int getNumFloatingWindows() const;

    // === Utility ===
    static juce::String panelTypeToString(PanelType type);
    static PanelType stringToPanelType(const juce::String& str);

private:
    // Panel Storage - EINE Instanz pro Panel-Typ
    std::unordered_map<PanelType, std::unique_ptr<DockablePanel>> panels;

    // Floating Windows - separater Storage
    std::unordered_map<PanelType, std::unique_ptr<FloatingWindowContainer>> floatingWindows;

    // Aktuelles Layout
    DockLayout currentLayout;

    // MainComponent Reference (non-owning)
    MainComponent* mainComponent = nullptr;

    // Track-Wavetable Editor (special floating window for per-track editing)
    std::unique_ptr<WavetableSynthEditor> trackWavetableEditor;
    std::unique_ptr<juce::DocumentWindow> trackWavetableWindow;
    int currentEditingTrack = -1;
    std::shared_ptr<WavetableParams> currentTrackParams;
    std::shared_ptr<WavetableData> currentTrackWavetableData;

    // NEU: ZoneSnapManager für Floating Panel Snapping
    ZoneSnapManager* zoneSnapManager = nullptr;

    // NEU: Extended Panel States
    std::unordered_map<PanelType, PanelState> panelStates;
    std::unordered_map<PanelType, PanelSizeMode> panelSizeModes;

    // NEU: Panel Grouping (verbundene Panels)
    std::unordered_map<juce::String, std::vector<PanelType>> panelGroups;

    // === Internal Helper ===
    void createFloatingWindow(PanelType type);
    void destroyFloatingWindow(PanelType type);

    void addPanelToParent(PanelType type);
    void removePanelFromParent(PanelType type);

    void updatePanelCallbacks(PanelType type);

    void broadcastLayoutChanged();

    // Verhindert rekursive Layout-Updates
    bool isUpdatingLayout = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockingManager)
};
