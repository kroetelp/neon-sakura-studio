#include "PanelManager.h"
#include "RhythmExplorer.h"
#include "MelodyPanel.h"
#include "WavetableUI/WavetableSynthEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>

PanelManager::PanelManager()
{
    // Create side panels (but don't show them yet)
    rhythmExplorer = std::make_unique<RhythmExplorer>();
    rhythmExplorer->setVisible(false);

    melodyPanel = std::make_unique<MelodyPanel>();
    melodyPanel->setVisible(false);
}

PanelManager::~PanelManager()
{
    // Clean up windows first
    if (trackWavetableWindow)
    {
        trackWavetableWindow->setVisible(false);
        trackWavetableWindow.reset();
    }
    if (wavetableSynthWindow)
    {
        wavetableSynthWindow->setVisible(false);
        wavetableSynthWindow.reset();
    }
}

// Rhythm Explorer
RhythmExplorer& PanelManager::getRhythmExplorer()
{
    return *rhythmExplorer;
}

const RhythmExplorer& PanelManager::getRhythmExplorer() const
{
    return *rhythmExplorer;
}

void PanelManager::setRhythmExplorerVisible(bool visible)
{
    rhythmExplorerVisible = visible;
    rhythmExplorer->setVisible(visible);
}

// Melody Panel
MelodyPanel& PanelManager::getMelodyPanel()
{
    return *melodyPanel;
}

const MelodyPanel& PanelManager::getMelodyPanel() const
{
    return *melodyPanel;
}

void PanelManager::setMelodyPanelVisible(bool visible)
{
    melodyPanelVisible = visible;
    melodyPanel->setVisible(visible);
}

// Wavetable Synth (standalone)
void PanelManager::setWavetableSynthVisible(bool visible, WavetableEngine* engine)
{
    wavetableSynthVisible = visible;

    if (visible && engine)
    {
        if (!wavetableSynthEditor)
        {
            wavetableSynthEditor = std::make_unique<WavetableSynthEditor>(*engine);
        }

        if (!wavetableSynthWindow)
        {
            wavetableSynthWindow = createDocumentWindow("Wavetable Synth", 1050, 750);
            wavetableSynthWindow->setContentOwned(wavetableSynthEditor.get(), true);
        }

        wavetableSynthWindow->setVisible(true);
        wavetableSynthWindow->toFront(true);
    }
    else if (wavetableSynthWindow)
    {
        wavetableSynthWindow->setVisible(false);
    }
}

// Per-track Wavetable Editor
void PanelManager::openTrackWavetableEditor(int trackIndex, std::shared_ptr<WavetableParams> params, std::shared_ptr<WavetableData> wavetableData)
{
    if (!params)
        return;

    currentEditingTrack = trackIndex;

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
        trackWavetableWindow = createDocumentWindow(
            "Track " + juce::String(trackIndex + 1) + " Wavetable", 1050, 750);
        trackWavetableWindow->setContentOwned(trackWavetableEditor.get(), true);
    }
    else
    {
        // Update window title for new track
        trackWavetableWindow->setName("Track " + juce::String(trackIndex + 1) + " Wavetable");
    }

    trackWavetableWindow->setVisible(true);
    trackWavetableWindow->toFront(true);
}

void PanelManager::closeTrackWavetableEditor()
{
    if (trackWavetableWindow)
    {
        trackWavetableWindow->setVisible(false);
    }
    currentEditingTrack = -1;
}

// Helper to create document window
std::unique_ptr<juce::DocumentWindow> PanelManager::createDocumentWindow(const juce::String& title, int width, int height)
{
    auto window = std::make_unique<juce::DocumentWindow>(
        title,
        getDarkBackground(),
        juce::DocumentWindow::allButtons,
        true
    );

    window->setCentrePosition(600, 400);
    window->setSize(width, height);

    return window;
}

juce::Colour PanelManager::getDarkBackground()
{
    return juce::Colour(15, 15, 25);
}
