#include "TrackComponent.h"

// StepButton neon glow rendering
void StepButton::paintButton(juce::Graphics& g, bool, bool)
{
    // Create rounded rectangle path with margin for glow
    auto bounds = getLocalBounds().toFloat();
    juce::Path p;
    p.addRoundedRectangle(bounds.reduced(8.0f), 4.0f);

    // Get the current button color
    juce::Colour baseColor = findColour(juce::TextButton::buttonColourId);

    // Render neon glow if this is an active step
    if (isActiveStep)
    {
        melatonin::DropShadow glow(baseColor, 8, {0, 0});
        glow.render(g, p);
    }

    // Draw the actual button
    g.setColour(baseColor);
    g.fillPath(p);

    // Draw button text
    g.setColour(juce::Colours::white);
    g.setFont(10.0f);
    g.drawText(getButtonText(), getLocalBounds(), juce::Justification::centred, false);
}

TrackComponent::TrackComponent(int trackIndex_, juce::AudioFormatManager& formatManager_)
    : trackIndex(trackIndex_), audioProcessor(formatManager_, model)
{
    // Track label
    addAndMakeVisible(trackLabel);
    trackLabel.setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
    trackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    trackLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    // Expand/Collapse button
    addAndMakeVisible(expandButton);
    expandButton.setButtonText("-");
    expandButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    expandButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    expandButton.onClick = [this] {
        model.setIsExpanded(!model.getIsExpanded());
        expandButton.setButtonText(model.getIsExpanded() ? "-" : "+");
        if (onStateChange)
            onStateChange();
    };

    // Category dropdown
    addAndMakeVisible(categoryComboBox);
    categoryComboBox.setTextWhenNothingSelected("Select Sample...");
    categoryComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    categoryComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    categoryComboBox.setColour(juce::ComboBox::arrowColourId, getNeonPink());

    // Sample selection buttons
    addAndMakeVisible(prevSampleButton);
    prevSampleButton.setButtonText("<");
    prevSampleButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    prevSampleButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    prevSampleButton.onClick = [this] { changeSampleIndex(-1); };

    addAndMakeVisible(nextSampleButton);
    nextSampleButton.setButtonText(">");
    nextSampleButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    nextSampleButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    nextSampleButton.onClick = [this] { changeSampleIndex(1); };

    addAndMakeVisible(sampleIndexLabel);
    sampleIndexLabel.setText("0 / 0", juce::dontSendNotification);
    sampleIndexLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    sampleIndexLabel.setFont(juce::Font(10.0f));

    // Mute button
    addAndMakeVisible(muteButton);
    muteButton.setButtonText("M");
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    muteButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    muteButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    muteButton.onClick = [this] {
        audioProcessor.setMuted(muteButton.getToggleState());
    };

    // Solo button
    addAndMakeVisible(soloButton);
    soloButton.setButtonText("S");
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow);
    soloButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    soloButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    soloButton.onClick = [this] {
        audioProcessor.setSolo(soloButton.getToggleState());
    };

    // Clear button
    addAndMakeVisible(clearButton);
    clearButton.setButtonText("C");
    clearButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    clearButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    clearButton.onClick = [this] {
        clearAllSteps();
    };

    // Loop Length slider for polyrhythms
    addAndMakeVisible(loopLengthSlider);
    loopLengthSlider.setRange(1.0, 64.0, 1.0);
    loopLengthSlider.setValue(16.0);
    loopLengthSlider.setSliderStyle(juce::Slider::IncDecButtons);
    loopLengthSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, 20);
    loopLengthSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    loopLengthSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    loopLengthSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    loopLengthSlider.setColour(juce::Slider::textBoxBackgroundColourId, getDarkBackground());
    loopLengthSlider.onValueChange = [this] {
        model.setTrackLoopLength(static_cast<int>(loopLengthSlider.getValue()));
    };

    addAndMakeVisible(loopLengthLabel);
    loopLengthLabel.setText("Steps", juce::dontSendNotification);
    loopLengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    loopLengthLabel.setFont(juce::Font(10.0f));

    // Bank selector buttons
    const char bankNames[4] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < TrackModel::numBanks; ++i)
    {
        bankButtons[i] = std::make_unique<juce::TextButton>();
        bankButtons[i]->setButtonText(juce::String(bankNames[i]));
        bankButtons[i]->setClickingTogglesState(true);
        bankButtons[i]->setColour(juce::TextButton::buttonColourId, getStepInactive());
        bankButtons[i]->setColour(juce::TextButton::buttonOnColourId, getNeonCyan());
        bankButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        bankButtons[i]->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible(bankButtons[i].get());

        const int bankIndex = i;
        bankButtons[i]->onClick = [this, bankIndex] { setBank(bankIndex); };
    }

    // Set initial bank selection (Bank A)
    bankButtons[0]->setToggleState(true, juce::dontSendNotification);
    updateBankButtonStyles();

    addAndMakeVisible(bankLabel);
    bankLabel.setText("BANK", juce::dontSendNotification);
    bankLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    bankLabel.setFont(juce::Font(8.0f));

    // Control sliders (Volume, Pitch, Attack, Decay, Cutoff)
    const char* knobNames[5] = {"Vol", "Pit", "Atk", "Dec", "Cut"};
    const juce::Colour knobColors[5] = {getNeonPink(), getNeonCyan(), getNeonCyan(), getNeonPink(), getNeonPink()};
    const double knobRanges[5][3] = {{0.0, 1.0, 0.01}, {-12.0, 12.0, 1.0}, {0.0, 2.0, 0.01}, {0.01, 2.0, 0.01}, {20.0, 20000.0, 1.0}};
    const double knobDefaults[5] = {0.8, 0.0, 0.0, 0.5, 20000.0};

    for (int i = 0; i < 5; ++i)
    {
        controlSliders[i] = std::make_unique<juce::Slider>();
        controlSliders[i]->setRange(knobRanges[i][0], knobRanges[i][1], knobRanges[i][2]);
        controlSliders[i]->setValue(knobDefaults[i]);
        controlSliders[i]->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        controlSliders[i]->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        controlSliders[i]->setColour(juce::Slider::thumbColourId, knobColors[i]);
        controlSliders[i]->setColour(juce::Slider::rotarySliderFillColourId, knobColors[i].withAlpha(0.5f));
        controlSliders[i]->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        addAndMakeVisible(controlSliders[i].get());

        controlLabels[i] = std::make_unique<juce::Label>();
        controlLabels[i]->setText(knobNames[i], juce::dontSendNotification);
        controlLabels[i]->setColour(juce::Label::textColourId, juce::Colours::white);
        controlLabels[i]->setFont(juce::Font(10.0f));
        controlLabels[i]->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(controlLabels[i].get());
    }

    // Cutoff needs logarithmic skew
    controlSliders[4]->setSkewFactorFromMidPoint(1000.0);

    // Slider callbacks
    controlSliders[0]->onValueChange = [this] {
        audioProcessor.setVolume(static_cast<float>(controlSliders[0]->getValue()));
    };
    controlSliders[1]->onValueChange = [this] {
        audioProcessor.setPitch(static_cast<int>(controlSliders[1]->getValue()));
    };
    controlSliders[2]->onValueChange = [this] {
        audioProcessor.setAttack(static_cast<float>(controlSliders[2]->getValue()));
    };
    controlSliders[3]->onValueChange = [this] {
        audioProcessor.setDecay(static_cast<float>(controlSliders[3]->getValue()));
    };
    controlSliders[4]->onValueChange = [this] {
        audioProcessor.setCutoff(static_cast<float>(controlSliders[4]->getValue()));
    };

    // Step buttons (64 visible at once)
    for (int i = 0; i < TrackModel::totalSteps; ++i)
    {
        stepButtons[i] = std::make_unique<StepButton>();
        stepButtons[i]->setButtonText(juce::String(i + 1));
        stepButtons[i]->setClickingTogglesState(true);
        stepButtons[i]->setColour(juce::TextButton::buttonColourId, getStepInactive());
        stepButtons[i]->setColour(juce::TextButton::buttonOnColourId, getNeonPink());
        stepButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        stepButtons[i]->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible(stepButtons[i].get());
    }

    // Add step button click handlers
    for (int i = 0; i < TrackModel::totalSteps; ++i)
    {
        const int globalStepIndex = i;
        const int bankIndex = i / TrackModel::stepsPerBank;
        const int stepInBank = i % TrackModel::stepsPerBank;

        // Left-click: Toggle step
        stepButtons[i]->onClick = [this, globalStepIndex, bankIndex, stepInBank] {
            StepModifierState state = model.getStepState(globalStepIndex);
            state.active = !state.active;
            model.setStepState(globalStepIndex, state);
            updateStepButtonState(globalStepIndex, state);
        };

        // Right-click: Context menu
        const int buttonIdx = i;
        stepButtons[i]->onRightClick = [this, globalStepIndex, bankIndex, stepInBank, buttonIdx] {
            juce::PopupMenu subSpeed, subSlow, subElongate, subReplicate, subProbability, subPitch, subVol;

            // Speed submenu (*)
            subSpeed.addItem(1, "2x");
            subSpeed.addItem(2, "3x");
            subSpeed.addItem(3, "4x");

            // Slow submenu (/)
            subSlow.addItem(10, "/2");
            subSlow.addItem(11, "/3");
            subSlow.addItem(12, "/4");

            // Elongate submenu (@)
            subElongate.addItem(20, "@2");
            subElongate.addItem(21, "@3");
            subElongate.addItem(22, "@4");

            // Replicate submenu (!)
            subReplicate.addItem(30, "!2");
            subReplicate.addItem(31, "!3");
            subReplicate.addItem(32, "!4");

            // Probability submenu (?)
            subProbability.addItem(40, "?25%");
            subProbability.addItem(41, "?50%");
            subProbability.addItem(42, "?75%");

            // P-Lock Pitch submenu
            for (int p = -12; p <= 12; ++p)
            {
                subPitch.addItem(112 + p, (p > 0 ? "+" : "") + juce::String(p) + " st");
            }

            // P-Lock Volume submenu
            subVol.addItem(200, "0%");
            subVol.addItem(201, "20%");
            subVol.addItem(202, "40%");
            subVol.addItem(203, "60%");
            subVol.addItem(204, "80%");
            subVol.addItem(205, "100%");

            // Main popup menu
            juce::PopupMenu m;
            m.addItem(99, "Clear / Normal");
            m.addSeparator();
            m.addSubMenu("Speed (*)", subSpeed);
            m.addSubMenu("Slow (/)", subSlow);
            m.addSubMenu("Elongate (@)", subElongate);
            m.addSubMenu("Replicate (!)", subReplicate);
            m.addSubMenu("Probability (?)", subProbability);
            m.addSeparator();
            m.addSubMenu("Lock Pitch", subPitch);
            m.addSubMenu("Lock Volume", subVol);
            m.addItem(98, "Clear P-Locks");

            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(stepButtons[buttonIdx].get()),
                [this, globalStepIndex, bankIndex, stepInBank](int result)
                {
                    if (result == 0) return;

                    StepModifierState state = model.getStepState(globalStepIndex);

                    if (result == 99)
                    {
                        state = StepModifierState{true, ' ', 1};
                    }
                    else if (result == 98)
                    {
                        state.hasPitchLock = false;
                        state.hasVolLock = false;
                    }
                    else if (result >= 1 && result <= 3)
                    {
                        int value = result + 1;
                        state = StepModifierState{true, '*', value};
                    }
                    else if (result >= 10 && result <= 12)
                    {
                        int value = result - 8;
                        state = StepModifierState{true, '/', value};
                    }
                    else if (result >= 20 && result <= 22)
                    {
                        int value = result - 18;
                        state = StepModifierState{true, '@', value};
                    }
                    else if (result >= 30 && result <= 32)
                    {
                        int value = result - 28;
                        state = StepModifierState{true, '!', value};
                    }
                    else if (result >= 40 && result <= 42)
                    {
                        int value = 25 + (result - 40) * 25;
                        state = StepModifierState{true, '?', value};
                    }
                    else if (result >= 100 && result <= 124)
                    {
                        state.active = true;
                        state.hasPitchLock = true;
                        state.pitchLock = result - 112;
                    }
                    else if (result >= 200 && result <= 205)
                    {
                        state.active = true;
                        state.hasVolLock = true;
                        state.volLock = (result - 200) * 0.2f;
                    }

                    model.setStepState(globalStepIndex, state);
                    updateStepButtonState(globalStepIndex, state);
                });
        };
    }
}

