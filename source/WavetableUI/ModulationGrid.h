#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Modulation/ModulationSource.h"
#include "../Modulation/ModulationMatrix.h"
#include <vector>

/**
 * ModulationGrid - Drag-and-drop modulation matrix
 *
 * Grid layout:
 * - Rows: Modulation sources (LFOs, Envelopes, etc.)
 * - Columns: Modulation targets (Osc params, Filter, etc.)
 *
 * Interaction:
 * - Click and drag vertically to set modulation amount
 * - Color intensity shows modulation depth
 */
class ModulationGrid : public juce::Component
{
public:
    ModulationGrid();
    ~ModulationGrid() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Connect to modulation matrix
    void connectToMatrix(ModulationMatrix* matrix);

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    ModulationMatrix* modMatrix = nullptr;

    // Grid cell structure
    struct Cell
    {
        juce::Rectangle<int> bounds;
        ModulationSource source;
        ModulationTarget target;
        float amount = 0.0f;
        bool active = false;
    };

    std::vector<Cell> cells;
    Cell* draggedCell = nullptr;
    int dragStartY = 0;
    float dragStartAmount = 0.0f;

    // Visible sources and targets
    static constexpr int numVisibleSources = 8;
    static constexpr int numVisibleTargets = 8;

    // Colors
    static juce::Colour getNeonPink() { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getNeonPurple() { return juce::Colour(180, 0, 255); }
    static juce::Colour getNeonGreen() { return juce::Colour(0, 255, 128); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

    void buildGrid();
    Cell* getCellAtPosition(const juce::Point<int>& pos);
    juce::Colour getCellColour(float amount) const;
};
