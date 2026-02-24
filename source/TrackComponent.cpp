#include "TrackComponent.h"

// Custom SamplerVoice that properly wraps JUCE's SamplerSound
class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    CustomSamplerVoice() = default;
};

TrackComponent::TrackComponent(int trackIndex_, juce::AudioFormatManager& formatManager_)
    : trackIndex(trackIndex_), formatManager(formatManager_)
{
    // Track label
    addAndMakeVisible(trackLabel);
    trackLabel.setText("Track " + juce::String(trackIndex + 1), juce::dontSendNotification);
    trackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    trackLabel.setFont(juce::Font(14.0f, juce::Font::bold));

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

    // Bank selector buttons
    const char bankNames[4] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < numBanks; ++i)
    {
        bankButtons[i] = std::make_unique<juce::TextButton>();
        bankButtons[i]->setButtonText(juce::String(bankNames[i]));
        bankButtons[i]->setClickingTogglesState(true);
        bankButtons[i]->setColour(juce::TextButton::buttonColourId, getStepInactive());
        bankButtons[i]->setColour(juce::TextButton::buttonOnColourId, getNeonCyan());
        bankButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        bankButtons[i]->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible(bankButtons[i].get());

        // Set bank button click handler
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

    // Volume knob
    addAndMakeVisible(volumeSlider);
    volumeSlider.setRange(0.0, 1.0, 0.01);
    volumeSlider.setValue(0.8);
    volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    volumeSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPink().withAlpha(0.5f));
    volumeSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    volumeSlider.onValueChange = [this] {
        volume.store(static_cast<float>(volumeSlider.getValue()));
    };

    addAndMakeVisible(volumeLabel);
    volumeLabel.setText("Vol", juce::dontSendNotification);
    volumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    volumeLabel.setFont(juce::Font(10.0f));
    volumeLabel.setJustificationType(juce::Justification::centred);

    // Pitch knob
    addAndMakeVisible(pitchSlider);
    pitchSlider.setRange(-12.0, 12.0, 1.0);
    pitchSlider.setValue(0.0);
    pitchSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    pitchSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    pitchSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonCyan().withAlpha(0.5f));
    pitchSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    pitchSlider.onValueChange = [this] {
        pitch.store(static_cast<int>(pitchSlider.getValue()));
    };

    addAndMakeVisible(pitchLabel);
    pitchLabel.setText("Pit", juce::dontSendNotification);
    pitchLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    pitchLabel.setFont(juce::Font(10.0f));
    pitchLabel.setJustificationType(juce::Justification::centred);

    // Attack knob
    addAndMakeVisible(attackSlider);
    attackSlider.setRange(0.0, 2.0, 0.01);
    attackSlider.setValue(0.0);
    attackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    attackSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    attackSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    attackSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonCyan().withAlpha(0.5f));
    attackSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    attackSlider.onValueChange = [this] {
        attack.store(static_cast<float>(attackSlider.getValue()));
        updateDecayEnvelope();
    };

    addAndMakeVisible(attackLabel);
    attackLabel.setText("Atk", juce::dontSendNotification);
    attackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    attackLabel.setFont(juce::Font(10.0f));
    attackLabel.setJustificationType(juce::Justification::centred);

    // Decay knob
    addAndMakeVisible(decaySlider);
    decaySlider.setRange(0.01, 2.0, 0.01);
    decaySlider.setValue(0.5);
    decaySlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    decaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    decaySlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    decaySlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPink().withAlpha(0.5f));
    decaySlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    decaySlider.onValueChange = [this] {
        decay.store(static_cast<float>(decaySlider.getValue()));
        updateDecayEnvelope();
    };

    addAndMakeVisible(decayLabel);
    decayLabel.setText("Dec", juce::dontSendNotification);
    decayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    decayLabel.setFont(juce::Font(10.0f));
    decayLabel.setJustificationType(juce::Justification::centred);

    // Cutoff knob
    addAndMakeVisible(cutoffSlider);
    cutoffSlider.setRange(20.0, 20000.0, 1.0);
    cutoffSlider.setValue(20000.0);
    cutoffSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    cutoffSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);  // Logarithmic feel
    cutoffSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    cutoffSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPink().withAlpha(0.5f));
    cutoffSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    cutoffSlider.onValueChange = [this] {
        cutoff.store(static_cast<float>(cutoffSlider.getValue()));
    };

    addAndMakeVisible(cutoffLabel);
    cutoffLabel.setText("Cut", juce::dontSendNotification);
    cutoffLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    cutoffLabel.setFont(juce::Font(10.0f));
    cutoffLabel.setJustificationType(juce::Justification::centred);

    // Step buttons (16 visible, showing current bank)
    for (int i = 0; i < stepsPerBank; ++i)
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

    // Initialize all bank steps to Off state
    for (int bank = 0; bank < numBanks; ++bank)
    {
        for (int step = 0; step < stepsPerBank; ++step)
        {
            bankSteps[bank][step] = StepModifierState{false, ' ', 1};
        }
    }

    // Add step button click handlers (toggle steps in current bank)
    for (int i = 0; i < stepsPerBank; ++i)
    {
        const int stepIndex = i;
        const int bankIndex = currentBank;

        // Left-click: Toggle between Off (active=false) and Normal (active=true, modifier=' ')
        stepButtons[i]->onClick = [this, stepIndex] {
            bool newState = !bankSteps[currentBank][stepIndex].active;
            bankSteps[currentBank][stepIndex].active = newState;
            updateStepButtonState(stepIndex, bankSteps[currentBank][stepIndex]);
        };

        // Right-click: Open context menu with modifier submenus
        const int buttonIdx = i;  // Capture i for lambda
        stepButtons[i]->onRightClick = [this, stepIndex, bankIndex, buttonIdx] {
            // Create submenus for each modifier type
            juce::PopupMenu subSpeed, subSlow, subElongate, subReplicate;

            // Speed submenu (*) - use simple IDs (1=2x, 2=3x, 3=4x)
            subSpeed.addItem(1, "2x");
            subSpeed.addItem(2, "3x");
            subSpeed.addItem(3, "4x");

            // Slow submenu (/) - base ID 10 (10=/2, 11=/3, 12=/4)
            subSlow.addItem(10, "/2");
            subSlow.addItem(11, "/3");
            subSlow.addItem(12, "/4");

            // Elongate submenu (@) - base ID 20 (20=@2, 21=@3, 22=@4)
            subElongate.addItem(20, "@2");
            subElongate.addItem(21, "@3");
            subElongate.addItem(22, "@4");

            // Replicate submenu (!) - base ID 30 (30=!2, 31=!3, 32=!4)
            subReplicate.addItem(30, "!2");
            subReplicate.addItem(31, "!3");
            subReplicate.addItem(32, "!4");

            // Main popup menu
            juce::PopupMenu m;
            m.addItem(0, "Clear / Normal");
            m.addSubMenu("Speed (*)", subSpeed);
            m.addSubMenu("Slow (/)", subSlow);
            m.addSubMenu("Elongate (@)", subElongate);
            m.addSubMenu("Replicate (!)", subReplicate);

            m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(stepButtons[buttonIdx].get()),
                [this, stepIndex, bankIndex](int result)
                {
                    if (result == 0)
                    {
                        // Clear / Normal
                        bankSteps[currentBank][stepIndex] = StepModifierState{true, ' ', 1};
                    }
                    else if (result >= 1 && result <= 3)
                    {
                        // Speed (*) - result 1=2x, 2=3x, 3=4x
                        int value = result + 1;  // Convert 0-index to 2,3,4
                        bankSteps[currentBank][stepIndex] = StepModifierState{true, '*', value};
                    }
                    else if (result >= 10 && result <= 12)
                    {
                        // Slow (/) - result 10=/2, 11=/3, 12=/4
                        int value = result - 8;  // Convert to 2,3,4
                        bankSteps[currentBank][stepIndex] = StepModifierState{true, '/', value};
                    }
                    else if (result >= 20 && result <= 22)
                    {
                        // Elongate (@) - result 20=@2, 21=@3, 22=@4
                        int value = result - 18;  // Convert to 2,3,4
                        bankSteps[currentBank][stepIndex] = StepModifierState{true, '@', value};
                    }
                    else if (result >= 30 && result <= 32)
                    {
                        // Replicate (!) - result 30=!2, 31=!3, 32=!4
                        int value = result - 28;  // Convert to 2,3,4
                        bankSteps[currentBank][stepIndex] = StepModifierState{true, '!', value};
                    }
                    updateStepButtonState(stepIndex, bankSteps[currentBank][stepIndex]);
                });
        };
    }

    // Add more sampler voices to prevent voice stealing during rapid triggers
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new CustomSamplerVoice());
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
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    bankSteps[bank][bankStep].active = active;
    if (!active)
    {
        bankSteps[bank][bankStep].modifierType = ' ';
        bankSteps[bank][bankStep].modifierValue = 1;
    }
    updateStepButtonState(bankStep, bankSteps[bank][bankStep]);
}

