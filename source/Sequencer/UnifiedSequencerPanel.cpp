// ============================================================================
// UnifiedSequencerPanel.cpp - Implementierung des Multi-Mode Sequencers
// ============================================================================

#include "UnifiedSequencerPanel.h"
#include "../Timeline/TimelineData.h"
#include "../TrackManager.h"

// Forward declarations (TODO: proper includes)
class RecordingManager;
class SampleManager;

// ============================================================================
// ModeTabComponent Implementierung
// ============================================================================

ModeTabComponent::ModeTabComponent()
{
    setupTabButton(trackTabButton, "Track");
    setupTabButton(patternTabButton, "Pattern");
    setupTabButton(timelineTabButton, "Timeline");

    addAndMakeVisible(trackTabButton);
    addAndMakeVisible(patternTabButton);
    addAndMakeVisible(timelineTabButton);

    updateTabStyles();

    // Button Callbacks
    trackTabButton.onClick = [this]()
    {
        setMode(Mode::Track);
    };

    patternTabButton.onClick = [this]()
    {
        setMode(Mode::Pattern);
    };

    timelineTabButton.onClick = [this]()
    {
        setMode(Mode::Timeline);
    };
}

void ModeTabComponent::setupTabButton(juce::TextButton& btn, const juce::String& text)
{
    btn.setButtonText(text);
    btn.setClickingTogglesState(false);
    btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    btn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.5f));
}

void ModeTabComponent::setMode(Mode newMode)
{
    if (currentMode == newMode)
        return;

    currentMode = newMode;
    updateTabStyles();

    if (onModeChanged)
        onModeChanged(newMode);
}

void ModeTabComponent::updateTabStyles()
{
    auto activeColor = getTabActiveColor();
    auto inactiveColor = getTabInactiveColor();

    trackTabButton.setColour(juce::TextButton::textColourOffId,
        currentMode == Mode::Track ? activeColor : inactiveColor);
    patternTabButton.setColour(juce::TextButton::textColourOffId,
        currentMode == Mode::Pattern ? activeColor : inactiveColor);
    timelineTabButton.setColour(juce::TextButton::textColourOffId,
        currentMode == Mode::Timeline ? activeColor : inactiveColor);

    // Active Tab gets underline
    trackTabButton.setColour(juce::TextButton::textColourOnId, activeColor);
    patternTabButton.setColour(juce::TextButton::textColourOnId, activeColor);
    timelineTabButton.setColour(juce::TextButton::textColourOnId, activeColor);
}

void ModeTabComponent::resized()
{
    auto bounds = getLocalBounds();
    int tabWidth = bounds.getWidth() / 3;

    trackTabButton.setBounds(bounds.removeFromLeft(tabWidth));
    patternTabButton.setBounds(bounds.removeFromLeft(tabWidth));
    timelineTabButton.setBounds(bounds);
}

void ModeTabComponent::paint(juce::Graphics& g)
{
    g.fillAll(getTabBackgroundColor());

    // Unterline für aktiven Tab
    int activeX = 0;
    if (currentMode == Mode::Pattern)
        activeX = getWidth() / 3;
    else if (currentMode == Mode::Timeline)
        activeX = getWidth() * 2 / 3;

    g.setColour(getTabActiveColor());
    g.fillRect(activeX, getHeight() - 3, getWidth() / 3, 3);
}

// ============================================================================
// UnifiedSequencerPanel Implementierung
// ============================================================================

UnifiedSequencerPanel::UnifiedSequencerPanel()
    : DockablePanel(PanelType::StepSequencer, "Unified Sequencer")
{
    sequencerModel = std::make_unique<UnifiedSequencerModel>();

    createViews();
    createModeTabs();

    setupModelCallbacks();
    setupViewCallbacks();

    // Setze Standard-Mode
    setSequencerMode(SequencerMode::TrackView);
}

UnifiedSequencerPanel::UnifiedSequencerPanel(PanelType type, const juce::String& panelName)
    : DockablePanel(type, panelName)
{
    sequencerModel = std::make_unique<UnifiedSequencerModel>();

    createViews();
    createModeTabs();

    setupModelCallbacks();
    setupViewCallbacks();

    // Setze Standard-Mode
    setSequencerMode(SequencerMode::TrackView);
}

UnifiedSequencerPanel::~UnifiedSequencerPanel()
{
    // Smart pointers räumen automatisch auf
}

void UnifiedSequencerPanel::createViews()
{
    // Track View
    trackView = std::make_unique<TrackViewComponent>();
    trackView->setModel(sequencerModel.get());
    addChildComponent(trackView.get());

    // Pattern View
    patternView = std::make_unique<PatternViewComponent>();
    patternView->setModel(sequencerModel.get());
    addChildComponent(patternView.get());

    // Timeline View
    timelineView = std::make_unique<TimelineViewComponent>();
    timelineView->setModel(sequencerModel.get());
    addChildComponent(timelineView.get());
}

void UnifiedSequencerPanel::createModeTabs()
{
    modeTabs = std::make_unique<ModeTabComponent>();
    modeTabs->setMode(ModeTabComponent::Mode::Track);

    modeTabs->onModeChanged = [this](ModeTabComponent::Mode mode)
    {
        switch (mode)
        {
            case ModeTabComponent::Mode::Track:
                setSequencerMode(SequencerMode::TrackView);
                break;
            case ModeTabComponent::Mode::Pattern:
                setSequencerMode(SequencerMode::PatternView);
                break;
            case ModeTabComponent::Mode::Timeline:
                setSequencerMode(SequencerMode::TimelineView);
                break;
        }
    };

    addAndMakeVisible(modeTabs.get());
}

