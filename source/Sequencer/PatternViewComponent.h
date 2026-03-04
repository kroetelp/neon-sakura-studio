// ============================================================================
// PatternViewComponent.h - Pattern-Grid View für UnifiedSequencerPanel
// ============================================================================
//
// Diese View zeigt ein Pattern-Grid mit Drag-to-Draw Unterstützung,
// ähnlich zum ursprünglichen StepSequencerPanel.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur.h>
#include <functional>
#include <memory>
#include "../StepSequencer/NeonStepButton.h"
#include "../Theme/ThemeManager.h"

// Forward declarations
class UnifiedSequencerModel;

/**
 * PatternViewComponent - Grid-basierte Pattern-Darstellung
 */
class PatternViewComponent : public juce::Component
{
public:
    PatternViewComponent();
    ~PatternViewComponent() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Configuration ===
    void setModel(UnifiedSequencerModel* model);
    void setNumTracks(int numTracks);
    void setNumSteps(int numSteps);
    int getNumTracks() const { return numTracks; }
    int getNumSteps() const { return numSteps; }

    // === Mouse Handling ===
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // === Callbacks ===
    std::function<void(int track, int step, bool active)> onStepChanged;
    std::function<void()> onPatternChanged;

    // === Pattern Access ===
    bool isStepActive(int track, int step) const;
    void setStepActive(int track, int step, bool active);

    // === Playback ===
    void setCurrentStep(int step);
    int getCurrentStep() const { return currentStep; }

private:
    UnifiedSequencerModel* model = nullptr;
    int numTracks = 8;
    int numSteps = 16;

    // Step Buttons Grid [track][step]
    std::vector<std::vector<std::unique_ptr<NeonStepButton>>> stepButtons;

    // === UI Controls ===
    juce::TextButton clearAllButton;
    juce::TextButton randomizeButton;
    juce::Label statusLabel;

    // === Layout Constants ===
    static constexpr int trackLabelWidth = 80;
    static constexpr int stepGap = 4;
    static constexpr int trackGap = 8;
    static constexpr int headerHeight = 40;
    static constexpr int footerHeight = 40;

    // === Track Labels ===
    std::vector<std::unique_ptr<juce::Label>> trackLabels;

    // === Playhead ===
    int currentStep = 0;
    bool showPlayhead = true;

    // === Drag-to-Draw State ===
    bool isDragging = false;
    bool dragState = false;
    NeonStepButton* lastDragButton = nullptr;

    // === Helper Methods ===
    void createStepGrid();
    void createTrackLabels();
    void updateLayout();
    void handleDragToDraw(const juce::Point<int>& position);
    NeonStepButton* getButtonAtPosition(const juce::Point<int>& position);

    // === Playhead ===
    void drawPlayhead(juce::Graphics& g, int step);

    // === Colors ===
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getBackgroundColor(); }
    juce::Colour getStepInactive() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatternViewComponent)
};
