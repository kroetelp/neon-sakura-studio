// ============================================================================
// MainComponentExtension.h - Floating Workspace Layout Integration
// ============================================================================
//
// Diese Klasse erweitert die MainComponent mit dem neuen Docking-System.
// Sie verwaltet:
// - Das ExtendedThemeSystem für erweiterte Themes
// - Die Registrierung und Integration aller neuen Panels
// - Die Verbindung zwischen MainComponent und DockingManager
//
// Die MainComponentExtension wird von MainComponent als Member verwendet
// und sorgt dafür, dass alle neuen UI-Elemente (RoutingMatrix, MasterBus, etc.)
// korrekt initialisiert und angezeigt werden.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

// Forward Declarations
class MainComponent;
class DockingManager;
class AudioRoutingGraph;
class WavetableEngine;
class AudioEngine;
class TrackManager;
class TimelineData;
class RecordingManager;
class ExtendedThemeSystem;
class SynthWorkspacePanel;
class RoutingMatrixPanel;
class MasterBusPanel;
class ZoneSnapManager;

// ============================================================================
/**
 * MainComponentExtension - Erweiterung für MainComponent
 *
 * Diese Klasse bietet:
 * - Theme-System Integration
 * - Docking-Panel Registrierung und Management
 * - Layout-Konfiguration für den Workspace
 * - Callbacks für Panel-Interaktionen
 *
 * Usage:
 *   1. MainComponentExtension in MainComponent erstellen
 *   2. Dependencies setzen (AudioEngine, etc.)
 *   3. initialize() aufrufen
 *   4. Panels werden automatisch registriert und angezeigt
 */
class MainComponentExtension : public juce::Component, private juce::Timer
{
public:
    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    MainComponentExtension();
    ~MainComponentExtension() override;

    //==============================================================================
    // Initialisierung
    //==============================================================================

    /**
     * Initialisiert die Extension mit allen nötigen Dependencies
     */
    void initialize(MainComponent* mainComp,
                   AudioEngine* audioEngine,
                   DockingManager* dockingManager);

    /**
     * Erstellt und registriert alle neuen Panels
     */
    void createAndRegisterPanels();

    /**
     * Konfiguriert das Standard-Layout
     */
    void setupDefaultLayout();

    //==============================================================================
    // Theme System
    //==============================================================================

    /**
     * Gibt das ExtendedThemeSystem zurück
     */
    ExtendedThemeSystem& getThemeSystem() { return *themeSystem; }
    const ExtendedThemeSystem& getThemeSystem() const { return *themeSystem; }

    /**
     * Ändert das aktive Theme
     */
    void setTheme(int themeIndex);

    /**
     * Gibt die Anzahl der verfügbaren Themes zurück
     */
    int getNumThemes() const;

    /**
     * Gibt den Namen eines Themes zurück
     */
    juce::String getThemeName(int index) const;

    //==============================================================================
    // Panel Access
    //==============================================================================

    /**
     * Gibt das SynthWorkspacePanel zurück
     */
    SynthWorkspacePanel* getSynthWorkspace() { return synthWorkspacePanel; }

    /**
     * Gibt das RoutingMatrixPanel zurück
     */
    RoutingMatrixPanel* getRoutingMatrix() { return routingMatrixPanel; }

    /**
     * Gibt das MasterBusPanel zurück
     */
    MasterBusPanel* getMasterBusPanel() { return masterBusPanel; }

    //==============================================================================
    // Layout Konfiguration
    //==============================================================================

    /**
     * Zeigt oder versteckt die Synth-Workspace-Panels
     */
    void setSynthWorkspaceVisible(bool visible);

    /**
     * Zeigt oder versteckt die Mixing-Panels (Routing, Master)
     */
    void setMixingPanelsVisible(bool visible);

    /**
     * Toggled zwischen verschiedenen Workspace-Konfigurationen
     */
    enum class WorkspaceMode
    {
        Full,       // Alle Panels sichtbar
        SynthOnly,  // Nur Synth-Workspace
        MixingOnly, // Nur Mixing-Tools
        Minimal     // Minimal Layout
    };

    void setWorkspaceMode(WorkspaceMode mode);
    WorkspaceMode getWorkspaceMode() const { return currentWorkspaceMode; }

    //==============================================================================
    // State Persistence
    //==============================================================================

    /**
     * Speichert den aktuellen Workspace-State
     */
    juce::ValueTree saveState() const;

    /**
     * Stellt den Workspace-State wieder her
     */
    void restoreState(const juce::ValueTree& state);

    //==============================================================================
    // Callbacks
    //==============================================================================

    /**
     * Callback, der aufgerufen wird, wenn das Workspace-Layout geändert wird
     */
    std::function<void()> onLayoutChanged;

    /**
     * Callback, der aufgerufen wird, wenn ein Panel angezeigt/versteckt wird
     */
    std::function<void(const juce::String&, bool)> onPanelVisibilityChanged;

    //==============================================================================
    // JUCE Component Overrides
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    //==============================================================================
    // Dependencies
    //==============================================================================
    MainComponent* mainComponent = nullptr;
    AudioEngine* audioEngine = nullptr;
    DockingManager* dockingManager = nullptr;
    AudioRoutingGraph* audioRoutingGraph = nullptr;

    //==============================================================================
    // Theme System
    //==============================================================================
    std::unique_ptr<ExtendedThemeSystem> themeSystem;

    //==============================================================================
    // Panels (NON-OWNING Zeiger - Panels gehören dem DockingManager)
    //==============================================================================
    SynthWorkspacePanel* synthWorkspacePanel = nullptr;
    RoutingMatrixPanel* routingMatrixPanel = nullptr;
    MasterBusPanel* masterBusPanel = nullptr;

    //==============================================================================
    // Layout State
    //==============================================================================
    WorkspaceMode currentWorkspaceMode = WorkspaceMode::Full;
    bool synthWorkspaceVisible = true;
    bool mixingPanelsVisible = true;

    // Guards um rekursive Aufrufe zu verhindern
    bool isInitializing = false;
    bool isUpdatingLayout = false;
    bool isUpdatingVisibility = false;
    bool isCreatingPanels = false;  // Guard für createAndRegisterPanels()

    //==============================================================================
    // Helper Methods
    //==============================================================================
    void initializeThemeSystem();
    void connectPanelCallbacks();
    void setupZoneSnapManager();

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponentExtension)
};