void UnifiedSequencerPanel::setupModelCallbacks()
{
    sequencerModel->onStepChanged = [this](int trackIndex, int step, bool active)
    {
        // Views werden automatisch über das Model aktualisiert
    };

    sequencerModel->onTrackChanged = [this](int trackIndex)
    {
        // Views werden automatisch über das Model aktualisiert
    };

    sequencerModel->onPatternChanged = [this]()
    {
        // Views werden automatisch über das Model aktualisiert
    };
}

void UnifiedSequencerPanel::setupViewCallbacks()
{
    // Track View Callbacks
    if (trackView)
    {
        trackView->onTrackSelected = [this](int trackIndex)
        {
            // Track Auswahl im PanelManager signalisieren
            // TODO: Implementieren
        };
    }

    // Pattern View Callbacks
    if (patternView)
    {
        patternView->onStepChanged = [this](int track, int step, bool active)
        {
            // Model wird bereits im PatternViewComponent aktualisiert
        };

        patternView->onPatternChanged = [this]()
        {
            // Pattern wurde geändert
        };
    }
}

// ============================================================================
// Dependencies
// ============================================================================

void UnifiedSequencerPanel::setTrackManager(TrackManager* manager)
{
    trackManager = manager;

    // TODO: TrackManager Integration
}

void UnifiedSequencerPanel::setSampleManager(SampleManager* manager)
{
    sampleManager = manager;

    if (trackView)
    {
        trackView->setSampleManager(manager);
    }
}

void UnifiedSequencerPanel::setTimelineDependencies(TimelineData* data, RecordingManager* recorder)
{
    timelineData = data;
    recordingManager = recorder;

    if (timelineView)
    {
        timelineView->setTimelineData(data);
    }
}

// ============================================================================
// Sequencer Mode
// ============================================================================

void UnifiedSequencerPanel::setSequencerMode(SequencerMode mode)
{
    sequencerModel->setSequencerMode(mode);

    // Aktualisiere Mode-Tabs
    if (modeTabs)
    {
        switch (mode)
        {
            case SequencerMode::TrackView:
                modeTabs->setMode(ModeTabComponent::Mode::Track);
                break;
            case SequencerMode::PatternView:
                modeTabs->setMode(ModeTabComponent::Mode::Pattern);
                break;
            case SequencerMode::TimelineView:
                modeTabs->setMode(ModeTabComponent::Mode::Timeline);
                break;
        }
    }

    // Zeige entsprechenden View
    showView(mode);
}

// ============================================================================
// View Management
// ============================================================================

void UnifiedSequencerPanel::showView(SequencerMode mode)
{
    hideAllViews();

    switch (mode)
    {
        case SequencerMode::TrackView:
            if (trackView)
                trackView->setVisible(true);
            break;
        case SequencerMode::PatternView:
            if (patternView)
                patternView->setVisible(true);
            break;
        case SequencerMode::TimelineView:
            if (timelineView)
                timelineView->setVisible(true);
            break;
    }
}

void UnifiedSequencerPanel::hideAllViews()
{
    if (trackView)
        trackView->setVisible(false);
    if (patternView)
        patternView->setVisible(false);
    if (timelineView)
        timelineView->setVisible(false);
}

// ============================================================================
// DockablePanel Interface
// ============================================================================

void UnifiedSequencerPanel::prepareForUndock()
{
    // Aufräumen vor dem Undock
}

void UnifiedSequencerPanel::prepareForDock()
{
    // Vorbereiten für das Docken
}

void UnifiedSequencerPanel::onDockStateChanged(DockState newState)
{
    // Reagiere auf Dock-State-Änderung
}

// ============================================================================
// State Persistence
// ============================================================================

juce::ValueTree UnifiedSequencerPanel::saveState() const
{
    juce::ValueTree state("UnifiedSequencerPanel");

    // Speichere Sequencer Mode
    state.setProperty("sequencerMode", (int)sequencerModel->getSequencerMode(), nullptr);

    // TODO: Speichere Pattern-Daten

    return state;
}

void UnifiedSequencerPanel::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    // Restore Sequencer Mode
    if (state.hasProperty("sequencerMode"))
    {
        int modeValue = state.getProperty("sequencerMode");
        setSequencerMode(static_cast<SequencerMode>(modeValue));
    }

    // TODO: Restore Pattern-Daten
}

// ============================================================================
// Layout
// ============================================================================

juce::Rectangle<int> UnifiedSequencerPanel::getContentBounds() const
{
    auto bounds = getLocalBounds();

    // Platz für Mode-Tabs oben
    return bounds.withTrimmedTop(modeTabsHeight);
}

void UnifiedSequencerPanel::resized()
{
    // Mode-Tabs oben
    modeTabs->setBounds(0, 0, getWidth(), modeTabsHeight);

    // Content-Bereich
    auto contentBounds = getContentBounds();

    if (trackView && trackView->isVisible())
        trackView->setBounds(contentBounds);
    if (patternView && patternView->isVisible())
        patternView->setBounds(contentBounds);
    if (timelineView && timelineView->isVisible())
        timelineView->setBounds(contentBounds);
}
