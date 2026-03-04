// ============================================================================
// TrackViewComponent.cpp - Track-basierte View Implementierung
// ============================================================================

#include "TrackViewComponent.h"
#include "UnifiedSequencerModel.h"

// ============================================================================
// TrackRowComponent Implementierung
// ============================================================================

TrackRowComponent::TrackRowComponent(int trackIndex)
    : trackIndex(trackIndex)
{
    // Track Label
    trackLabel.setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
    trackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    trackLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(trackLabel);

    // Track Controls Sliders
    setupSlider(volumeSlider, 0.0, 1.0, 0.8, "Vol");
    setupSlider(pitchSlider, -24, 24, 0, "Pitch");
    setupSlider(attackSlider, 0.0, 1.0, 0.01, "Atk");
    setupSlider(decaySlider, 0.0, 2.0, 0.1, "Dec");

    // Create Step Buttons
    createStepButtons();
}

void TrackRowComponent::createStepButtons()
{
    for (int i = 0; i < 64; ++i)
    {
        auto button = std::make_unique<juce::TextButton>();
        button->setButtonText("");
        button->setClickingTogglesState(false);
        button->setColour(juce::TextButton::buttonColourId, getStepInactive());
        button->setColour(juce::TextButton::buttonOnColourId, getNeonPink());
        button->setLookAndFeel(nullptr);

        // Setze finalen Step-Index
        const int stepIndex = i;

        button->onClick = [this, stepIndex]()
        {
            if (onStepClicked)
                onStepClicked(stepIndex, !stepButtons[stepIndex]->getToggleState());
        };

        button->setMouseClickGrabsKeyboardFocus(false);
        addAndMakeVisible(button.get());
        stepButtons[i] = std::move(button);
    }
}

void TrackRowComponent::setupSlider(juce::Slider& slider, double min, double max, double initial, const juce::String& labelText)
{
    slider.setRange(min, max);
    slider.setValue(initial);
    slider.setSliderStyle(juce::Slider::LinearBar);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    slider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    addAndMakeVisible(slider);
}

void TrackRowComponent::setModel(UnifiedSequencerModel* model)
{
    this->model = model;
}

void TrackRowComponent::setStepActive(int step, bool active)
{
    jassert(step >= 0 && step < 64);
    stepButtons[step]->setToggleState(active, juce::dontSendNotification);
    updateStepButtonState(step);
}

void TrackRowComponent::setStepState(int step, const StepModifierState& state)
{
    jassert(step >= 0 && step < 64);
    stepButtons[step]->setToggleState(state.active, juce::dontSendNotification);
    stepButtons[step]->setButtonText(state.getDisplayText());
    updateStepButtonState(step);
}

void TrackRowComponent::updateStepButtonState(int step)
{
    jassert(step >= 0 && step < 64);

    if (stepButtons[step]->getToggleState())
    {
        stepButtons[step]->setColour(juce::TextButton::buttonColourId, getNeonPink());
        stepButtons[step]->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }
    else
    {
        stepButtons[step]->setColour(juce::TextButton::buttonColourId, getStepInactive());
        stepButtons[step]->setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    }
}

void TrackRowComponent::resized()
{
    auto bounds = getLocalBounds();

    // Track Label (linke Seite)
    auto labelBounds = bounds.removeFromLeft(80);
    trackLabel.setBounds(labelBounds.reduced(4));

    // Track Controls (nach dem Label)
    auto controlsBounds = bounds.removeFromLeft(120);

    int controlHeight = bounds.getHeight() / 4;
    int y = 0;

    auto volBounds = controlsBounds.removeFromTop(controlHeight).reduced(2);
    volumeSlider.setBounds(volBounds);

    auto pitchBounds = controlsBounds.removeFromTop(controlHeight).reduced(2);
    pitchSlider.setBounds(pitchBounds);

    auto atkBounds = controlsBounds.removeFromTop(controlHeight).reduced(2);
    attackSlider.setBounds(atkBounds);

    auto decBounds = controlsBounds.removeFromTop(controlHeight).reduced(2);
    decaySlider.setBounds(decBounds);

    // Step Buttons Grid
    int numCols = 16;
    int numRows = 4;
    int buttonWidth = bounds.getWidth() / numCols;
    int buttonHeight = bounds.getHeight() / numRows;

    for (int row = 0; row < numRows; ++row)
    {
        for (int col = 0; col < numCols; ++col)
        {
            int index = row * numCols + col;
            if (index < 64)
            {
                auto buttonBounds = juce::Rectangle<int>(
                    col * buttonWidth,
                    row * buttonHeight,
                    buttonWidth,
                    buttonHeight
                ).reduced(2);
                stepButtons[index]->setBounds(buttonBounds);
            }
        }
    }
}

void TrackRowComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());
}

// ============================================================================
// TrackViewComponent Implementierung
// ============================================================================

TrackViewComponent::TrackViewComponent()
{
    setupHeader();
    createTrackRows();
}

void TrackViewComponent::setupHeader()
{
    headerLabel.setText("Track View - Step Sequencer", juce::dontSendNotification);
    headerLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    headerLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    headerLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(headerLabel);
}

void TrackViewComponent::createTrackRows()
{
    for (int i = 0; i < 8; ++i)
    {
        auto row = std::make_unique<TrackRowComponent>(i);

        // Finaler Index für Lambda-Capture
        const int trackIdx = i;

        row->onStepClicked = [this, trackIdx](int step, bool active)
        {
            if (model)
            {
                model->setStepActive(trackIdx, step, active);
            }
        };

        row->onStepRightClicked = [this, trackIdx](int step)
        {
            // Rechter Klick könnte Modifier-Menü öffnen
            // TODO: Implementiere Modifier-Popup
        };

        addAndMakeVisible(row.get());
        trackRows[i] = std::move(row);
    }
}

void TrackViewComponent::setModel(UnifiedSequencerModel* model)
{
    this->model = model;

    // Aktualisiere alle Rows mit dem neuen Model
    for (int i = 0; i < 8; ++i)
    {
        if (trackRows[i])
        {
            trackRows[i]->setModel(model);
        }
    }
}

void TrackViewComponent::setSampleManager(SampleManager* manager)
{
    // SampleManager für Samples laden
    // TODO: Implementiere Sample-Auswahl
}

void TrackViewComponent::resized()
{
    auto bounds = getLocalBounds();

    // Header oben
    auto headerBounds = bounds.removeFromTop(40);
    headerLabel.setBounds(headerBounds);

    // Track Rows
    int rowHeight = bounds.getHeight() / 8;
    for (int i = 0; i < 8; ++i)
    {
        if (trackRows[i])
        {
            trackRows[i]->setBounds(bounds.removeFromTop(rowHeight));
        }
    }
}

void TrackViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(30, 30, 40));

    // Subtiler Rahmen um die Rows
    auto bounds = getLocalBounds().withTrimmedTop(40);
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(bounds, 1);
}
