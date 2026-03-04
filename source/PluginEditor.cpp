#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "WavetableUI/NeonSakuraLookAndFeel.h"
#include "Theme/ThemeManager.h"

//==============================================================================
NeonSakuraPluginEditor::NeonSakuraPluginEditor(NeonSakuraProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Set up custom look and feel
    lookAndFeel = std::make_unique<NeonSakuraLookAndFeel>();
    setLookAndFeel(lookAndFeel.get());

    // Set size constraints for DAW compatibility
    setResizable(true, true);
    setResizeLimits(minEditorWidth, minEditorHeight, 1600, 1000);
    setSize(defaultWidth, defaultHeight);

    // Create the synth editor using the processor's engine
    auto& wavetableEngine = processor.getWavetableEngine();
    synthEditor = std::make_unique<WavetableSynthEditor>(wavetableEngine);

    // Add as child component
    addAndMakeVisible(synthEditor.get());

    // Start timer for UI updates (30 Hz)
    startTimerHz(30);
}

NeonSakuraPluginEditor::~NeonSakuraPluginEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void NeonSakuraPluginEditor::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();

    // Fill background with theme
    g.fillAll(theme.getBackgroundColor());

    // Draw subtle border
    g.setColour(theme.getPanelBorderColor());
    g.drawRect(getLocalBounds().toFloat(), 2.0f);
}

void NeonSakuraPluginEditor::resized()
{
    // The synth editor fills the entire plugin window
    if (synthEditor)
        synthEditor->setBounds(getLocalBounds().reduced(5));
}

//==============================================================================
void NeonSakuraPluginEditor::timerCallback()
{
    // Trigger repaint for any animated components
    // The WavetableSynthEditor has its own timer for oscilloscope updates
    // This timer is for any additional plugin-level animations
    repaint();
}