bool TrackComponent::isStepActive(int step) const
{
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep].active;
}

StepModifierState TrackComponent::getStepState(int step) const
{
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep];
}

void TrackComponent::setStepState(int step, const StepModifierState& state)
{
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    bankSteps[bank][bankStep] = state;
    updateStepButtonState(bankStep, state);
}

void TrackComponent::setStepModifier(int step, char modifierType, int modifierValue)
{
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    bankSteps[bank][bankStep].active = true;
    bankSteps[bank][bankStep].modifierType = modifierType;
    bankSteps[bank][bankStep].modifierValue = modifierValue;
    updateStepButtonState(bankStep, bankSteps[bank][bankStep]);
}

int TrackComponent::getModifierValue(int step) const
{
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep].modifierValue;
}

char TrackComponent::getModifierType(int step) const
{
    jassert(step >= 0 && step < getTotalSteps());
    const int bank = step / stepsPerBank;
    const int bankStep = step % stepsPerBank;
    return bankSteps[bank][bankStep].modifierType;
}

void TrackComponent::setBank(int bank)
{
    jassert(bank >= 0 && bank < numBanks);
    currentBank = bank;

    // Update all bank buttons
    for (int i = 0; i < numBanks; ++i)
    {
        bankButtons[i]->setToggleState(i == bank, juce::dontSendNotification);
    }

    // Update bank button styles
    updateBankButtonStyles();

    // Update visible step buttons to show new bank
    refreshStepButtons();
}

