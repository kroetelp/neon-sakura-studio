// ============================================================================
// DockingManager.cpp - Implementierung
// ============================================================================

#include "DockingManager.h"
#include "MainComponent.h"  // Für triggerLayoutUpdate
#include "Theme/ThemeManager.h"
#include "WavetableUI/WavetablePanel.h"
#include "WavetableUI/WavetableSynthEditor.h"
#include "WavetableSynth/WavetableParams.h"
#include "WavetableSynth/WavetableData.h"
#include "RhythmExplorer/RhythmExplorerPanel.h"
#include "MelodyPanel/MelodyPanelPanel.h"
#include "UI/SynthWorkspacePanel.h"
#include "UI/ZoneSnapManager.h"

// ============================================================================
// FloatingWindowContainer Implementation
// ============================================================================

FloatingWindowContainer::FloatingWindowContainer(const juce::String& windowTitle,
                                                 DockablePanel* panelToContain,
                                                 bool shouldReDockOnClose)
    : juce::DocumentWindow(windowTitle,
                            ThemeManager::getInstance().getBackgroundColor(),
                            juce::DocumentWindow::allButtons,
                            true)
    , panel(panelToContain)
    , reDockOnClose(shouldReDockOnClose)
{
    auto& theme = ThemeManager::getInstance();

    jassert(panel != nullptr);  // Panel darf nicht null sein!

    // Window-Eigenschaften
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(300, 200, 4000, 3000);

    // Panel in das Fenster einfügen
    if (panel)
    {
        setContentNonOwned(panel, true);
        setContentComponentSize(panel->getPreferredFloatingBounds().getWidth(),
                                panel->getPreferredFloatingBounds().getHeight());
    }

    // Theme Styling
    setColour(juce::DocumentWindow::textColourId, theme.getTextSecondaryColor());
}

FloatingWindowContainer::~FloatingWindowContainer()
{
    // Panel NICHT löschen - es gehört dem DockingManager!
    setContentNonOwned(nullptr, false);
}

void FloatingWindowContainer::closeButtonPressed()
{
    if (onWindowCloseRequested)
    {
        onWindowCloseRequested(this);
    }
    else if (reDockOnClose && panel)
    {
        // Default-Verhalten: Re-Dock durch Ausblenden
        // Der DockingManager kümmert sich um das eigentliche Re-Docking
        setVisible(false);
    }
    else
    {
        setVisible(false);
    }
}

void FloatingWindowContainer::moved()
{
    DocumentWindow::moved();

    // NEU: Snapping Logic für Floating Panels
    if (panel && dynamic_cast<FloatingPanelBase*>(panel))
    {
        // TODO: Implementiere Zone-Snapping
        // Dies erfordert Zugriff auf ZoneSnapManager
        // Die Position kann automatisch zur nächsten Zone gesnappt werden
    }

    if (onWindowMoved)
        onWindowMoved(this);
}

void FloatingWindowContainer::resized()
{
    DocumentWindow::resized();

    if (onWindowResized)
        onWindowResized(this);
}

juce::ValueTree FloatingWindowContainer::saveWindowState() const
{
    juce::ValueTree state("FloatingWindow");
    state.setProperty("x", getX(), nullptr);
    state.setProperty("y", getY(), nullptr);
    state.setProperty("width", getWidth(), nullptr);
    state.setProperty("height", getHeight(), nullptr);
    state.setProperty("reDockOnClose", reDockOnClose, nullptr);

    if (panel)
    {
        state.setProperty("panelType", static_cast<int>(panel->getPanelType()), nullptr);
    }

    return state;
}

void FloatingWindowContainer::restoreWindowState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    if (state.hasProperty("x") && state.hasProperty("y"))
    {
        setTopLeftPosition(static_cast<int>(state.getProperty("x")),
                           static_cast<int>(state.getProperty("y")));
    }

    if (state.hasProperty("width") && state.hasProperty("height"))
    {
        setSize(static_cast<int>(state.getProperty("width")),
                static_cast<int>(state.getProperty("height")));
    }

    if (state.hasProperty("reDockOnClose"))
    {
        reDockOnClose = state.getProperty("reDockOnClose");
    }
}

// ============================================================================
// DockLayout Implementation
// ============================================================================

