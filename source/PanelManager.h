#pragma once

/**
 * PanelManager - Manages side panels and floating windows
 *
 * Responsibilities:
 * - RhythmExplorer panel management
 * - MelodyPanel management
 * - WavetableSynth standalone window management
 * - Per-track Wavetable Editor window management
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>

class RhythmExplorer;
class MelodyPanel;
class WavetableSynthEditor;
class WavetableEngine;
class WavetableParams;

class PanelManager
{
public:
    PanelManager();
    ~PanelManager();

    // Rhythm Explorer panel
    RhythmExplorer& getRhythmExplorer();
    const RhythmExplorer& getRhythmExplorer() const;

    void setRhythmExplorerVisible(bool visible);
    bool isRhythmExplorerVisible() const { return rhythmExplorerVisible; }

    // Melody Panel
    MelodyPanel& getMelodyPanel();
    const MelodyPanel& getMelodyPanel() const;

    void setMelodyPanelVisible(bool visible);
    bool isMelodyPanelVisible() const { return melodyPanelVisible; }

    // Wavetable Synth (standalone window with WavetableEngine reference)
    void setWavetableSynthVisible(bool visible, WavetableEngine* engine = nullptr);
    bool isWavetableSynthVisible() const { return wavetableSynthVisible; }

    // Per-track Wavetable Editor (uses shared params)
    void openTrackWavetableEditor(int trackIndex, std::shared_ptr<WavetableParams> params);
    void closeTrackWavetableEditor();
    bool isTrackWavetableEditorOpen() const { return trackWavetableWindow != nullptr && trackWavetableWindow->isVisible(); }
    int getCurrentEditingTrack() const { return currentEditingTrack; }

    // Component getters (for MainComponent layout)
    RhythmExplorer* getRhythmExplorerComponent() { return rhythmExplorer.get(); }
    MelodyPanel* getMelodyPanelComponent() { return melodyPanel.get(); }
    juce::DocumentWindow* getWavetableSynthWindow() { return wavetableSynthWindow.get(); }
    juce::DocumentWindow* getTrackWavetableWindow() { return trackWavetableWindow.get(); }

    // Window background color helper
    static juce::Colour getDarkBackground();

private:
    // Side panels
    std::unique_ptr<RhythmExplorer> rhythmExplorer;
    bool rhythmExplorerVisible = false;

    std::unique_ptr<MelodyPanel> melodyPanel;
    bool melodyPanelVisible = false;

    // Wavetable synth (standalone window)
    std::unique_ptr<WavetableSynthEditor> wavetableSynthEditor;
    std::unique_ptr<juce::DocumentWindow> wavetableSynthWindow;
    bool wavetableSynthVisible = false;

    // Per-track wavetable editor
    std::unique_ptr<WavetableSynthEditor> trackWavetableEditor;
    std::unique_ptr<juce::DocumentWindow> trackWavetableWindow;
    int currentEditingTrack = -1;

    // Helper to create document window
    std::unique_ptr<juce::DocumentWindow> createDocumentWindow(const juce::String& title, int width, int height);
};