void TrackComponent::refreshStepButtons()
{
    for (int i = 0; i < stepsPerBank; ++i)
    {
        updateStepButtonState(i, bankSteps[currentBank][i]);
    }
}

void TrackComponent::updateBankButtonStyles()
{
    for (int i = 0; i < numBanks; ++i)
    {
        if (i == currentBank)
        {
            bankButtons[i]->setColour(juce::TextButton::buttonColourId, this->getNeonCyan());
            bankButtons[i]->setColour(juce::TextButton::buttonOnColourId, this->getNeonCyan().withAlpha(0.3f));
        }
        else
        {
            bankButtons[i]->setColour(juce::TextButton::buttonColourId, this->getStepInactive());
            bankButtons[i]->setColour(juce::TextButton::buttonOnColourId, this->getNeonPink());
        }
    }
}

void TrackComponent::updateStepButtonState(int bankStep, const StepModifierState& state)
{
    if (bankStep >= 0 && bankStep < stepsPerBank && stepButtons[bankStep].get())
    {
        const bool isActive = state.active;
        stepButtons[bankStep]->setToggleState(isActive, juce::dontSendNotification);

        if (isActive)
        {
            stepButtons[bankStep]->setColour(juce::TextButton::buttonColourId, this->getNeonPink());
            // Show modifier and value if not normal
            juce::String displayText = state.getDisplayText();
            if (displayText.isNotEmpty())
            {
                stepButtons[bankStep]->setButtonText(displayText);
            }
            else
            {
                stepButtons[bankStep]->setButtonText(juce::String(bankStep + 1));
            }
        }
        else
        {
            stepButtons[bankStep]->setColour(juce::TextButton::buttonColourId, this->getStepInactive());
            stepButtons[bankStep]->setButtonText(juce::String(bankStep + 1));
        }
    }
}

