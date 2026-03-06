#include "PanelTogglesBar.h"
#include "../Theme/ThemeManager.h"

PanelTogglesBar::PanelTogglesBar()
{
    auto& theme = ThemeManager::getInstance();

    // Rhythm Explorer Button
    rhythmExplorerButton = std::make_unique<juce::TextButton>("Rhythm");
    rhythmExplorerButton->setClickingTogglesState(true);
    rhythmExplorerButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    rhythmExplorerButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    rhythmExplorerButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());

    // Melody Button
    melodyButton = std::make_unique<juce::TextButton>("Melody");
    melodyButton->setClickingTogglesState(true);
    melodyButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    melodyButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    melodyButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());

    // Wavetable Button
    wavetableButton = std::make_unique<juce::TextButton>("Wavetable");
    wavetableButton->setClickingTogglesState(true);
    wavetableButton->setToggleState(true, juce::dontSendNotification);  // Default on
    wavetableButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    wavetableButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    wavetableButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());

    // Timeline Button
    timelineButton = std::make_unique<juce::TextButton>("Timeline");
    timelineButton->setClickingTogglesState(true);
    timelineButton->setToggleState(true, juce::dontSendNotification);  // Default on
    timelineButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    timelineButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    timelineButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());

    // Step Sequencer Button
    stepSequencerButton = std::make_unique<juce::TextButton>("Step Sequencer");
    stepSequencerButton->setClickingTogglesState(true);
    stepSequencerButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    stepSequencerButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    stepSequencerButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());

    // Plugin Browser Button (legacy - nicht im DockingManager)
    pluginBrowserButton = std::make_unique<juce::TextButton>("Plugins");
    pluginBrowserButton->setClickingTogglesState(true);
    pluginBrowserButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    pluginBrowserButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    pluginBrowserButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());
    pluginBrowserButton->onClick = [this]() {
        if (onPluginBrowserToggled)
            onPluginBrowserToggled(true);
    };

    addAndMakeVisible(*rhythmExplorerButton);
    addAndMakeVisible(*melodyButton);
    addAndMakeVisible(*wavetableButton);
    addAndMakeVisible(*timelineButton);
    addAndMakeVisible(*stepSequencerButton);
    addAndMakeVisible(*pluginBrowserButton);
}

void PanelTogglesBar::resized()
{
    auto bounds = getLocalBounds();
    auto height = bounds.getHeight();
    auto buttonWidth = 80;
    auto gap = 5;
    auto x = 0;

    // Rhythm Explorer Button
    rhythmExplorerButton->setBounds(x, 0, buttonWidth, height);
    x += buttonWidth + gap;

    // Melody Button
    melodyButton->setBounds(x, 0, buttonWidth, height);
    x += buttonWidth + gap;

    // Wavetable Button
    wavetableButton->setBounds(x, 0, buttonWidth, height);
    x += buttonWidth + gap;

    // Timeline Button
    timelineButton->setBounds(x, 0, buttonWidth, height);
    x += buttonWidth + gap;

    // Step Sequencer Button
    stepSequencerButton->setBounds(x, 0, 110, height);
    x += 110 + gap;

    // Plugin Browser Button
    pluginBrowserButton->setBounds(x, 0, buttonWidth, height);
}

// ============================================================================
// DockingManager Integration
// ============================================================================

void PanelTogglesBar::setDockingManager(DockingManager* manager)
{
    dockingManager = manager;

    if (dockingManager)
    {
        // Jetzt den DockingManager setzen - Callbacks einrichten
        // Rhythm Explorer Callback
        rhythmExplorerButton->onClick = [this]() {
            // Toggle-Button hat den Zustand bereits geändert
            // Wir prüfen den neuen Zustand (inverted, da wir vor dem Click prüfen wollen)
            bool newState = !dockingManager->isPanelVisible(PanelType::RhythmExplorer);
            if (dockingManager)
                dockingManager->setPanelVisible(PanelType::RhythmExplorer, newState);
        };

        // Melody Callback
        melodyButton->onClick = [this]() {
            bool newState = !dockingManager->isPanelVisible(PanelType::MelodyPanel);
            if (dockingManager)
                dockingManager->setPanelVisible(PanelType::MelodyPanel, newState);
        };

        // Wavetable Callback
        wavetableButton->onClick = [this]() {
            bool newState = !dockingManager->isPanelVisible(PanelType::WavetableSynth);
            if (dockingManager)
                dockingManager->setPanelVisible(PanelType::WavetableSynth, newState);
        };

        // Timeline Callback
        timelineButton->onClick = [this]() {
            bool newState = !dockingManager->isPanelVisible(PanelType::Timeline);
            if (dockingManager)
                dockingManager->setPanelVisible(PanelType::Timeline, newState);
        };

        // Step Sequencer Callback
        stepSequencerButton->onClick = [this]() {
            bool newState = !dockingManager->isPanelVisible(PanelType::StepSequencer);
            if (dockingManager)
                dockingManager->setPanelVisible(PanelType::StepSequencer, newState);
        };

        // Timer starten für periodische Sync
        startTimerHz(30);  // 30 Hz - oft genug für UI-Updates
        syncButtonStates();
    }
    else
    {
        stopTimer();
    }
}

void PanelTogglesBar::syncButtonStates()
{
    if (!dockingManager)
        return;

    // Button-Status syncen mit DockingManager
    rhythmExplorerButton->setToggleState(dockingManager->isPanelVisible(PanelType::RhythmExplorer), juce::dontSendNotification);
    melodyButton->setToggleState(dockingManager->isPanelVisible(PanelType::MelodyPanel), juce::dontSendNotification);
    wavetableButton->setToggleState(dockingManager->isPanelVisible(PanelType::WavetableSynth), juce::dontSendNotification);
    timelineButton->setToggleState(dockingManager->isPanelVisible(PanelType::Timeline), juce::dontSendNotification);
    stepSequencerButton->setToggleState(dockingManager->isPanelVisible(PanelType::StepSequencer), juce::dontSendNotification);
}

void PanelTogglesBar::timerCallback()
{
    // Periodisch Button-Status syncen
    syncButtonStates();
}

// ============================================================================
// Button <-> PanelType Mapping
// ============================================================================

PanelType PanelTogglesBar::getPanelTypeForButton(juce::TextButton* button) const
{
    if (button == rhythmExplorerButton.get()) return PanelType::RhythmExplorer;
    if (button == melodyButton.get()) return PanelType::MelodyPanel;
    if (button == wavetableButton.get()) return PanelType::WavetableSynth;
    if (button == timelineButton.get()) return PanelType::Timeline;
    if (button == stepSequencerButton.get()) return PanelType::StepSequencer;

    return PanelType::Unknown;
}

juce::TextButton* PanelTogglesBar::getButtonForPanelType(PanelType type) const
{
    switch (type)
    {
        case PanelType::RhythmExplorer: return rhythmExplorerButton.get();
        case PanelType::MelodyPanel: return melodyButton.get();
        case PanelType::WavetableSynth: return wavetableButton.get();
        case PanelType::Timeline: return timelineButton.get();
        case PanelType::StepSequencer: return stepSequencerButton.get();
        default: return nullptr;
    }
}

PanelTogglesBar::~PanelTogglesBar()
{
    stopTimer();
}