TrackComponent::~TrackComponent()
{
}

void TrackComponent::setSampleCategories(const juce::StringArray& categories)
{
    categoryComboBox.clear();
    categoryComboBox.addItemList(categories, 1);
}

void TrackComponent::setSelectedCategory(const juce::String& category)
{
    categoryComboBox.setText(category);
}

juce::String TrackComponent::getSelectedCategory() const
{
    return categoryComboBox.getText();
}

void TrackComponent::setStepActive(int step, bool active)
{
    model.setStepActive(step, active);
    updateStepButtonState(step, model.getStepState(step));
}

bool TrackComponent::isStepActive(int step) const
{
    return model.isStepActive(step);
}

StepModifierState TrackComponent::getStepState(int step) const
{
    return model.getStepState(step);
}

void TrackComponent::setStepState(int step, const StepModifierState& state)
{
    model.setStepState(step, state);
    updateStepButtonState(step, state);
}

void TrackComponent::setStepModifier(int step, char modifierType, int modifierValue)
{
    model.setStepModifier(step, modifierType, modifierValue);
    updateStepButtonState(step, model.getStepState(step));
}

int TrackComponent::getModifierValue(int step) const
{
    return model.getModifierValue(step);
}

char TrackComponent::getModifierType(int step) const
{
    return model.getModifierType(step);
}