juce::ValueTree DockLayout::saveState() const
{
    juce::ValueTree state("DockLayout");

    // Panels in Listen speichern
    auto savePanelList = [&state](const juce::String& name, const std::vector<PanelType>& panels)
    {
        juce::ValueTree list(name);
        for (auto type : panels)
        {
            juce::ValueTree panel("Panel");
            panel.setProperty("type", static_cast<int>(type), nullptr);
            list.addChild(panel, -1, nullptr);
        }
        state.addChild(list, -1, nullptr);
    };

    savePanelList("LeftPanels", leftPanels);
    savePanelList("RightPanels", rightPanels);
    savePanelList("BottomPanels", bottomPanels);
    savePanelList("CenterTabs", centerTabs);

    state.setProperty("activeCenterTab", static_cast<int>(activeCenterTab), nullptr);
    state.setProperty("leftWidth", leftWidth, nullptr);
    state.setProperty("rightWidth", rightWidth, nullptr);
    state.setProperty("bottomHeight", bottomHeight, nullptr);

    return state;
}

void DockLayout::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    auto restorePanelList = [&state](const juce::String& name, std::vector<PanelType>& panels)
    {
        panels.clear();
        auto list = state.getChildWithName(name);
        if (list.isValid())
        {
            for (int i = 0; i < list.getNumChildren(); ++i)
            {
                auto panel = list.getChild(i);
                if (panel.hasProperty("type"))
                {
                    panels.push_back(static_cast<PanelType>(static_cast<int>(panel.getProperty("type"))));
                }
            }
        }
    };

    restorePanelList("LeftPanels", leftPanels);
    restorePanelList("RightPanels", rightPanels);
    restorePanelList("BottomPanels", bottomPanels);
    restorePanelList("CenterTabs", centerTabs);

    if (state.hasProperty("activeCenterTab"))
        activeCenterTab = static_cast<PanelType>(static_cast<int>(state.getProperty("activeCenterTab")));

    if (state.hasProperty("leftWidth"))
        leftWidth = static_cast<int>(state.getProperty("leftWidth"));

    if (state.hasProperty("rightWidth"))
        rightWidth = static_cast<int>(state.getProperty("rightWidth"));

    if (state.hasProperty("bottomHeight"))
        bottomHeight = static_cast<int>(state.getProperty("bottomHeight"));
}

void DockLayout::clear()
{
    leftPanels.clear();
    rightPanels.clear();
    bottomPanels.clear();
    centerTabs.clear();
    activeCenterTab = PanelType::Timeline;
}

// ============================================================================
// DockingManager Implementation
// ============================================================================

DockingManager::DockingManager()
{
    // Default-Layout initialisieren
    currentLayout.rightWidth = 320;
}

DockingManager::~DockingManager()
{
    // Track-Wavetable Editor und Window schließen
    trackWavetableEditor.reset();
    trackWavetableWindow.reset();

    // Floating Windows schließen
    closeAllFloatingWindows();

    // Panels werden automatisch durch unique_ptr gelöscht
}

// === Panel Registration ===

void DockingManager::registerPanel(std::unique_ptr<DockablePanel> panel)
{
    if (!panel)
        return;

    PanelType type = panel->getPanelType();

    // Callbacks einrichten
    panel->onRequestUndock = [this, type]()
    {
        undockPanel(type);
    };

    panel->onRequestClose = [this, type]()
    {
        closePanel(type);
    };

    panels[type] = std::move(panel);
}

DockablePanel* DockingManager::getPanel(PanelType type) const
{
    auto it = panels.find(type);
    if (it != panels.end())
        return it->second.get();

    return nullptr;
}

bool DockingManager::hasPanel(PanelType type) const
{
    return panels.find(type) != panels.end();
}

std::vector<PanelType> DockingManager::getRegisteredPanelTypes() const
{
    std::vector<PanelType> types;
    types.reserve(panels.size());

    for (const auto& pair : panels)
    {
        types.push_back(pair.first);
    }

    return types;
}

// === Visibility & Dock State ===

void DockingManager::setPanelVisible(PanelType type, bool visible)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    if (visible)
    {
        // Panel anzeigen
        if (panel->isHidden())
        {
            // === Single-Window Workspace: Alle Panels standardmäßig DOCKED ===
            dockPanel(type, DockPosition::Center);
        }
        else if (panel->isFloating())
        {
            // Floating window anzeigen
            auto* window = getFloatingWindow(type);
            if (window)
            {
                window->setVisible(true);
                window->toFront(true);
            }
        }
        else if (panel->isDocked())
        {
            panel->setVisible(true);
            triggerLayoutUpdate();
        }
    }
    else
    {
        // Panel verstecken
        closePanel(type);
    }
}

