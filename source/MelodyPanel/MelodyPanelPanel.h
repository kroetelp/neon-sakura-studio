// ============================================================================
// MelodyPanelPanel.h - DockablePanel Wrapper für MelodyPanel
// ============================================================================
//
// Wandelt den MelodyPanel in ein DockablePanel um, damit es über
// den DockingManager verwaltet werden kann.

#pragma once

#include "../DockablePanel.h"
#include "MelodyPanel.h"
#include "../Theme/ThemeManager.h"
#include "../MusicTheory.h"
#include <memory>

/**
 * MelodyPanelPanel - DockablePanel Wrapper für MelodyPanel
 *
 * Usage:
 *   1. MelodyPanelPanel erstellen
 *   2. setTargetTrack() aufrufen falls benötigt
 *   3. Beim DockingManager registrieren
 */
class MelodyPanelPanel : public DockablePanel
{
public:
    MelodyPanelPanel();
    ~MelodyPanelPanel() override;

    // === Target Track ===
    void setTargetTrack(int trackIndex);
    int getTargetTrack() const { return targetTrack; }

    // === Callbacks ===
    std::function<void(int, const std::vector<std::pair<int, int>>&)> onApplyMelody;

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
        return { 0, 0, 380, 550 };
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        return { 100, 100, 450, 650 };
    }

    int getMinimumWidth() const override { return 320; }
    int getMinimumHeight() const override { return 350; }

    // === Zugriff auf interne Component ===
    MelodyPanel* getMelodyPanel() const { return melodyPanel.get(); }

private:
    std::unique_ptr<MelodyPanel> melodyPanel;
    int targetTrack = 0;

    void createMelodyPanel();
    void updateMelodyPanelBounds();

    void resized() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MelodyPanelPanel)
};