void TrackComponent::setBank(int bank)
{
    model.setBank(bank);

    // Update all bank buttons
    for (int i = 0; i < TrackModel::numBanks; ++i)
    {
        bankButtons[i]->setToggleState(i == bank, juce::dontSendNotification);
    }

    updateBankButtonStyles();
    refreshStepButtons();
}

void TrackComponent::refreshStepButtons()
{
    for (int i = 0; i < TrackModel::totalSteps; ++i)
    {
        updateStepButtonState(i, model.getStepState(i));
    }
}

void TrackComponent::updateBankButtonStyles()
{
    const int currentBank = model.getBank();
    for (int i = 0; i < TrackModel::numBanks; ++i)
    {
        if (i == currentBank)
        {
            bankButtons[i]->setColour(juce::TextButton::buttonColourId, getNeonCyan());
            bankButtons[i]->setColour(juce::TextButton::buttonOnColourId, getNeonCyan().withAlpha(0.3f));
        }
        else
        {
            bankButtons[i]->setColour(juce::TextButton::buttonColourId, getStepInactive());
            bankButtons[i]->setColour(juce::TextButton::buttonOnColourId, getNeonPink());
        }
    }
}

void TrackComponent::updateStepButtonState(int globalStep, const StepModifierState& state)
{
    if (globalStep >= 0 && globalStep < TrackModel::totalSteps && stepButtons[globalStep].get())
    {
        const bool isActive = state.active;
        stepButtons[globalStep]->setToggleState(isActive, juce::dontSendNotification);
        stepButtons[globalStep]->isActiveStep = isActive;

        if (isActive)
        {
            stepButtons[globalStep]->setColour(juce::TextButton::buttonColourId, getNeonPink());
            juce::String displayText = state.getDisplayText();
            if (displayText.isNotEmpty())
            {
                stepButtons[globalStep]->setButtonText(displayText);
            }
            else
            {
                stepButtons[globalStep]->setButtonText(juce::String(globalStep + 1));
            }
        }
        else
        {
            stepButtons[globalStep]->setColour(juce::TextButton::buttonColourId, getStepInactive());
            stepButtons[globalStep]->setButtonText(juce::String(globalStep + 1));
        }
    }
}

