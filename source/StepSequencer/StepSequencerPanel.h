// ============================================================================
// StepSequencerPanel.h - Step Sequencer Panel mit Drag-to-Draw
// ============================================================================
//
// Ein kompletter Step Sequencer mit:
// - Konfigurierbare Anzahl von Tracks und Steps
// - Drag-to-Draw Unterstützung
// - Neon Sakura Design
// - Integration mit Track-System

#pragma once

#include "../DockablePanel.h"
#include "NeonStepButton.h"
#include <vector>
#include <array>
#include <functional>

// Forward declarations
class TrackManager;

/**
 * StepSequencerPanel - DockablePanel für den Step Sequencer
 *
 * Features:
 * - Grid aus NeonStepButtons
 * - Drag-to-Draw (Maus gedrückt halten und wischen)
 * - Pro-Track Step-Pattern
 * - Integration mit TrackManager
 */
class StepSequencerPanel : public DockablePanel
{
public:
    // Konfiguration
    static constexpr int defaultNumTracks = 8;
    static constexpr int defaultNumSteps = 16;

    StepSequencerPanel();
    ~StepSequencerPanel() override;

    // === Configuration ===
    void setNumTracks(int numTracks);
    void setNumSteps(int numSteps);
    int getNumTracks() const { return numTracks; }
    int getNumSteps() const { return numSteps; }

    // === TrackManager Integration ===
    void setTrackManager(TrackManager* manager);
    void setCurrentTrack(int trackIndex);

    // === Pattern Access ===
    bool isStepActive(int track, int step) const;
    void setStepActive(int track, int step, bool active);
    void clearTrack(int track);
    void clearAll();

    // === Pattern Callbacks ===
    std::function<void(int track, int step, bool active)> onStepChanged;
    std::function<void()> onPatternChanged;

    // === DockablePanel Interface ===
    juce::Rectangle<int> getPreferredDockedBounds() const override
    {
        return { 0, 0, 600, 400 };
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        return { 100, 100, 800, 500 };
    }

    int getMinimumWidth() const override { return 400; }
    int getMinimumHeight() const override { return 200; }

    // === Component Override ===
    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Mouse Handling für Drag-to-Draw ===
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    int numTracks = defaultNumTracks;
    int numSteps = defaultNumSteps;

    TrackManager* trackManager = nullptr;
    int currentTrack = 0;

    // Step Buttons Grid [track][step]
    std::vector<std::vector<std::unique_ptr<NeonStepButton>>> stepButtons;

    // === Layout ===
    static constexpr int trackLabelWidth = 80;
    static constexpr int stepGap = 4;
    static constexpr int trackGap = 8;
    static constexpr int headerHeight = 40;
    static constexpr int footerHeight = 30;

    // === Track Labels ===
    std::vector<std::unique_ptr<juce::Label>> trackLabels;

    // === Playhead ===
    int currentStep = 0;
    bool showPlayhead = true;

    // === Drag-to-Draw State ===
    bool isDragging = false;
    bool dragState = false;  // Der State, der beim Drag gesetzt wird
    NeonStepButton* lastDragButton = nullptr;

    // === Helper Methods ===
    void createStepGrid();
    void updateLayout();
    void handleDragToDraw(const juce::Point<int>& position);
    NeonStepButton* getButtonAtPosition(const juce::Point<int>& position);

    // === Playhead ===
    void drawPlayhead(juce::Graphics& g, int step);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencerPanel)
};
