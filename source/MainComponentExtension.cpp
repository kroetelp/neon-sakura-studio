// ============================================================================
// MainComponentExtension.cpp - Floating Workspace Layout Implementation
// ============================================================================

#include "MainComponentExtension.h"
#include "MainComponent.h"
#include "AudioEngine.h"
#include "AudioRouting/AudioRoutingGraph.h"
#include "DockingManager.h"
#include "Theme/ExtendedThemeSystem.h"
#include "UI/SynthWorkspacePanel.h"
#include "UI/RoutingMatrixPanel.h"
#include "UI/MasterBusPanel.h"
#include "UI/FloatingPanelEnums.h"
#include "WavetableSynth/WavetableEngine.h"
#include "Theme/ThemeManager.h"
#include <algorithm>

//==============================================================================
MainComponentExtension::MainComponentExtension()
{
    // Theme-System initialisieren
    themeSystem = std::make_unique<ExtendedThemeSystem>();
}

MainComponentExtension::~MainComponentExtension()
{
    stopTimer();
}

//==============================================================================
void MainComponentExtension::initialize(MainComponent* mainComp,
                                      AudioEngine* engine,
                                      DockingManager* manager)
{
    // Guard: Verhindere rekursive Aufrufe von initialize()
    if (isInitializing)
        return;

    juce::ScopedValueSetter<bool> guard(isInitializing, true);

    mainComponent = mainComp;
    audioEngine = engine;
    dockingManager = manager;

    // Audio-Routing-Graph Referenz holen
    if (audioEngine)
        audioRoutingGraph = audioEngine->getAudioRoutingGraph();

    // Theme-System initialisieren
    initializeThemeSystem();

    // Panels erstellen und registrieren
    createAndRegisterPanels();

    // Standard-Layout einrichten
    setupDefaultLayout();

    // Callbacks verbinden
    connectPanelCallbacks();

    // Timer für Layout-Updates starten (30fps)
    startTimerHz(30);
}

//==============================================================================
void MainComponentExtension::initializeThemeSystem()
{
    if (!themeSystem)
        return;

    // Das Standard-Theme basierend auf der aktuellen Konfiguration wählen
    themeSystem->applyNeonSakuraTheme();

    // Theme-Change Callback für Repaints
    themeSystem->setThemeChangeCallback([this](auto) {
        if (mainComponent)
            mainComponent->repaint();
        repaint();
    });
}

//==============================================================================
void MainComponentExtension::createAndRegisterPanels()
{
    // Guard: Verhindere rekursive Aufrufe von createAndRegisterPanels()
    if (isCreatingPanels)
        return;

    juce::ScopedValueSetter<bool> guard(isCreatingPanels, true);

    if (!dockingManager || !audioRoutingGraph)
        return;

    // === SynthWorkspacePanel ===
    auto synthPanel = std::make_unique<SynthWorkspacePanel>();
    if (audioEngine)
        synthPanel->setEngine(&audioEngine->getWavetableEngine());
    dockingManager->registerPanel(std::move(synthPanel));

    // Hole die Referenz zurück (das Panel gehört nun dem DockingManager)
    synthWorkspacePanel = dockingManager->getPanelAs<SynthWorkspacePanel>(PanelType::WavetableSynth);

    // === RoutingMatrixPanel ===
    auto routingPanel = std::make_unique<RoutingMatrixPanel>(*audioRoutingGraph);
    dockingManager->registerPanel(std::move(routingPanel));

    // Hole die Referenz zurück
    routingMatrixPanel = dockingManager->getPanelAs<RoutingMatrixPanel>(PanelType::RoutingMatrix);

    // === MasterBusPanel ===
    auto masterPanel = std::make_unique<MasterBusPanel>(*audioRoutingGraph);
    dockingManager->registerPanel(std::move(masterPanel));

    // Hole die Referenz zurück
    masterBusPanel = dockingManager->getPanelAs<MasterBusPanel>(PanelType::MasterBus);

    // WICHTIG: Panels an SynthWorkspacePanel übergeben
    // Dies verhindert Null-Pointer-Abstürze beim Tab-Setup
    // Wir nutzen die neue setPanelsDirect Methode, die setupTabs() nicht aufruft
    if (synthWorkspacePanel)
    {
        synthWorkspacePanel->setPanelsDirect(routingMatrixPanel, masterBusPanel);

        // Tabs neu erstellen (via öffentliche refreshTabs Methode)
        synthWorkspacePanel->refreshTabs();
    }
}

