#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modulation/ModulationSource.h"
#include "../Modulation/ModulationMatrix.h"
#include <vector>
#include <atomic>

/**
 * ModulationGrid - Drag-and-drop modulation matrix with live visual feedback
 *
 * Grid layout:
 * - Rows: Modulation sources (LFOs, Envelopes, etc.)
 * - Columns: Modulation targets (Osc params, Filter, etc.)
 *
 * Interaction:
 * - Click and drag vertically to set modulation amount
 * - Color intensity shows modulation depth
 * - Live pulsing/glowing effect shows current modulation activity
 */
class ModulationGrid : public juce::Component,
                       private juce::Timer
{
public:
    ModulationGrid();
    ~ModulationGrid() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Connect to modulation matrix
    void connectToMatrix(ModulationMatrix* matrix);

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Enable/disable live feedback (starts/stops timer)
    void setLiveFeedbackEnabled(bool enabled);
    bool isLiveFeedbackEnabled() const { return liveFeedbackEnabled; }

private:
    ModulationMatrix* modMatrix = nullptr;

    // Grid cell structure
    struct Cell
    {
        juce::Rectangle<int> bounds;
        ModulationSource source;
        ModulationTarget target;
        float amount = 0.0f;           // Set modulation amount
        float currentValue = 0.0f;     // Live modulation value (from matrix)
        bool active = false;
    };

    std::vector<Cell> cells;
    Cell* draggedCell = nullptr;
    int dragStartY = 0;
    float dragStartAmount = 0.0f;

    // Visible sources and targets
    static constexpr int numVisibleSources = 8;
    static constexpr int numVisibleTargets = 8;

    // Live feedback
    bool liveFeedbackEnabled = false;
    float animationPhase = 0.0f;  // For pulsing animation (0.0 - 1.0)

    // Colors
    static juce::Colour getNeonPink() { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getNeonPurple() { return juce::Colour(180, 0, 255); }
    static juce::Colour getNeonGreen() { return juce::Colour(0, 255, 128); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

    void buildGrid();
    Cell* getCellAtPosition(const juce::Point<int>& pos);
    juce::Colour getCellColour(float amount) const;
    juce::Colour getLiveFeedbackColour(float currentValue, float amount) const;

    // Timer callback for live updates
    void timerCallback() override;
    void updateLiveValues();
};
