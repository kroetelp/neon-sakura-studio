// ============================================================================
// SynthWorkspacePanel.cpp - Synthesizer-Workspace Implementation
// ============================================================================

#include "SynthWorkspacePanel.h"
#include "RoutingMatrixPanel.h"
#include "MasterBusPanel.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../WavetableSynth/WavetableEngine.h"
#include "../Theme/ThemeManager.h"
#include "../Theme/ExtendedThemeSystem.h"
#include <algorithm>

//==============================================================================
SynthWorkspacePanel::SynthWorkspacePanel()
    : DockablePanel(PanelType::SynthWorkspace, "Synth Workspace")
{
    // Tab-Visibility initialisieren
    tabVisibility[WorkspaceTab::Synth] = true;
    tabVisibility[WorkspaceTab::Routing] = true;
    tabVisibility[WorkspaceTab::Master] = true;
}

SynthWorkspacePanel::~SynthWorkspacePanel()
{
    stopTimer();
}

//==============================================================================
void SynthWorkspacePanel::initializeContent()
{
    // Tab Component erstellen
    createTabComponent();

    // Timer für Animationen und Updates starten (30fps)
    startTimerHz(30);
}

//==============================================================================
void SynthWorkspacePanel::setEngine(WavetableEngine* engine)
{
    wavetableEngine = engine;

    // Engine-Reference an RoutingPanel übergeben, falls benötigt
    if (routingMatrixPanel)
    {
        // RoutingMatrixPanel könnte die Engine für Signal-Updates nutzen
    }
}

//==============================================================================
void SynthWorkspacePanel::setTargetTrack(int trackIndex)
{
    if (trackIndex < 0)
        trackIndex = 0;

    currentTrack = trackIndex;

    // Track-Index an sub-Panels weitergeben
    // RoutingMatrixPanel und MasterBusPanel können diese Info nutzen
    // um track-spezifische Einstellungen anzuzeigen
}

//==============================================================================
void SynthWorkspacePanel::createTabComponent()
{
    auto& theme = ThemeManager::getInstance();

    // Tab-Component erstellen
    tabbedComponent = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    tabbedComponent->setTabBarDepth(tabBarHeight);

    // Theme-Colors
    tabbedComponent->setColour(juce::TabbedComponent::backgroundColourId, theme.getBackgroundColor());
    tabbedComponent->setColour(juce::TabbedComponent::outlineColourId, theme.getPanelBorderColor());

    auto& tabBar = tabbedComponent->getTabbedButtonBar();
    tabBar.setColour(juce::TabbedButtonBar::tabOutlineColourId, theme.getPanelBorderColor());
    tabBar.setColour(juce::TabbedButtonBar::frontOutlineColourId, theme.getAccentColor());

    // Tabs NICHT hier erstellen - werden später nach Panel-Registrierung aufgerufen
    // Setup wird erst aufgerufen, wenn alle Panels verfügbar sind

    addAndMakeVisible(tabbedComponent.get());
}

//==============================================================================
void SynthWorkspacePanel::setupTabs()
{
    // Guard: Prüfe ob tabbedComponent existiert
    if (!tabbedComponent)
        return;

    // Guard: Verhindere rekursive Aufrufe von setupTabs()
    if (isSettingUpTabs)
        return;

    // Guard: Wenn Tabs bereits initialisiert wurden, nicht erneut aufrufen
    // (außer es ist ein explizites Refresh)
    if (tabsInitialized)
        return;

    juce::ScopedValueSetter<bool> guard(isSettingUpTabs, true);

    // WICHTIG: Die Panels werden im MainComponentExtension erstellt
    // und hier nur als References verwaltet.
    // Wenn die Panels noch nicht gesetzt wurden, werden Placeholder-Components erstellt.

    // Wir erstellen hier keine temporären Panels mehr,
    // da die Setter-Methoden die korrekten Panels liefern.

    // Zuerst alle bestehenden Tabs entfernen, um Duplikate zu vermeiden
    tabbedComponent->clearTabs();

    // Synth Tab (Wavetable-Editor)
    // Hinweis: Das WavetablePanel wird separat im DockingManager verwaltet
    // und kann hier über eine Referenz oder als Wrapper angezeigt werden
    if (!synthPlaceholder)
        synthPlaceholder = std::make_unique<juce::Component>();
    tabbedComponent->addTab(getTabName(WorkspaceTab::Synth),
                           getTabColour(WorkspaceTab::Synth),
                           synthPlaceholder.get(),
                           false);

    // Routing Tab
    if (routingMatrixPanel)
    {
        // Entferne das Panel aus seinem aktuellen Parent falls nötig
        if (auto* currentParent = routingMatrixPanel->getParentComponent())
        {
            if (currentParent != tabbedComponent.get())
                currentParent->removeChildComponent(routingMatrixPanel);
        }
        tabbedComponent->addTab(getTabName(WorkspaceTab::Routing),
                               getTabColour(WorkspaceTab::Routing),
                               routingMatrixPanel,
                               false);
    }
    else
    {
        // Placeholder
        if (!routingPlaceholder)
            routingPlaceholder = std::make_unique<juce::Component>();
        tabbedComponent->addTab(getTabName(WorkspaceTab::Routing),
                               getTabColour(WorkspaceTab::Routing),
                               routingPlaceholder.get(),
                               false);
    }

    // Master Tab
    if (masterBusPanel)
    {
        // Entferne das Panel aus seinem aktuellen Parent falls nötig
        if (auto* currentParent = masterBusPanel->getParentComponent())
        {
            if (currentParent != tabbedComponent.get())
                currentParent->removeChildComponent(masterBusPanel);
        }
        tabbedComponent->addTab(getTabName(WorkspaceTab::Master),
                               getTabColour(WorkspaceTab::Master),
                               masterBusPanel,
                               false);
    }
    else
    {
        // Placeholder
        if (!masterPlaceholder)
            masterPlaceholder = std::make_unique<juce::Component>();
        tabbedComponent->addTab(getTabName(WorkspaceTab::Master),
                               getTabColour(WorkspaceTab::Master),
                               masterPlaceholder.get(),
                               false);
    }

    // Tabs als initialisiert markieren
    tabsInitialized = true;

    // Standardmäßig Synth Tab anzeigen
    tabbedComponent->setCurrentTabIndex(static_cast<int>(WorkspaceTab::Synth));

    // Tab-Change Callback - In JUCE 8 müssen wir einen Listener verwenden
    // Da TabbedComponent keinen einfachen onChange Callback hat, verwenden wir einen Timer
    // oder wir speichern den aktuellen Tab beim nächsten resized/paint Aufruf
}