//==============================================================================
void MainComponentExtension::setupDefaultLayout()
{
    // Guard: Verhindere rekursive Aufrufe von setupDefaultLayout()
    if (isUpdatingLayout)
        return;

    juce::ScopedValueSetter<bool> guard(isUpdatingLayout, true);

    if (!dockingManager)
        return;

    auto& layout = dockingManager->getCurrentLayout();

    // Synth-Workspace in den linken Bereich
    layout.leftPanels.clear();
    layout.leftPanels.push_back(PanelType::WavetableSynth);
    layout.leftWidth = 350;

    // Mixing-Panels in den rechten Bereich
    layout.rightPanels.clear();
    layout.rightPanels.push_back(PanelType::RoutingMatrix);  // RoutingMatrix
    layout.rightWidth = 320;

    // Master-Bus unten
    layout.bottomPanels.clear();
    layout.bottomPanels.push_back(PanelType::MasterBus);
    layout.bottomHeight = 150;

    // Panels sichtbar machen
    if (synthWorkspacePanel)
        dockingManager->setPanelVisible(PanelType::WavetableSynth, true);

    if (routingMatrixPanel)
        dockingManager->setPanelVisible(PanelType::RoutingMatrix, true);

    if (masterBusPanel)
        dockingManager->setPanelVisible(PanelType::MasterBus, true);

    // Layout anwenden
    dockingManager->addPanelToLayout(PanelType::WavetableSynth, DockPosition::Left);
    dockingManager->addPanelToLayout(PanelType::RoutingMatrix, DockPosition::Right);
    dockingManager->addPanelToLayout(PanelType::MasterBus, DockPosition::Bottom);
}

//==============================================================================
void MainComponentExtension::connectPanelCallbacks()
{
    // RoutingMatrixPanel Callbacks
    if (routingMatrixPanel)
    {
        routingMatrixPanel->onConnectionAdded = [this](int connectionId) {
            juce::ignoreUnused(connectionId);
            // Audio-Graph aktualisieren wenn nötig
        };

        routingMatrixPanel->onConnectionRemoved = [this](int connectionId) {
            juce::ignoreUnused(connectionId);
            // Audio-Graph aktualisieren wenn nötig
        };
    }

    // MasterBusPanel Callbacks
    if (masterBusPanel)
    {
        masterBusPanel->onExportClicked = [this]() {
            // Export-Dialog zeigen
            if (mainComponent)
            {
                // MainComponent->showExportDialog();
            }
        };

        masterBusPanel->onRenderClicked = [this]() {
            // Render starten
            if (mainComponent)
            {
                // MainComponent->startRender();
            }
        };

        masterBusPanel->setLevelUpdateCallback([this](float left, float right) {
            // Level-Updates an AudioEngine weitergeben
            if (audioRoutingGraph)
                juce::ignoreUnused(left, right);
        });
    }

    // SynthWorkspacePanel Callbacks
    if (synthWorkspacePanel)
    {
        synthWorkspacePanel->onPanelVisibilityChanged = [this](const juce::String& panelName, bool visible) {
            if (onPanelVisibilityChanged)
                onPanelVisibilityChanged(panelName, visible);
        };
    }
}

//==============================================================================
void MainComponentExtension::setupZoneSnapManager()
{
    // ZoneSnapManager Integration für Floating Panels
    // Dies kann in einer späteren Phase implementiert werden
}

//==============================================================================
// Theme System Methods
//==============================================================================

void MainComponentExtension::setTheme(int themeIndex)
{
    if (!themeSystem)
        return;

    switch (themeIndex)
    {
        case 0:
            themeSystem->applyNeonSakuraTheme();
            break;
        case 1:
            themeSystem->applyGlassUITheme();
            break;
        case 2:
            themeSystem->applyCyberpunkTheme();
            break;
        case 3:
            themeSystem->applyProfessionalTheme();
            break;
        case 4:
            themeSystem->applyMinimalTheme();
            break;
        case 5:
            themeSystem->applyLiquidTheme();
            break;
        default:
            themeSystem->applyNeonSakuraTheme();
            break;
    }

    repaint();
}

int MainComponentExtension::getNumThemes() const
{
    return 6;  // NeonSakura, GlassUI, Cyberpunk, Professional, Minimal, Liquid
}

juce::String MainComponentExtension::getThemeName(int index) const
{
    switch (index)
    {
        case 0: return "Neon Sakura";
        case 1: return "Glass UI";
        case 2: return "Cyberpunk";
        case 3: return "Professional";
        case 4: return "Minimal";
        case 5: return "Liquid";
        default: return "Unknown";
    }
}

//==============================================================================
// Panel Visibility Methods
//==============================================================================

void MainComponentExtension::setSynthWorkspaceVisible(bool visible)
{
    // Guard: Verhindere rekursive Aufrufe
    if (isUpdatingVisibility)
        return;

    juce::ScopedValueSetter<bool> guard(isUpdatingVisibility, true);

    synthWorkspaceVisible = visible;

    if (synthWorkspacePanel && dockingManager)
    {
        dockingManager->setPanelVisible(PanelType::WavetableSynth, visible);
    }

    if (onPanelVisibilityChanged)
        onPanelVisibilityChanged("Synth Workspace", visible);

    if (onLayoutChanged)
        onLayoutChanged();
}