bool DockingManager::isPanelVisible(PanelType type) const
{
    auto* panel = getPanel(type);
    if (!panel)
        return false;

    if (panel->isFloating())
    {
        auto* window = getFloatingWindow(type);
        return window && window->isVisible();
    }

    return panel->isVisibleInLayout();
}

void DockingManager::dockPanel(PanelType type, DockPosition position)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    // Falls floating, erst das Window schließen
    if (panel->isFloating())
    {
        destroyFloatingWindow(type);
    }

    // Panel zum MainComponent hinzufügen
    addPanelToParent(type);

    // State updaten
    panel->setDockPositionInternal(position);
    panel->setDockStateInternal(DockState::Docked);
    panel->prepareForDock();

    // Zum Layout hinzufügen
    addPanelToLayout(type, position);

    triggerLayoutUpdate();
}

void DockingManager::undockPanel(PanelType type)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    // Bereits floating - nichts zu tun
    if (panel->isFloating())
        return;

    // Panel vorbereiten
    panel->prepareForUndock();

    // Aus MainComponent entfernen (falls angedockt)
    if (panel->isDocked())
    {
        removePanelFromParent(type);
        removePanelFromLayout(type);
    }

    // Floating Window erstellen
    createFloatingWindow(type);

    // State updaten
    panel->setDockStateInternal(DockState::Floating);

    triggerLayoutUpdate();
}

void DockingManager::togglePanelDockState(PanelType type)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    if (panel->isFloating())
    {
        dockPanel(type, panel->getDockPosition());
    }
    else if (panel->isDocked())
    {
        undockPanel(type);
    }
    else if (panel->isHidden())
    {
        dockPanel(type, DockPosition::Right);
    }
}

void DockingManager::closePanel(PanelType type)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    if (panel->isFloating())
    {
        destroyFloatingWindow(type);
    }
    else if (panel->isDocked())
    {
        removePanelFromParent(type);
        removePanelFromLayout(type);
    }

    panel->setDockStateInternal(DockState::Hidden);
    triggerLayoutUpdate();
}

// === Layout Management ===

void DockingManager::setLeftWidth(int width)
{
    currentLayout.leftWidth = juce::jlimit(DockLayout::minSidebarWidth,
                                            DockLayout::maxSidebarWidth,
                                            width);
    triggerLayoutUpdate();
}

void DockingManager::setRightWidth(int width)
{
    currentLayout.rightWidth = juce::jlimit(DockLayout::minSidebarWidth,
                                             DockLayout::maxSidebarWidth,
                                             width);
    triggerLayoutUpdate();
}

void DockingManager::setBottomHeight(int height)
{
    currentLayout.bottomHeight = juce::jlimit(DockLayout::minBottomHeight,
                                               DockLayout::maxBottomHeight,
                                               height);
    triggerLayoutUpdate();
}

void DockingManager::addPanelToLayout(PanelType type, DockPosition position)
{
    // Helper um Panel zu Liste hinzuzufügen (falls nicht vorhanden)
    auto addToList = [](std::vector<PanelType>& list, PanelType t)
    {
        if (std::find(list.begin(), list.end(), t) == list.end())
        {
            list.push_back(t);
        }
    };

    switch (position)
    {
        case DockPosition::Left:
            addToList(currentLayout.leftPanels, type);
            break;
        case DockPosition::Right:
            addToList(currentLayout.rightPanels, type);
            break;
        case DockPosition::Bottom:
            addToList(currentLayout.bottomPanels, type);
            break;
        case DockPosition::Center:
            addToList(currentLayout.centerTabs, type);
            break;
        case DockPosition::Top:
            // Wird aktuell nicht unterstützt
            break;
    }
}

void DockingManager::removePanelFromLayout(PanelType type)
{
    // Helper um Panel aus Liste zu entfernen
    auto removeFromList = [](std::vector<PanelType>& list, PanelType t)
    {
        list.erase(std::remove(list.begin(), list.end(), t), list.end());
    };

    removeFromList(currentLayout.leftPanels, type);
    removeFromList(currentLayout.rightPanels, type);
    removeFromList(currentLayout.bottomPanels, type);
    removeFromList(currentLayout.centerTabs, type);
}

