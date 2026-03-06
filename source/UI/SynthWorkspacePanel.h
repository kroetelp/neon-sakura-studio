// ============================================================================
// SynthWorkspacePanel.h - Synthesizer-Workspace mit drei Modi
// ============================================================================
//
// Das SynthWorkspacePanel ist der zentrale Container für alle Synthesis-
// und Mixing-bezogenen Panels.
//
// Features:
// - Drei Modi: Perform, Design, Patch
// - Integrat die RoutingMatrix und MasterBus Panels
// - Bietet eine einheitliche Oberfläche für alle Audio-Routing-Funktionen
// - Unterstützt Docking, Floating und Minimized States

#pragma once

#include "../DockablePanel.h"
#include "./FloatingPanelEnums.h"
#include <memory>
#include <functional>
#include <vector>

// Forward Declarations
class WavetableEngine;
class RoutingMatrixPanel;
class MasterBusPanel;
class AudioRoutingGraph;
class juce::TabbedComponent;
class juce::StretchableLayoutManager;

// ============================================================================
/**
 * SynthWorkspacePanel - Synthesizer-Workspace
 *
 * Layout-Struktur:
 * ┌─────────────────────────────────────────────────┐
 * │ [Synth] [Routing] [Master] ← Tabs             │
 * ├─────────────────────────────────────────────────┤
 * │                                                 │
 * │            TAB CONTENT (dynamisch)               │
 │                                                 │
 * │   - Synth Tab: Wavetable-Editor                 │
 * │   - Routing Tab: RoutingMatrixPanel             │
 * │   - Master Tab: MasterBusPanel                 │
 │                                                 │
 * └─────────────────────────────────────────────────┘
 */
class SynthWorkspacePanel : public DockablePanel, private juce::Timer
{
public:
    //==============================================================================
    // Tab-Konfiguration
    //==============================================================================
    enum class WorkspaceTab
    {
        Synth = 0,      // Wavetable Synth Editor
        Routing = 1,    // Routing Matrix
        Master = 2      // Master Bus
    };

    //==============================================================================
    // Konstruktor & Destruktor
    //==============================================================================
    SynthWorkspacePanel();
    ~SynthWorkspacePanel() override;

    //==============================================================================
    // Engine Setup
    //==============================================================================
    void setEngine(WavetableEngine* engine);

    //==============================================================================
    // Synth Mode (Perform, Design, Patch)
    //==============================================================================
    void setSynthMode(SynthMode mode);
    SynthMode getSynthMode() const { return currentMode; }

    //==============================================================================
    // Tab Management
    //==============================================================================
    void setActiveTab(WorkspaceTab tab);
    WorkspaceTab getActiveTab() const { return currentTab; }

    void showSynthTab();
    void showRoutingTab();
    void showMasterTab();

    //==============================================================================
    // Panel Access
    //==============================================================================
    RoutingMatrixPanel* getRoutingMatrixPanel() { return routingMatrixPanel; }
    MasterBusPanel* getMasterBusPanel() { return masterBusPanel; }

    // Setter für Panels (werden von MainComponentExtension aufgerufen)
    void setRoutingMatrixPanel(RoutingMatrixPanel* panel);
    void setMasterBusPanel(MasterBusPanel* panel);

    // Methode zum direkten Setzen beider Panels ohne setupTabs() aufzurufen
    // Dies wird während der Initialisierung verwendet, um rekursive Aufrufe zu vermeiden
    void setPanelsDirect(RoutingMatrixPanel* routing, MasterBusPanel* master);

    // Öffentliche Methode, um setupTabs() aufzurufen (wird während Initialisierung verwendet)
    void refreshTabs();

    //==============================================================================
    // Track Context
    //==============================================================================
    void setTargetTrack(int trackIndex);
    int getCurrentTrack() const { return currentTrack; }

    //==============================================================================
    // Visibility Control
    //==============================================================================
    void setPanelVisible(WorkspaceTab tab, bool visible);
    bool isPanelVisible(WorkspaceTab tab) const;

    //==============================================================================
    // Callbacks
    //==============================================================================
    std::function<void()> onModeChanged;
    std::function<void(WorkspaceTab)> onTabChanged;
    std::function<void(const juce::String&, bool)> onPanelVisibilityChanged;

    //==============================================================================
    // JUCE Component Overrides
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

protected:
    //==============================================================================
    // DockablePanel Overrides
    //==============================================================================
    void initializeContent() override;

private:
    //==============================================================================
    // Dependencies
    //==============================================================================
    WavetableEngine* wavetableEngine = nullptr;

    //==============================================================================
    // Panels (NON-OWNING Zeiger - Panels gehören dem DockingManager)
    //==============================================================================
    RoutingMatrixPanel* routingMatrixPanel = nullptr;
    MasterBusPanel* masterBusPanel = nullptr;

    // Note: Wavetable-Panel ist bereits separat registriert im DockingManager
    // und wird hier nur als Referenz verwaltet, wenn nötig

    //==============================================================================
    // Tab Component
    //==============================================================================
    std::unique_ptr<juce::TabbedComponent> tabbedComponent;

    //==============================================================================
    // Placeholder Components (für Memory Leak Prevention)
    //==============================================================================
    std::unique_ptr<juce::Component> synthPlaceholder;
    std::unique_ptr<juce::Component> routingPlaceholder;
    std::unique_ptr<juce::Component> masterPlaceholder;

    //==============================================================================
    // Layout Manager
    //==============================================================================
    std::unique_ptr<juce::StretchableLayoutManager> layoutManager;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerBar;

    //==============================================================================
    // State
    //==============================================================================
    SynthMode currentMode = SynthMode::Perform;
    WorkspaceTab currentTab = WorkspaceTab::Synth;
    int currentTrack = 0;  // Aktueller Track-Index für Panel-Updates

    std::unordered_map<WorkspaceTab, bool> tabVisibility;

    // Guards um rekursive Aufrufe zu verhindern
    bool isSettingUpTabs = false;
    bool tabsInitialized = false;  // Track ob setupTabs() bereits aufgerufen wurde
    bool isResizing = false;  // Guard für resized()
    bool isPainting = false;  // Guard für paint()

    //==============================================================================
    // Layout Konstanten
    //==============================================================================
    static constexpr int tabBarHeight = 32;
    static constexpr int headerHeight = 40;
    static constexpr int minPanelWidth = 300;
    static constexpr int minPanelHeight = 200;

    //==============================================================================
    // Helper Methods
    //==============================================================================
    void createTabComponent();
    void setupTabs();
    void connectPanelCallbacks();

    void drawModeIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawTabContent(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    juce::String getTabName(WorkspaceTab tab) const;
    juce::Colour getTabColour(WorkspaceTab tab) const;

    void updatePanelVisibility();

    //==============================================================================
    // Leak Detector
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthWorkspacePanel)
};