void MainComponentExtension::setMixingPanelsVisible(bool visible)
{
    // Guard: Verhindere rekursive Aufrufe
    if (isUpdatingVisibility)
        return;

    juce::ScopedValueSetter<bool> guard(isUpdatingVisibility, true);

    mixingPanelsVisible = visible;

    if (routingMatrixPanel && dockingManager)
        dockingManager->setPanelVisible(PanelType::RoutingMatrix, visible);

    if (masterBusPanel && dockingManager)
        dockingManager->setPanelVisible(PanelType::MasterBus, visible);

    if (onPanelVisibilityChanged)
        onPanelVisibilityChanged("Mixing Panels", visible);

    if (onLayoutChanged)
        onLayoutChanged();
}

void MainComponentExtension::setWorkspaceMode(WorkspaceMode mode)
{
    if (currentWorkspaceMode == mode)
        return;

    currentWorkspaceMode = mode;

    switch (mode)
    {
        case WorkspaceMode::Full:
            setSynthWorkspaceVisible(true);
            setMixingPanelsVisible(true);
            break;

        case WorkspaceMode::SynthOnly:
            setSynthWorkspaceVisible(true);
            setMixingPanelsVisible(false);
            break;

        case WorkspaceMode::MixingOnly:
            setSynthWorkspaceVisible(false);
            setMixingPanelsVisible(true);
            break;

        case WorkspaceMode::Minimal:
            setSynthWorkspaceVisible(false);
            setMixingPanelsVisible(false);
            break;
    }

    if (onLayoutChanged)
        onLayoutChanged();
}

//==============================================================================
// State Persistence
//==============================================================================

juce::ValueTree MainComponentExtension::saveState() const
{
    juce::ValueTree state("MainComponentExtension");

    // Workspace-Mode speichern
    state.setProperty("workspaceMode", static_cast<int>(currentWorkspaceMode), nullptr);
    state.setProperty("synthWorkspaceVisible", synthWorkspaceVisible, nullptr);
    state.setProperty("mixingPanelsVisible", mixingPanelsVisible, nullptr);

    // Theme speichern
    if (themeSystem)
        state.setProperty("themeType", static_cast<int>(themeSystem->getThemeType()), nullptr);

    // Panel-States speichern
    if (synthWorkspacePanel)
        state.addChild(synthWorkspacePanel->saveState(), -1, nullptr);

    if (routingMatrixPanel)
    {
        juce::ValueTree routingState("RoutingMatrix");
        for (const auto& conn : routingMatrixPanel->getAllConnections())
            routingState.addChild(conn.saveState(), -1, nullptr);
        state.addChild(routingState, -1, nullptr);
    }

    return state;
}

void MainComponentExtension::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    // Workspace-Mode wiederherstellen
    int modeValue = state.getProperty("workspaceMode", static_cast<int>(WorkspaceMode::Full));
    currentWorkspaceMode = static_cast<WorkspaceMode>(modeValue);
    synthWorkspaceVisible = state.getProperty("synthWorkspaceVisible", true);
    mixingPanelsVisible = state.getProperty("mixingPanelsVisible", true);

    // Theme wiederherstellen
    int themeType = state.getProperty("themeType", 0);
    setTheme(themeType);

    // Panel-States wiederherstellen
    for (auto child : state)
    {
        if (child.hasType("DockablePanelState") && synthWorkspacePanel)
            synthWorkspacePanel->restoreState(child);

        if (child.hasType("RoutingMatrix") && routingMatrixPanel)
        {
            for (auto connChild : child)
            {
                Connection conn;
                conn.restoreState(connChild);
                routingMatrixPanel->addConnection(conn);
            }
        }
    }

    // Workspace-Mode anwenden
    setWorkspaceMode(currentWorkspaceMode);
}

//==============================================================================
// JUCE Component Overrides
//==============================================================================

void MainComponentExtension::paint(juce::Graphics& g)
{
    // Wir zeichnen keinen Hintergrund, damit die Panels darunter sichtbar bleiben
    // Die Extension verwaltet nur die Layout-Logik, nicht die Darstellung
    g.fillAll(juce::Colours::transparentBlack);
}

void MainComponentExtension::resized()
{
    // Guard: Verhindere rekursive Aufrufe von resized()
    if (isUpdatingLayout)
        return;

    juce::ScopedValueSetter<bool> guard(isUpdatingLayout, true);

    // Layout wird vom DockingManager verwaltet
    // Diese Methode kann für zusätzliche Layout-Logik verwendet werden
}

void MainComponentExtension::timerCallback()
{
    // Periodische Updates
    // Level-Meter werden in den Panels selbst über Timer aktualisiert
}