// === Center Tab Management ===

void DockingManager::setActiveCenterTab(PanelType type)
{
    // Prüfen ob das Panel in den Center-Tabs ist
    auto& tabs = currentLayout.centerTabs;
    if (std::find(tabs.begin(), tabs.end(), type) == tabs.end())
    {
        // Panel zu Center-Tabs hinzufügen
        addPanelToLayout(type, DockPosition::Center);
    }

    currentLayout.activeCenterTab = type;
    triggerLayoutUpdate();
}

bool DockingManager::isCenterTabActive(PanelType type) const
{
    return currentLayout.activeCenterTab == type;
}

// === MainComponent Integration ===

void DockingManager::setMainComponent(MainComponent* mainComp)
{
    mainComponent = mainComp;
}

void DockingManager::triggerLayoutUpdate()
{
    if (isUpdatingLayout)
        return;  // Rekursion verhindern

    isUpdatingLayout = true;

    if (mainComponent)
    {
        mainComponent->triggerLayoutUpdate();
    }

    isUpdatingLayout = false;
}

// === Layout Update für MainComponent ===

void DockingManager::updateDockedLayout(const juce::Rectangle<int>& availableArea)
{
    if (availableArea.isEmpty())
        return;

    auto area = availableArea;

    // === LEFT SIDEBAR ===
    if (currentLayout.hasLeftPanels())
    {
        auto leftWidth = juce::jmin(currentLayout.leftWidth, area.getWidth() / 3);
        auto leftArea = area.removeFromLeft(leftWidth);

        int y = leftArea.getY();
        int panelHeight = leftArea.getHeight() / static_cast<int>(currentLayout.leftPanels.size());

        for (auto type : currentLayout.leftPanels)
        {
            auto* panel = getPanel(type);
            if (panel && panel->isDocked())
            {
                panel->setBounds(leftArea.getX(), y, leftArea.getWidth(), panelHeight);
                y += panelHeight;
            }
        }
    }

    // === RIGHT SIDEBAR ===
    if (currentLayout.hasRightPanels())
    {
        auto rightWidth = juce::jmin(currentLayout.rightWidth, area.getWidth() / 3);
        auto rightArea = area.removeFromRight(rightWidth);

        int y = rightArea.getY();
        int panelHeight = rightArea.getHeight() / static_cast<int>(currentLayout.rightPanels.size());

        for (auto type : currentLayout.rightPanels)
        {
            auto* panel = getPanel(type);
            if (panel && panel->isDocked())
            {
                panel->setBounds(rightArea.getX(), y, rightArea.getWidth(), panelHeight);
                y += panelHeight;
            }
        }
    }

    // === BOTTOM AREA ===
    if (currentLayout.hasBottomPanels())
    {
        auto bottomHeight = juce::jmin(currentLayout.bottomHeight, area.getHeight() / 3);
        auto bottomArea = area.removeFromBottom(bottomHeight);

        int x = bottomArea.getX();
        int panelWidth = bottomArea.getWidth() / static_cast<int>(currentLayout.bottomPanels.size());

        for (auto type : currentLayout.bottomPanels)
        {
            auto* panel = getPanel(type);
            if (panel && panel->isDocked())
            {
                panel->setBounds(x, bottomArea.getY(), panelWidth, bottomArea.getHeight());
                x += panelWidth;
            }
        }
    }

    // === CENTER AREA (Tabs oder einzelnes Panel) ===
    if (currentLayout.hasCenterPanels())
    {
        // Das aktive Center-Panel erhält den gesamten restlichen Bereich
        auto* activePanel = getPanel(currentLayout.activeCenterTab);
        if (activePanel && activePanel->isDocked())
        {
            activePanel->setBounds(area);
        }
    }
}

