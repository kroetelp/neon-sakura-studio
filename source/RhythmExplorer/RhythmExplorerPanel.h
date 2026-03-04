// ============================================================================
// RhythmExplorerPanel.h - DockablePanel Wrapper für RhythmExplorer
// ============================================================================
//
// Wandelt den RhythmExplorer in ein DockablePanel um, damit es über
// den DockingManager verwaltet werden kann.

#pragma once

#include "../DockablePanel.h"
#include "RhythmExplorer.h"
#include "../Theme/ThemeManager.h"
#include <memory>

/**
 * RhythmExplorerPanel - DockablePanel Wrapper für RhythmExplorer
 *
 * Usage:
 *   1. RhythmExplorerPanel erstellen
 *   2. setTargetTrack() aufrufen falls benötigt
 *   3. Beim DockingManager registrieren
 */
class RhythmExplorerPanel : public DockablePanel
{
public:
    RhythmExplorerPanel();
    ~RhythmExplorerPanel() override;

    // === Target Track ===
    void setTargetTrack(int trackIndex);
    int getTargetTrack() const { return targetTrack; }

    // === Callbacks ===
    std::function<void(int, const std::vector<int>&, bool)> onApplyPattern;
    std::function<void(int, const std::vector<int>&)> onApplyFill;

    // === DockablePanel Interface ===
    void prepareForUndock() override;
    void prepareForDock() override;
    void onDockStateChanged(DockState newState) override;

    // === State Persistence ===
    juce::ValueTree saveState() const override;
    void restoreState(const juce::ValueTree& state);

    // === Preferred Sizes ===
    juce::Rectangle<int> getPreferredDockedBounds() const override
    {
        return { 0, 0, 320, 500 };
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        return { 100, 100, 400, 600 };
    }

    int getMinimumWidth() const override { return 280; }
    int getMinimumHeight() const override { return 300; }

    // === Zugriff auf interne Component ===
    RhythmExplorer* getRhythmExplorer() const { return rhythmExplorer.get(); }

private:
    std::unique_ptr<RhythmExplorer> rhythmExplorer;
    int targetTrack = 0;

    void createRhythmExplorer();
    void updateRhythmExplorerBounds();

    void resized() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmExplorerPanel)
};