//==============================================================================
void SynthWorkspacePanel::connectPanelCallbacks()
{
    if (routingMatrixPanel)
    {
        routingMatrixPanel->onConnectionAdded = [this](int id) {
            juce::ignoreUnused(id);
            // Routing-Änderung signalisieren
        };

        routingMatrixPanel->onConnectionRemoved = [this](int id) {
            juce::ignoreUnused(id);
            // Routing-Änderung signalisieren
        };
    }

    if (masterBusPanel)
    {
        masterBusPanel->onExportClicked = [this]() {
            // Export-Aktion
        };

        masterBusPanel->onRenderClicked = [this]() {
            // Render-Aktion
        };
    }
}

//==============================================================================
// Synth Mode Methods
//==============================================================================

void SynthWorkspacePanel::setSynthMode(SynthMode mode)
{
    if (currentMode == mode)
        return;

    currentMode = mode;

    // Je nach Mode das Layout anpassen
    switch (mode)
    {
        case SynthMode::Perform:
            // Perform Mode: Größere Controls, weniger Details
            break;

        case SynthMode::Design:
            // Design Mode: Vollständiger Editor
            break;

        case SynthMode::Patch:
            // Patch Mode: Preset-Browser hat Fokus
            break;
    }

    if (onModeChanged)
        onModeChanged();

    repaint();
}

//==============================================================================
// Tab Management
//==============================================================================

void SynthWorkspacePanel::setActiveTab(WorkspaceTab tab)
{
    if (!tabbedComponent)
        return;

    tabbedComponent->setCurrentTabIndex(static_cast<int>(tab));
}

void SynthWorkspacePanel::showSynthTab()
{
    setActiveTab(WorkspaceTab::Synth);
}

void SynthWorkspacePanel::showRoutingTab()
{
    setActiveTab(WorkspaceTab::Routing);
}

void SynthWorkspacePanel::showMasterTab()
{
    setActiveTab(WorkspaceTab::Master);
}

//==============================================================================
// Visibility Control
//==============================================================================

void SynthWorkspacePanel::setPanelVisible(WorkspaceTab tab, bool visible)
{
    tabVisibility[tab] = visible;

    if (onPanelVisibilityChanged)
        onPanelVisibilityChanged(getTabName(tab), visible);

    updatePanelVisibility();
    repaint();
}

bool SynthWorkspacePanel::isPanelVisible(WorkspaceTab tab) const
{
    auto it = tabVisibility.find(tab);
    return it != tabVisibility.end() && it->second;
}

void SynthWorkspacePanel::updatePanelVisibility()
{
    // Tab-Sichtbarkeit aktualisieren
    // In JUCE TabbedComponent können Tabs nicht einfach versteckt werden,
    // daher zeigen wir sie alle an und filtern die Content-Visibility
}

//==============================================================================
// Helper Methods
//==============================================================================

juce::String SynthWorkspacePanel::getTabName(WorkspaceTab tab) const
{
    switch (tab)
    {
        case WorkspaceTab::Synth: return "Synth";
        case WorkspaceTab::Routing: return "Routing";
        case WorkspaceTab::Master: return "Master";
        default: return "Unknown";
    }
}