void TrackComponent::loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory)
{
    juce::File categoryDir = sampleDirectory.getChildFile(category);

    if (!categoryDir.exists())
        return;

    // Find all .wav files in directory and store them
    currentSampleFiles.clear();
    categoryDir.findChildFiles(currentSampleFiles, juce::File::findFiles, false, "*.wav");

    if (currentSampleFiles.isEmpty())
        return;

    // Reset index to 0 and update label
    currentSampleIndex = 0;
    updateSampleIndexLabel();

    juce::File sampleFile = currentSampleFiles[currentSampleIndex];
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));

    if (!reader)
        return;

    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);

    // Remove old sounds and add new one with generous ADSR envelope
    juce::ScopedLock lock(synthLock);
    synth.clearSounds();

    // Create SamplerSound with generous ADSR envelope for full sample playback
    // Attack knob value, Decay knob value, Sustain=1.0 (implicit), Release=1.0
    const float attackTime = attack.load();
    const float decayTime = decay.load();
    synth.addSound(new juce::SamplerSound(category, *reader, allNotes, 60, attackTime, decayTime, 1.0));
}

void TrackComponent::resized()
{
    auto area = getLocalBounds();

    // Top row: label, category selector, sample selection, and bank selector
    auto headerArea = area.removeFromTop(40);

    trackLabel.setBounds(headerArea.removeFromLeft(80));

    // Calculate required width for sample controls + bank buttons
    const int sampleControlsWidth = 130;  // < 2/2 > + space
    const int bankButtonsWidth = 120;      // A B C D buttons + label + space
    const int rightControlsWidth = sampleControlsWidth + bankButtonsWidth + 10;

    auto rightArea = headerArea.removeFromRight(rightControlsWidth);

    // Bank selector buttons (right side)
    auto bankArea = rightArea.removeFromRight(bankButtonsWidth);
    const int bankButtonSize = 22;
    bankLabel.setBounds(bankArea.removeFromLeft(25).reduced(2, 10));
    for (int i = 0; i < numBanks; ++i)
    {
        bankButtons[i]->setBounds(bankArea.removeFromLeft(bankButtonSize).reduced(1, 5));
    }

    // Sample selection buttons
    auto sampleControlsArea = rightArea;
    prevSampleButton.setBounds(sampleControlsArea.removeFromLeft(22).reduced(2, 8));
    sampleIndexLabel.setBounds(sampleControlsArea.removeFromLeft(55).reduced(3, 10));
    nextSampleButton.setBounds(sampleControlsArea.removeFromLeft(22).reduced(2, 8));

    // Category dropdown fills remaining space
    categoryComboBox.setBounds(headerArea.reduced(5, 5));

    // Second row: control knobs (5 knobs: Volume, Pitch, Attack, Decay, Cutoff)
    auto controlArea = area.removeFromTop(50);
    const int knobSize = 38;
    const int knobSpacing = 8;
    const int totalKnobsWidth = (knobSize * 5) + (knobSpacing * 4);
    auto knobRow = controlArea.withTrimmedLeft((controlArea.getWidth() - totalKnobsWidth) / 2).withWidth(totalKnobsWidth);

    volumeSlider.setBounds(knobRow.removeFromLeft(knobSize));
    volumeLabel.setBounds(volumeSlider.getBounds().withTrimmedBottom(5).withTrimmedTop(23));

    knobRow.removeFromLeft(knobSpacing);

    pitchSlider.setBounds(knobRow.removeFromLeft(knobSize));
    pitchLabel.setBounds(pitchSlider.getBounds().withTrimmedBottom(5).withTrimmedTop(23));

    knobRow.removeFromLeft(knobSpacing);

    attackSlider.setBounds(knobRow.removeFromLeft(knobSize));
    attackLabel.setBounds(attackSlider.getBounds().withTrimmedBottom(5).withTrimmedTop(23));

    knobRow.removeFromLeft(knobSpacing);

    decaySlider.setBounds(knobRow.removeFromLeft(knobSize));
    decayLabel.setBounds(decaySlider.getBounds().withTrimmedBottom(5).withTrimmedTop(23));

    knobRow.removeFromLeft(knobSpacing);

    cutoffSlider.setBounds(knobRow.removeFromLeft(knobSize));
    cutoffLabel.setBounds(cutoffSlider.getBounds().withTrimmedBottom(5).withTrimmedTop(23));

    // Step buttons row
    auto stepArea = area.withHeight(50);
    const int buttonWidth = stepArea.getWidth() / stepsPerBank;
    for (int i = 0; i < stepsPerBank; ++i)
    {
        stepButtons[i]->setBounds(
            stepArea.getX() + i * buttonWidth,
            stepArea.getY(),
            buttonWidth - 2,
            stepArea.getHeight()
        );
    }
}

void TrackComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRect(getLocalBounds().toFloat(), 1.0f);
}

void TrackComponent::updatePlayhead(int currentStep, bool isPlaying)
{
    // Calculate which step in current bank to highlight
    // currentStep is 0-63, we need to show only steps in current bank
    const int bankOfCurrentStep = currentStep / stepsPerBank;
    const int stepInBank = currentStep % stepsPerBank;

    for (int i = 0; i < stepsPerBank; ++i)
    {
        const bool isCurrentStep = (i == stepInBank && bankOfCurrentStep == currentBank && isPlaying);
        const bool isActive = stepButtons[i]->getToggleState();

        if (isCurrentStep)
        {
            stepButtons[i]->setColour(juce::TextButton::buttonColourId,
                isActive ? getNeonPink() : getNeonCyan());
        }
        else if (isActive)
        {
            stepButtons[i]->setColour(juce::TextButton::buttonColourId, getNeonPink());
        }
        else
        {
            stepButtons[i]->setColour(juce::TextButton::buttonColourId, getStepInactive());
        }
        stepButtons[i]->repaint();
    }
}

void TrackComponent::updateDecayEnvelope()
{
    juce::ScopedLock lock(synthLock);

    const float attackTime = attack.load();
    const float decayTime = decay.load();

    // Update all SamplerSound instances with new ADSR parameters
    for (int i = 0; i < synth.getNumSounds(); ++i)
    {
        if (auto* sound = dynamic_cast<juce::SamplerSound*>(synth.getSound(i).get()))
        {
            // Note: In JUCE 8, SamplerSound doesn't have a direct setADSR method
            // The ADSR is set at creation time, so we need to reload sample
            // For now, we store values for future sample loads
        }
    }
}

void TrackComponent::changeSampleIndex(int delta)
{
    if (currentSampleFiles.isEmpty())
        return;

    // Calculate new index with wrap-around
    currentSampleIndex += delta;
    if (currentSampleIndex < 0)
        currentSampleIndex = currentSampleFiles.size() - 1;
    else if (currentSampleIndex >= currentSampleFiles.size())
        currentSampleIndex = 0;

    // Load new sample
    loadSampleAtIndex(currentSampleIndex);
}

void TrackComponent::loadSampleAtIndex(int index)
{
    if (index < 0 || index >= currentSampleFiles.size())
        return;

    juce::File sampleFile = currentSampleFiles[index];
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));

    if (!reader)
        return;

    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);

    juce::ScopedLock lock(synthLock);
    synth.clearSounds();

    const float attackTime = attack.load();
    const float decayTime = decay.load();
    juce::String category = getSelectedCategory();
    synth.addSound(new juce::SamplerSound(category, *reader, allNotes, 60, attackTime, decayTime, 1.0));

    updateSampleIndexLabel();
}

void TrackComponent::updateSampleIndexLabel()
{
    if (currentSampleFiles.isEmpty())
        sampleIndexLabel.setText("0 / 0", juce::dontSendNotification);
    else
        sampleIndexLabel.setText(juce::String(currentSampleIndex + 1) + " / " +
            juce::String(currentSampleFiles.size()), juce::dontSendNotification);
}

void TrackComponent::prepareAudio(double sampleRate, int samplesPerBlock)
{
    // Prepare low-pass filter with current cutoff frequency
    auto spec = juce::dsp::ProcessSpec();
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    lowPassFilter.prepare(spec);

    // Set initial filter coefficients (Low-Pass filter)
    const float cutoffFreq = cutoff.load();
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFreq);

    // Store sample rate for filter coefficient updates
    currentSampleRate = sampleRate;
}

void TrackComponent::processAudioBlock(juce::AudioBuffer<float>& buffer)
{
    // Apply low-pass filter with current cutoff frequency
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Update filter coefficients if cutoff changed
    const float cutoffFreq = cutoff.load();
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        currentSampleRate, cutoffFreq);

    lowPassFilter.process(context);
}