void TrackComponent::loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory)
{
    audioProcessor.loadSampleForCategory(category, sampleDirectory);
    updateSampleIndexLabel();
}

void TrackComponent::resized()
{
    isResizing = true;

    auto area = getLocalBounds();

    // Top row
    auto headerArea = area.removeFromTop(40);

    // Expand button first
    expandButton.setBounds(headerArea.removeFromLeft(25).reduced(2, 8));
    trackLabel.setBounds(headerArea.removeFromLeft(70));

    // Mute/Solo/Clear buttons
    const int controlBtnWidth = 22;
    auto leftControls = headerArea.removeFromLeft(80);
    muteButton.setBounds(leftControls.removeFromLeft(controlBtnWidth).reduced(1, 8));
    soloButton.setBounds(leftControls.removeFromLeft(controlBtnWidth).reduced(1, 8));
    clearButton.setBounds(leftControls.removeFromLeft(controlBtnWidth).reduced(1, 8));

    // Loop length slider
    auto loopArea = headerArea.removeFromLeft(100);
    loopLengthLabel.setBounds(loopArea.removeFromLeft(30).reduced(0, 12));
    loopLengthSlider.setBounds(loopArea.reduced(0, 8));

    // Calculate required width
    const int sampleControlsWidth = 130;
    const int bankButtonsWidth = 120;
    const int rightControlsWidth = sampleControlsWidth + bankButtonsWidth + 10;

    auto rightArea = headerArea.removeFromRight(rightControlsWidth);

    // Bank selector buttons
    auto bankArea = rightArea.removeFromRight(bankButtonsWidth);
    const int bankButtonSize = 22;
    bankLabel.setBounds(bankArea.removeFromLeft(25).reduced(2, 10));
    for (int i = 0; i < TrackModel::numBanks; ++i)
    {
        bankButtons[i]->setBounds(bankArea.removeFromLeft(bankButtonSize).reduced(1, 5));
    }

    // Sample selection buttons
    auto sampleControlsArea = rightArea;
    prevSampleButton.setBounds(sampleControlsArea.removeFromLeft(22).reduced(2, 8));
    sampleIndexLabel.setBounds(sampleControlsArea.removeFromLeft(55).reduced(3, 10));
    nextSampleButton.setBounds(sampleControlsArea.removeFromLeft(22).reduced(2, 8));

    // Category dropdown
    categoryComboBox.setBounds(headerArea.reduced(5, 5));

    // CRITICAL: If collapsed, stop here
    if (!model.getIsExpanded())
    {
        isResizing = false;
        return;
    }

    // Second row: control knobs
    auto controlArea = area.removeFromTop(50);
    const int knobSize = 38;
    const int knobSpacing = 8;
    const int totalKnobsWidth = (knobSize * 5) + (knobSpacing * 4);
    auto knobRow = controlArea.withTrimmedLeft((controlArea.getWidth() - totalKnobsWidth) / 2).withWidth(totalKnobsWidth);

    for (int i = 0; i < 5; ++i)
    {
        controlSliders[i]->setBounds(knobRow.removeFromLeft(knobSize));
        controlLabels[i]->setBounds(controlSliders[i]->getBounds().withTrimmedBottom(5).withTrimmedTop(23));
        if (i < 4) knobRow.removeFromLeft(knobSpacing);
    }

    // Step buttons - 2 rows of 32 steps
    auto stepRow1 = area.removeFromTop(35);
    auto stepRow2 = area.removeFromTop(35);
    const int buttonsPerRow = 32;
    const int buttonWidth1 = stepRow1.getWidth() / buttonsPerRow;
    const int buttonWidth2 = stepRow2.getWidth() / buttonsPerRow;
    const int row1X = stepRow1.getX();
    const int row1Y = stepRow1.getY();
    const int row1H = stepRow1.getHeight();
    const int row2X = stepRow2.getX();
    const int row2Y = stepRow2.getY();
    const int row2H = stepRow2.getHeight();

    // First row: steps 1-32
    for (int i = 0; i < buttonsPerRow; ++i)
    {
        stepButtons[i]->setBounds(row1X + i * buttonWidth1, row1Y, buttonWidth1 - 1, row1H);
    }

    // Second row: steps 33-64
    for (int i = 0; i < buttonsPerRow; ++i)
    {
        stepButtons[i + buttonsPerRow]->setBounds(row2X + i * buttonWidth2, row2Y, buttonWidth2 - 1, row2H);
    }

    isResizing = false;
}

void TrackComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);
}

void TrackComponent::updatePlayhead(int currentStep, bool isPlaying)
{
    if (isResizing)
        return;

    if (currentStep == lastPlayedStep && isPlaying)
        return;

    // Restore previous step's color
    if (lastPlayedStep >= 0 && lastPlayedStep < TrackModel::totalSteps)
    {
        const bool wasActive = stepButtons[lastPlayedStep]->getToggleState();
        stepButtons[lastPlayedStep]->setColour(juce::TextButton::buttonColourId,
            wasActive ? getNeonPink() : getStepInactive());
        stepButtons[lastPlayedStep]->isActiveStep = wasActive;
        stepButtons[lastPlayedStep]->repaint();
    }

    // Highlight new current step
    if (currentStep >= 0 && currentStep < TrackModel::totalSteps)
    {
        const bool isActive = stepButtons[currentStep]->getToggleState();
        stepButtons[currentStep]->setColour(juce::TextButton::buttonColourId,
            isActive ? getNeonPink() : getNeonCyan());
        stepButtons[currentStep]->isActiveStep = true;
        stepButtons[currentStep]->repaint();
    }

    lastPlayedStep = currentStep;
}

void TrackComponent::changeSampleIndex(int delta)
{
    if (model.getNumSamples() == 0)
        return;

    int newIndex = model.getCurrentSampleIndex() + delta;
    if (newIndex < 0)
        newIndex = model.getNumSamples() - 1;
    else if (newIndex >= model.getNumSamples())
        newIndex = 0;

    loadSampleAtIndex(newIndex);
}