juce::Rectangle<int> DockingManager::calculatePanelBounds(PanelType type, const juce::Rectangle<int>& totalArea) const
{
    auto* panel = getPanel(type);
    if (!panel)
        return {};

    // Prüfen in welchem Bereich das Panel ist
    auto isInList = [](const std::vector<PanelType>& list, PanelType t) {
        return std::find(list.begin(), list.end(), t) != list.end();
    };

    if (isInList(currentLayout.leftPanels, type))
    {
        int y = 0;
        for (auto t : currentLayout.leftPanels)
        {
            if (t == type) break;
            y++;
        }
        int panelHeight = totalArea.getHeight() / static_cast<int>(currentLayout.leftPanels.size());
        return { 0, y * panelHeight, currentLayout.leftWidth, panelHeight };
    }

    if (isInList(currentLayout.rightPanels, type))
    {
        int y = 0;
        for (auto t : currentLayout.rightPanels)
        {
            if (t == type) break;
            y++;
        }
        int panelHeight = totalArea.getHeight() / static_cast<int>(currentLayout.rightPanels.size());
        return { totalArea.getRight() - currentLayout.rightWidth, y * panelHeight,
                 currentLayout.rightWidth, panelHeight };
    }

    if (isInList(currentLayout.centerTabs, type))
    {
        return totalArea;
    }

    return {};
}

// === State Persistence ===

juce::ValueTree DockingManager::saveLayoutState() const
{
    juce::ValueTree state("DockingManagerState");

    // Layout speichern
    state.addChild(currentLayout.saveState(), 0, nullptr);

    // Panel-States speichern
    for (const auto& pair : panels)
    {
        auto* panel = pair.second.get();
        if (panel)
        {
            state.addChild(panel->saveState(), -1, nullptr);
        }
    }

    // Floating Window States speichern
    for (const auto& pair : floatingWindows)
    {
        auto* window = pair.second.get();
        if (window)
        {
            state.addChild(window->saveWindowState(), -1, nullptr);
        }
    }

    return state;
}

void DockingManager::restoreLayoutState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    // Layout restoren
    auto layoutState = state.getChildWithName("DockLayout");
    if (layoutState.isValid())
    {
        currentLayout.restoreState(layoutState);
    }

    // Panel States restoren
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.isValid() && child.hasProperty("panelType"))
        {
            auto type = static_cast<PanelType>(static_cast<int>(child.getProperty("panelType")));
            auto* panel = getPanel(type);
            if (panel)
            {
                panel->restoreState(child);
            }
        }
    }

    // Floating Windows restoren
    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        auto child = state.getChild(i);
        if (child.isValid() && child.getType() == juce::Identifier("FloatingWindow"))
        {
            if (child.hasProperty("panelType"))
            {
                auto type = static_cast<PanelType>(static_cast<int>(child.getProperty("panelType")));
                auto* window = getFloatingWindow(type);
                if (window)
                {
                    window->restoreWindowState(child);
                }
            }
        }
    }

    triggerLayoutUpdate();
}

// === Bulk Operations ===

void DockingManager::closeAllFloatingWindows()
{
    // Kopie der Keys erstellen, da wir während der Iteration löschen
    std::vector<PanelType> types;
    for (const auto& pair : floatingWindows)
    {
        types.push_back(pair.first);
    }

    for (auto type : types)
    {
        destroyFloatingWindow(type);
    }
}

void DockingManager::hideAllPanels()
{
    for (const auto& pair : panels)
    {
        closePanel(pair.first);
    }
}

void DockingManager::resetToDefaultLayout()
{
    closeAllFloatingWindows();
    currentLayout.clear();
    currentLayout.rightWidth = 320;
    triggerLayoutUpdate();
}

// === Dependencies ===

void DockingManager::setTimelineDependencies(TimelineData* data, RecordingManager* recorder)
{
    // Wird implementiert wenn TimelinePanel existiert
    juce::ignoreUnused(data, recorder);
}

void DockingManager::setWavetableEngine(WavetableEngine* engine)
{
    auto* wavetablePanel = getPanelAs<WavetablePanel>(PanelType::WavetableSynth);
    if (wavetablePanel)
    {
        wavetablePanel->setEngine(engine);
    }
}

