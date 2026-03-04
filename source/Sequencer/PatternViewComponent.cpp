// ============================================================================
// PatternViewComponent.cpp - Pattern-Grid View Implementierung
// ============================================================================

#include "PatternViewComponent.h"
#include "UnifiedSequencerModel.h"

// ============================================================================
// PatternViewComponent Implementierung
// ============================================================================

PatternViewComponent::PatternViewComponent()
{
    // Header Controls
    clearAllButton.setButtonText("Clear All");
    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 20, 60));
    clearAllButton.onClick = [this]()
    {
        if (model)
        {
            model->clearAll();
        }
        if (onPatternChanged)
            onPatternChanged();
    };
    addAndMakeVisible(clearAllButton);

    randomizeButton.setButtonText("Randomize");
    randomizeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(20, 60, 60));
    randomizeButton.onClick = [this]()
    {
        // Randomisiere alle Steps
        if (model)
        {
            for (int t = 0; t < numTracks; ++t)
            {
                for (int s = 0; s < numSteps; ++s)
                {
                    bool active = (juce::Random::getSystemRandom().nextFloat() < 0.3f);
                    model->setStepActive(t, s, active);
                }
            }
        }
        if (onPatternChanged)
            onPatternChanged();
    };
    addAndMakeVisible(randomizeButton);

    statusLabel.setText("Pattern Grid - Drag to Draw", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    statusLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(statusLabel);

    // Create UI
    createTrackLabels();
    createStepGrid();
}

void PatternViewComponent::createTrackLabels()
{
    trackLabels.clear();

    for (int i = 0; i < numTracks; ++i)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText("T" + juce::String(i + 1), juce::dontSendNotification);
        label->setColour(juce::Label::textColourId, getNeonCyan());
        label->setFont(juce::Font(12.0f, juce::Font::bold));
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label.get());
        trackLabels.push_back(std::move(label));
    }
}

void PatternViewComponent::createStepGrid()
{
    // Lösche alte Buttons
    stepButtons.clear();

    // Neue Grid erstellen
    stepButtons.resize(numTracks);

    for (int t = 0; t < numTracks; ++t)
    {
        stepButtons[t].resize(numSteps);

        for (int s = 0; s < numSteps; ++s)
        {
            auto button = std::make_unique<NeonStepButton>();

            // Setze Farben
            button->setActiveColour(getNeonPink());
            button->setInactiveColour(getStepInactive());
            button->setGlowColour(getNeonPink());

            // Callback für Drag-to-Draw
            button->onDragEnter = [this, t, s](NeonStepButton*, bool isEnter)
            {
                if (isEnter && isDragging)
                {
                    setStepActive(t, s, dragState);
                }
            };

            // Callback für State-Änderung
            button->onStateChanged = [this, t, s](bool newState)
            {
                if (onStepChanged)
                    onStepChanged(t, s, newState);

                if (model)
                    model->setStepActive(t, s, newState);
            };

            addAndMakeVisible(button.get());
            stepButtons[t][s] = std::move(button);
        }
    }
}

void PatternViewComponent::setModel(UnifiedSequencerModel* model)
{
    this->model = model;

    // Aktualisiere alle Buttons mit dem Model
    if (model)
    {
        for (int t = 0; t < numTracks; ++t)
        {
            const auto& pattern = model->getPattern(t);
            for (int s = 0; s < numSteps && s < pattern.activeSteps.size(); ++s)
            {
                if (stepButtons[t][s])
                {
                    stepButtons[t][s]->setActive(pattern.activeSteps[s], false);
                }
            }
        }
    }
}

void PatternViewComponent::setNumTracks(int numTracks)
{
    if (this->numTracks == numTracks)
        return;

    this->numTracks = juce::jlimit(1, 16, numTracks);
    createTrackLabels();
    createStepGrid();
    resized();
}

void PatternViewComponent::setNumSteps(int numSteps)
{
    if (this->numSteps == numSteps)
        return;

    this->numSteps = juce::jlimit(4, 64, numSteps);
    createStepGrid();
    resized();
}

// ============================================================================
// Mouse Handling für Drag-to-Draw
// ============================================================================

void PatternViewComponent::mouseDown(const juce::MouseEvent& e)
{
    auto* button = getButtonAtPosition(e.getPosition());

    if (button)
    {
        isDragging = true;
        dragState = !button->isActive();
        button->setActive(dragState);
    }
}

void PatternViewComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging)
        return;

    auto* button = getButtonAtPosition(e.getPosition());

    if (button && button != lastDragButton)
    {
        button->setActive(dragState);
        lastDragButton = button;
    }
}

void PatternViewComponent::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    lastDragButton = nullptr;

    if (onPatternChanged)
        onPatternChanged();
}

NeonStepButton* PatternViewComponent::getButtonAtPosition(const juce::Point<int>& position)
{
    for (int t = 0; t < numTracks; ++t)
    {
        for (int s = 0; s < numSteps; ++s)
        {
            if (stepButtons[t][s] && stepButtons[t][s]->getBounds().contains(position))
            {
                return stepButtons[t][s].get();
            }
        }
    }
    return nullptr;
}

// ============================================================================
// Pattern Access
// ============================================================================

bool PatternViewComponent::isStepActive(int track, int step) const
{
    if (track >= 0 && track < numTracks &&
        step >= 0 && step < numSteps &&
        stepButtons[track][step])
    {
        return stepButtons[track][step]->isActive();
    }
    return false;
}

void PatternViewComponent::setStepActive(int track, int step, bool active)
{
    if (track >= 0 && track < numTracks &&
        step >= 0 && step < numSteps &&
        stepButtons[track][step])
    {
        stepButtons[track][step]->setActive(active);

        if (model)
            model->setStepActive(track, step, active);
    }
}

// ============================================================================
// Playback
// ============================================================================

void PatternViewComponent::setCurrentStep(int step)
{
    currentStep = step;
    repaint();
}

// ============================================================================
// Layout & Paint
// ============================================================================

void PatternViewComponent::resized()
{
    auto bounds = getLocalBounds();

    // Header
    auto headerBounds = bounds.removeFromTop(headerHeight);

    int buttonWidth = 100;
    clearAllButton.setBounds(headerBounds.removeFromLeft(buttonWidth).reduced(4));
    randomizeButton.setBounds(headerBounds.removeFromLeft(buttonWidth).reduced(4));
    statusLabel.setBounds(headerBounds.reduced(4));

    // Footer
    bounds.removeFromBottom(footerHeight);

    // Track Labels
    auto trackLabelBounds = bounds.removeFromLeft(trackLabelWidth);
    int trackLabelHeight = trackLabelBounds.getHeight() / numTracks;

    for (int i = 0; i < numTracks && i < trackLabels.size(); ++i)
    {
        trackLabels[i]->setBounds(trackLabelBounds.removeFromTop(trackLabelHeight));
    }

    // Step Grid
    auto gridBounds = bounds;
    int stepWidth = (gridBounds.getWidth() - (numSteps - 1) * stepGap) / numSteps;
    int trackHeight = (gridBounds.getHeight() - (numTracks - 1) * trackGap) / numTracks;

    for (int t = 0; t < numTracks; ++t)
    {
        for (int s = 0; s < numSteps; ++s)
        {
            if (stepButtons[t][s])
            {
                auto buttonBounds = juce::Rectangle<int>(
                    s * (stepWidth + stepGap),
                    t * (trackHeight + trackGap),
                    stepWidth,
                    trackHeight
                );
                stepButtons[t][s]->setBounds(buttonBounds);
            }
        }
    }
}

void PatternViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());

    // Zeichne Playhead
    if (showPlayhead)
    {
        drawPlayhead(g, currentStep);
    }
}

void PatternViewComponent::drawPlayhead(juce::Graphics& g, int step)
{
    if (step < 0 || step >= numSteps)
        return;

    // Berechne Playhead Position
    int headerWidth = trackLabelWidth;
    int buttonWidth = 0;

    if (stepButtons.size() > 0 && stepButtons[0].size() > step)
    {
        buttonWidth = stepButtons[0][step]->getWidth();
    }

    int x = headerWidth + step * (buttonWidth + stepGap);

    // Zeichne Playhead-Linie
    g.setColour(getNeonCyan().withAlpha(0.8f));
    g.drawLine((float)x, (float)headerHeight, (float)x, (float)(getHeight() - footerHeight));

    // Glow Effekt
    g.setColour(getNeonCyan().withAlpha(0.2f));
    g.fillRect((float)x - 2, (float)headerHeight, 4.0f, (float)(getHeight() - headerHeight - footerHeight));
}