void TrackComponent::loadSampleAtIndex(int index)
{
    audioProcessor.loadSampleAtIndex(index);
    updateSampleIndexLabel();
}

void TrackComponent::updateSampleIndexLabel()
{
    if (model.getNumSamples() == 0)
        sampleIndexLabel.setText("0 / 0", juce::dontSendNotification);
    else
        sampleIndexLabel.setText(juce::String(model.getCurrentSampleIndex() + 1) + " / " +
            juce::String(model.getNumSamples()), juce::dontSendNotification);
}

void TrackComponent::prepareAudio(double sampleRate, int samplesPerBlock)
{
    audioProcessor.prepareAudio(sampleRate, samplesPerBlock);
}

void TrackComponent::processAudioBlock(juce::AudioBuffer<float>& buffer)
{
    audioProcessor.processAudioBlock(buffer);
}

void TrackComponent::setMuted(bool muted)
{
    audioProcessor.setMuted(muted);
    muteButton.setToggleState(muted, juce::dontSendNotification);
}

void TrackComponent::setSolo(bool solo)
{
    audioProcessor.setSolo(solo);
    soloButton.setToggleState(solo, juce::dontSendNotification);
}

void TrackComponent::clearAllSteps()
{
    model.clearAllSteps();
    refreshStepButtons();
}