void DockingManager::openTrackWavetableEditor(int trackIndex,
                                               std::shared_ptr<WavetableParams> params,
                                               std::shared_ptr<WavetableData> wavetableData)
{
    if (!params)
        return;

    currentEditingTrack = trackIndex;
    currentTrackParams = params;
    currentTrackWavetableData = wavetableData;

    // Create or update the editor
    if (!trackWavetableEditor)
    {
        trackWavetableEditor = std::make_unique<WavetableSynthEditor>(params);
    }
    else
    {
        trackWavetableEditor->setSharedParams(params);
    }

    // Set wavetable data if available
    if (wavetableData)
    {
        trackWavetableEditor->setWavetableData(wavetableData);
    }

    // Create window if needed
    if (!trackWavetableWindow)
    {
        auto& theme = ThemeManager::getInstance();

        // Custom DocumentWindow that just hides on close
        class TrackWavetableWindow : public juce::DocumentWindow
        {
        public:
            TrackWavetableWindow(const juce::String& name)
                : juce::DocumentWindow(name, ThemeManager::getInstance().getBackgroundColor(),
                                      juce::DocumentWindow::allButtons, true)
            {
                setUsingNativeTitleBar(true);
                setResizable(true, true);
                setResizeLimits(600, 400, 2000, 1500);
            }

            void closeButtonPressed() override
            {
                setVisible(false);
            }
        };

        trackWavetableWindow = std::make_unique<TrackWavetableWindow>(
            "Track " + juce::String(trackIndex + 1) + " Wavetable");
        trackWavetableWindow->setContentOwned(trackWavetableEditor.get(), false);
    }
    else
    {
        // Update window title for new track
        trackWavetableWindow->setName("Track " + juce::String(trackIndex + 1) + " Wavetable");
    }

    trackWavetableWindow->setSize(1050, 750);
    trackWavetableWindow->setVisible(true);
    trackWavetableWindow->toFront(true);
}

void DockingManager::closeTrackWavetableEditor()
{
    if (trackWavetableWindow)
    {
        trackWavetableWindow->setVisible(false);
    }
    currentEditingTrack = -1;
    currentTrackParams.reset();
    currentTrackWavetableData.reset();
}

bool DockingManager::isTrackWavetableEditorOpen() const
{
    return trackWavetableWindow != nullptr && trackWavetableWindow->isVisible();
}

int DockingManager::getCurrentEditingTrack() const
{
    return currentEditingTrack;
}

void DockingManager::updatePanelsForTrack(int trackIndex)
{
    // Update Wavetable Synth Panel with track params
    // (For now, this doesn't automatically switch WavetablePanel to track mode
    // - the user must explicitly open track editor)

    // Update RhythmExplorer Panel
    auto* rhythmPanel = getPanelAs<RhythmExplorerPanel>(PanelType::RhythmExplorer);
    if (rhythmPanel)
    {
        rhythmPanel->setTargetTrack(trackIndex);
    }

    // Update MelodyPanel
    auto* melodyPanel = getPanelAs<MelodyPanelPanel>(PanelType::MelodyPanel);
    if (melodyPanel)
    {
        melodyPanel->setTargetTrack(trackIndex);
    }

    // Update SynthWorkspacePanel
    auto* synthWorkspacePanel = getPanelAs<SynthWorkspacePanel>(PanelType::SynthWorkspace);
    if (synthWorkspacePanel)
    {
        synthWorkspacePanel->setTargetTrack(trackIndex);
    }

    // Update Track-Wavetable Editor if open
    if (isTrackWavetableEditorOpen() && currentTrackParams)
    {
        // Note: trackWavetableEditor already has shared params,
        // so changes are reflected automatically
    }
}

// === Floating Window Management ===

FloatingWindowContainer* DockingManager::getFloatingWindow(PanelType type) const
{
    auto it = floatingWindows.find(type);
    if (it != floatingWindows.end())
        return it->second.get();

    return nullptr;
}

bool DockingManager::hasFloatingWindow(PanelType type) const
{
    return floatingWindows.find(type) != floatingWindows.end();
}

int DockingManager::getNumFloatingWindows() const
{
    return static_cast<int>(floatingWindows.size());
}

// === Internal Helper ===

void DockingManager::createFloatingWindow(PanelType type)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    // Bestehendes Window entfernen falls vorhanden
    destroyFloatingWindow(type);

    // Neues Window erstellen
    auto window = std::make_unique<FloatingWindowContainer>(panel->getPanelName(), panel);

    // Window Callbacks einrichten
    window->onWindowCloseRequested = [this, type](FloatingWindowContainer* w)
    {
        // Panel re-docken statt nur zu schließen
        dockPanel(type, getPanel(type)->getDockPosition());
    };

    // Window positionieren (zentriert oder an gespeicherter Position)
    auto& desktop = juce::Desktop::getInstance();
    auto displayArea = desktop.getDisplays().getPrimaryDisplay()->userArea;

    auto preferredBounds = panel->getPreferredFloatingBounds();
    window->setTopLeftPosition(
        displayArea.getCentre().translated(-preferredBounds.getWidth() / 2,
                                            -preferredBounds.getHeight() / 2)
    );

    window->setVisible(true);
    window->toFront(true);

    floatingWindows[type] = std::move(window);
}