juce::Colour SynthWorkspacePanel::getTabColour(WorkspaceTab tab) const
{
    auto& theme = ThemeManager::getInstance();

    switch (tab)
    {
        case WorkspaceTab::Synth:
            return theme.getAccentColor();
        case WorkspaceTab::Routing:
            return theme.getInfoColor();
        case WorkspaceTab::Master:
            return theme.getSuccessColor();
        default:
            return juce::Colours::white;
    }
}

//==============================================================================
// Panel Access - Setter Methods
//==============================================================================

void SynthWorkspacePanel::setRoutingMatrixPanel(RoutingMatrixPanel* panel)
{
    // Nur aktualisieren, wenn sich das Panel tatsächlich geändert hat
    if (routingMatrixPanel == panel)
        return;

    routingMatrixPanel = panel;
    // Tab-Setup veranlassen, wenn beide Panels gesetzt sind
    // Wir rufen setupTabs() nur auf, wenn noch nicht initialisiert
    // und beide Panels verfügbar sind
    if (tabbedComponent && routingMatrixPanel && masterBusPanel && !tabsInitialized)
    {
        setupTabs();
    }
}

void SynthWorkspacePanel::setMasterBusPanel(MasterBusPanel* panel)
{
    // Nur aktualisieren, wenn sich das Panel tatsächlich geändert hat
    if (masterBusPanel == panel)
        return;

    masterBusPanel = panel;
    // Tab-Setup veranlassen, wenn beide Panels gesetzt sind
    // Wir rufen setupTabs() nur auf, wenn noch nicht initialisiert
    // und beide Panels verfügbar sind
    if (tabbedComponent && routingMatrixPanel && masterBusPanel && !tabsInitialized)
    {
        setupTabs();
    }
}

void SynthWorkspacePanel::setPanelsDirect(RoutingMatrixPanel* routing, MasterBusPanel* master)
{
    // Panels direkt setzen, ohne setupTabs() aufzurufen
    // Dies wird während der Initialisierung verwendet, um rekursive Aufrufe zu vermeiden
    routingMatrixPanel = routing;
    masterBusPanel = master;
}

void SynthWorkspacePanel::refreshTabs()
{
    // Öffentliche Methode zum Neu-Erstellen der Tabs
    // Ruft setupTabs() auf, aber mit dem bereits implementierten Guard
    setupTabs();
}

//==============================================================================
// JUCE Component Overrides
//==============================================================================

void SynthWorkspacePanel::paint(juce::Graphics& g)
{
    // Guard: Verhindere rekursive Aufrufe von paint()
    if (isPainting)
        return;

    juce::ScopedValueSetter<bool> guard(isPainting, true);

    auto bounds = getLocalBounds();
    auto contentBounds = getContentBounds();

    // Hintergrund zeichnen
    g.fillAll(ThemeManager::getInstance().getPanelBackgroundColor());

    // Header zeichnen (falls sichtbar)
    if (shouldShowHeader())
    {
        auto headerBounds = bounds.removeFromTop(PanelHeader::headerHeight);
        g.setColour(ThemeManager::getInstance().getPanelHeaderColor());
        g.fillRect(headerBounds);

        // Titel zeichnen
        g.setColour(ThemeManager::getInstance().getTextPrimaryColor());
        g.setFont(16.0f);
        g.drawText(getPanelName(), headerBounds.toFloat(),
                  juce::Justification::centredLeft, true);

        // Mode-Indicator zeichnen
        drawModeIndicator(g, headerBounds);
    }

    // Content zeichnen
    if (tabbedComponent)
    {
        // Tab-Component wird von JUCE selbst gezeichnet
    }
}

void SynthWorkspacePanel::drawModeIndicator(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Mode-Text oben rechts im Header
    juce::String modeText;
    switch (currentMode)
    {
        case SynthMode::Perform: modeText = "PERFORM"; break;
        case SynthMode::Design:  modeText = "DESIGN"; break;
        case SynthMode::Patch:   modeText = "PATCH"; break;
    }

    auto modeBounds = bounds.withTrimmedLeft(bounds.getWidth() - 100).toFloat();
    g.setFont(10.0f);
    g.drawText(modeText, modeBounds, juce::Justification::centred, true);
}

void SynthWorkspacePanel::resized()
{
    // Guard: Verhindere rekursive Aufrufe von resized()
    if (isResizing)
        return;

    juce::ScopedValueSetter<bool> guard(isResizing, true);

    auto bounds = getLocalBounds();

    // Header platzieren
    if (shouldShowHeader())
    {
        bounds.removeFromTop(PanelHeader::headerHeight);
    }

    // Tab-Component platzieren
    if (tabbedComponent)
    {
        tabbedComponent->setBounds(bounds);
    }
}

void SynthWorkspacePanel::timerCallback()
{
    // Periodische Updates
    // Panels selbst haben ihre eigenen Timer für Meter-Updates
}
