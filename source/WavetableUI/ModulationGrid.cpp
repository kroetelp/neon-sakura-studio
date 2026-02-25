#include "ModulationGrid.h"

ModulationGrid::ModulationGrid()
{
    buildGrid();
}

void ModulationGrid::connectToMatrix(ModulationMatrix* matrix)
{
    modMatrix = matrix;

    // Update cells from existing routings
    if (modMatrix)
    {
        for (int i = 0; i < modMatrix->getNumRoutings(); ++i)
        {
            const auto& routing = modMatrix->getRouting(i);
            for (auto& cell : cells)
            {
                if (cell.source == routing.source && cell.target == routing.target)
                {
                    cell.amount = routing.amount;
                    cell.active = routing.active;
                }
            }
        }
    }
    repaint();
}

void ModulationGrid::buildGrid()
{
    cells.clear();

    // Define visible sources (rows)
    ModulationSource sources[numVisibleSources] = {
        ModulationSource::LFO1,
        ModulationSource::LFO2,
        ModulationSource::Envelope1,
        ModulationSource::Envelope2,
        ModulationSource::Velocity,
        ModulationSource::ModWheel,
        ModulationSource::Macro1,
        ModulationSource::Macro2
    };

    // Define visible targets (columns)
    ModulationTarget targets[numVisibleTargets] = {
        ModulationTarget::Osc1_Pitch,
        ModulationTarget::Osc1_Morph,
        ModulationTarget::Osc1_Level,
        ModulationTarget::Osc2_Pitch,
        ModulationTarget::Filter_Cutoff,
        ModulationTarget::Filter_Resonance,
        ModulationTarget::Master_Level,
        ModulationTarget::FX1_Param
    };

    // Create cells
    for (int row = 0; row < numVisibleSources; ++row)
    {
        for (int col = 0; col < numVisibleTargets; ++col)
        {
            Cell cell;
            cell.source = sources[row];
            cell.target = targets[col];
            cell.amount = 0.0f;
            cell.active = false;
            cells.push_back(cell);
        }
    }
}

void ModulationGrid::resized()
{
    auto bounds = getLocalBounds().reduced(5);

    // Leave space for labels
    bounds.removeFromLeft(70);   // Source labels
    bounds.removeFromTop(30);    // Target labels

    int cellWidth = bounds.getWidth() / numVisibleTargets;
    int cellHeight = bounds.getHeight() / numVisibleSources;

    for (int row = 0; row < numVisibleSources; ++row)
    {
        for (int col = 0; col < numVisibleTargets; ++col)
        {
            int index = row * numVisibleTargets + col;
            cells[index].bounds = juce::Rectangle<int>(
                bounds.getX() + col * cellWidth,
                bounds.getY() + row * cellHeight,
                cellWidth - 1,
                cellHeight - 1
            );
        }
    }
}

ModulationGrid::Cell* ModulationGrid::getCellAtPosition(const juce::Point<int>& pos)
{
    for (auto& cell : cells)
    {
        if (cell.bounds.contains(pos))
            return &cell;
    }
    return nullptr;
}

juce::Colour ModulationGrid::getCellColour(float amount) const
{
    if (std::abs(amount) < 0.01f)
        return juce::Colours::darkgrey.withAlpha(0.3f);

    if (amount > 0)
        return getNeonCyan().withAlpha(amount);
    else
        return getNeonPink().withAlpha(-amount);
}

void ModulationGrid::mouseDown(const juce::MouseEvent& e)
{
    draggedCell = getCellAtPosition(e.getPosition());
    if (draggedCell)
    {
        dragStartY = e.y;
        dragStartAmount = draggedCell->amount;
    }
}

void ModulationGrid::mouseDrag(const juce::MouseEvent& e)
{
    if (!draggedCell)
        return;

    // Vertical drag controls amount
    int deltaY = dragStartY - e.y;  // Inverted: up = positive
    float newAmount = dragStartAmount + deltaY / 100.0f;
    newAmount = juce::jlimit(-1.0f, 1.0f, newAmount);

    draggedCell->amount = newAmount;
    draggedCell->active = std::abs(newAmount) > 0.01f;

    // Update modulation matrix
    if (modMatrix)
    {
        int routingIndex = modMatrix->findRouting(draggedCell->source, draggedCell->target);
        if (routingIndex >= 0)
        {
            modMatrix->setRoutingAmount(routingIndex, newAmount);
        }
        else if (draggedCell->active)
        {
            modMatrix->addRouting(draggedCell->source, draggedCell->target, newAmount);
        }
    }

    repaint();
}

void ModulationGrid::mouseUp(const juce::MouseEvent& e)
{
    draggedCell = nullptr;
}

void ModulationGrid::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Border
    g.setColour(getNeonPurple().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);

    // Title
    g.setColour(getNeonPurple());
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("MODULATION MATRIX", bounds.removeFromTop(15),
               juce::Justification::centred, false);

    // Draw source labels (rows)
    auto labelBounds = bounds.reduced(5);
    int cellHeight = (labelBounds.getHeight() - 15) / numVisibleSources;

    ModulationSource sources[numVisibleSources] = {
        ModulationSource::LFO1, ModulationSource::LFO2,
        ModulationSource::Envelope1, ModulationSource::Envelope2,
        ModulationSource::Velocity, ModulationSource::ModWheel,
        ModulationSource::Macro1, ModulationSource::Macro2
    };

    g.setFont(juce::Font(9.0f));
    for (int row = 0; row < numVisibleSources; ++row)
    {
        auto rowBounds = labelBounds.removeFromTop(cellHeight);
        g.setColour(juce::Colours::lightgrey);
        g.drawText(ModulationHelpers::getSourceName(sources[row]),
                   rowBounds.removeFromLeft(65), juce::Justification::right, false);
    }

    // Draw target labels (columns)
    ModulationTarget targets[numVisibleTargets] = {
        ModulationTarget::Osc1_Pitch, ModulationTarget::Osc1_Morph,
        ModulationTarget::Osc1_Level, ModulationTarget::Osc2_Pitch,
        ModulationTarget::Filter_Cutoff, ModulationTarget::Filter_Resonance,
        ModulationTarget::Master_Level, ModulationTarget::FX1_Param
    };

    auto topLabelBounds = getLocalBounds().reduced(5);
    topLabelBounds.removeFromTop(15);
    topLabelBounds.removeFromLeft(70);
    int cellWidth = topLabelBounds.getWidth() / numVisibleTargets;

    g.setFont(juce::Font(8.0f));
    for (int col = 0; col < numVisibleTargets; ++col)
    {
        auto colBounds = topLabelBounds.removeFromLeft(cellWidth);
        g.setColour(juce::Colours::lightgrey);
        g.drawText(ModulationHelpers::getTargetName(targets[col]),
                   colBounds.removeFromTop(15), juce::Justification::centred, false);
    }

    // Draw cells
    for (const auto& cell : cells)
    {
        g.setColour(getCellColour(cell.amount));
        g.fillRect(cell.bounds);

        // Border
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRect(cell.bounds);

        // Amount text
        if (std::abs(cell.amount) > 0.01f)
        {
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(8.0f));
            g.drawText(juce::String(cell.amount, 2), cell.bounds,
                       juce::Justification::centred, false);
        }
    }
}