void DockingManager::destroyFloatingWindow(PanelType type)
{
    auto it = floatingWindows.find(type);
    if (it != floatingWindows.end())
    {
        // Panel aus dem Window entfernen (bevor Window zerstört wird)
        auto* window = it->second.get();
        if (window)
        {
            window->setContentNonOwned(nullptr, false);
            window->setVisible(false);
        }

        floatingWindows.erase(it);
    }
}

void DockingManager::addPanelToParent(PanelType type)
{
    auto* panel = getPanel(type);
    if (!panel || !mainComponent)
        return;

    // Panel zur MainComponent hinzufügen
    // Die MainComponent kümmert sich um das Layout in resized()
    mainComponent->addAndMakeVisible(panel);
}

void DockingManager::removePanelFromParent(PanelType type)
{
    auto* panel = getPanel(type);
    if (!panel)
        return;

    // Panel aus Parent entfernen
    if (panel->getParentComponent())
    {
        panel->getParentComponent()->removeChildComponent(panel);
    }
}

// === Utility ===

juce::String DockingManager::panelTypeToString(PanelType type)
{
    switch (type)
    {
        case PanelType::Timeline:       return "Timeline";
        case PanelType::WavetableSynth: return "WavetableSynth";
        case PanelType::RhythmExplorer: return "RhythmExplorer";
        case PanelType::MelodyPanel:    return "MelodyPanel";
        case PanelType::Mixer:          return "Mixer";
        case PanelType::StepSequencer:  return "StepSequencer";
        case PanelType::TrackEditor:    return "TrackEditor";
        default:                        return "Unknown";
    }
}

PanelType DockingManager::stringToPanelType(const juce::String& str)
{
    if (str == "Timeline")        return PanelType::Timeline;
    if (str == "WavetableSynth")  return PanelType::WavetableSynth;
    if (str == "RhythmExplorer")  return PanelType::RhythmExplorer;
    if (str == "MelodyPanel")     return PanelType::MelodyPanel;
    if (str == "Mixer")           return PanelType::Mixer;
    if (str == "StepSequencer")   return PanelType::StepSequencer;
    if (str == "TrackEditor")     return PanelType::TrackEditor;

    return PanelType::Unknown;
}

// ============================================================================
// NEU: Floating Panel Features Implementation
// ============================================================================

void DockingManager::setZoneSnapManager(ZoneSnapManager* manager)
{
    zoneSnapManager = manager;

    // Initialisiere Zone-Bounds wenn verfügbar
    if (zoneSnapManager && mainComponent)
    {
        auto bounds = mainComponent->getBounds();
        zoneSnapManager->updateZoneBounds(bounds);
    }
}

void DockingManager::setPanelState(PanelType type, PanelState state)
{
    panelStates[type] = state;

    // Wenn das Panel ein FloatingPanelBase ist, aktualisiere es
    if (auto* panel = getPanel(type))
    {
        if (auto* floatingPanel = dynamic_cast<FloatingPanelBase*>(panel))
        {
            floatingPanel->setPanelState(state);
        }
    }
}

PanelState DockingManager::getPanelState(PanelType type) const
{
    auto it = panelStates.find(type);
    if (it != panelStates.end())
    {
        return it->second;
    }
    return PanelState::Hidden;
}

void DockingManager::setPanelSizeMode(PanelType type, PanelSizeMode mode)
{
    panelSizeModes[type] = mode;

    // Wenn das Panel ein FloatingPanelBase ist, aktualisiere es
    DockablePanel* panel = getPanel(type);

    if (auto* floatingPanel = dynamic_cast<FloatingPanelBase*>(panel))
    {
        floatingPanel->setSizeMode(mode);
    }

    // Wenn das Panel angedockt ist, aktualisiere das Layout
    if (panel && panel->isDocked())
    {
        triggerLayoutUpdate();
    }
}

PanelSizeMode DockingManager::getPanelSizeMode(PanelType type) const
{
    auto it = panelSizeModes.find(type);
    if (it != panelSizeModes.end())
    {
        return it->second;
    }
    return PanelSizeMode::Standard;
}
