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
    rhythmExplorerButton->onClick = [this]() {
        if (dockingManager) {
            bool visible = rhythmExplorerButton->getToggleState();
            dockingManager->setPanelVisible(PanelType::RhythmExplorer, visible);
        }
    };

    // Melody Button
    melodyButton = std::make_unique<juce::TextButton>("Melody");
    melodyButton->setClickingTogglesState(true);
    melodyButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    melodyButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    melodyButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());
    melodyButton->onClick = [this]() {
        if (dockingManager) {
            bool visible = melodyButton->getToggleState();
            dockingManager->setPanelVisible(PanelType::MelodyPanel, visible);
        }
    };

    // Wavetable Button
    wavetableButton = std::make_unique<juce::TextButton>("Wavetable");
    wavetableButton->setClickingTogglesState(true);
    wavetableButton->setToggleState(true, juce::dontSendNotification);  // Default on
    wavetableButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    wavetableButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    wavetableButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());
    wavetableButton->onClick = [this]() {
        if (dockingManager) {
            bool visible = wavetableButton->getToggleState();
            dockingManager->setPanelVisible(PanelType::WavetableSynth, visible);
        }
    };

    // Timeline Button
    timelineButton = std::make_unique<juce::TextButton>("Timeline");
    timelineButton->setClickingTogglesState(true);
    timelineButton->setToggleState(true, juce::dontSendNotification);  // Default on
    timelineButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    timelineButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    timelineButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());
    timelineButton->onClick = [this]() {
        if (dockingManager) {
            bool visible = timelineButton->getToggleState();
            dockingManager->setPanelVisible(PanelType::Timeline, visible);
        }
    };

    // Plugin Browser Button (legacy - nicht im DockingManager)
    pluginBrowserButton = std::make_unique<juce::TextButton>("Plugins");
    pluginBrowserButton->setClickingTogglesState(true);
    pluginBrowserButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    pluginBrowserButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    pluginBrowserButton->setColour(juce::TextButton::buttonOnColourId, theme.getAccentColor());

    addAndMakeVisible(*rhythmExplorerButton);
    addAndMakeVisible(*melodyButton);
    addAndMakeVisible(*wavetableButton);
    addAndMakeVisible(*timelineButton);
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
        default: return nullptr;
    }
}

PanelTogglesBar::~PanelTogglesBar()
{
    stopTimer();
}
