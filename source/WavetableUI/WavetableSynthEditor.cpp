#include "WavetableSynthEditor.h"
#include "OscillatorSection.h"
#include "FilterSection.h"
#include "EnvelopeSection.h"
#include "WavetableDisplay.h"
#include "Oscilloscope.h"
#include "ModulationGrid.h"
#include "../WavetableSynth/WavetablePreset.h"

WavetableSynthEditor::WavetableSynthEditor(WavetableEngine& externalEngine)
    : engine(&externalEngine), isEngineMode(true)
{
    // Get shared params from engine
    sharedParams = engine->getSynthesiser().getSharedParams();

    // Initialize preset manager
    presetManager = std::make_unique<WavetablePresetManager>();
    // Create wavetable display
    wavetableDisplay = std::make_unique<WavetableDisplay>();
    addAndMakeVisible(wavetableDisplay.get());

    // Create oscillator sections
    osc1Section = std::make_unique<OscillatorSection>(0, "OSC 1");
    addAndMakeVisible(osc1Section.get());

    osc2Section = std::make_unique<OscillatorSection>(1, "OSC 2");
    addAndMakeVisible(osc2Section.get());

    osc3Section = std::make_unique<OscillatorSection>(2, "OSC 3");
    addAndMakeVisible(osc3Section.get());

    // Create filter section
    filterSection = std::make_unique<FilterSection>();
    addAndMakeVisible(filterSection.get());

    // Create envelope section
    envelopeSection = std::make_unique<EnvelopeSection>();
    addAndMakeVisible(envelopeSection.get());

    // Create oscilloscope
    oscilloscope = std::make_unique<Oscilloscope>();
    addAndMakeVisible(oscilloscope.get());

    // Create modulation grid
    modulationGrid = std::make_unique<ModulationGrid>();
    addAndMakeVisible(modulationGrid.get());

    // Setup master controls
    setupMasterControls();

    // Setup keyboard
    setupKeyboard();

    // Setup preset UI
    setupPresetUI();

    // Setup wavetable loading UI
    setupWavetableUI();

    // Connect UI to engine
    connectUIToEngine();
    if (sharedParams)
        sharedParams->addListener(this);

    // Start timer for UI updates (30 fps)
    startTimerHz(30);

    // Default size
    setSize(1050, 800);
}

WavetableSynthEditor::WavetableSynthEditor(std::shared_ptr<WavetableParams> params)
    : engine(nullptr), sharedParams(params), isEngineMode(false)
{
    // Initialize preset manager
    presetManager = std::make_unique<WavetablePresetManager>();
    // Create wavetable display
    wavetableDisplay = std::make_unique<WavetableDisplay>();
    addAndMakeVisible(wavetableDisplay.get());

    // Create oscillator sections
    osc1Section = std::make_unique<OscillatorSection>(0, "OSC 1");
    addAndMakeVisible(osc1Section.get());

    osc2Section = std::make_unique<OscillatorSection>(1, "OSC 2");
    addAndMakeVisible(osc2Section.get());

    osc3Section = std::make_unique<OscillatorSection>(2, "OSC 3");
    addAndMakeVisible(osc3Section.get());

    // Create filter section
    filterSection = std::make_unique<FilterSection>();
    addAndMakeVisible(filterSection.get());

    // Create envelope section
    envelopeSection = std::make_unique<EnvelopeSection>();
    addAndMakeVisible(envelopeSection.get());

    // Create oscilloscope
    oscilloscope = std::make_unique<Oscilloscope>();
    addAndMakeVisible(oscilloscope.get());

    // Create modulation grid
    modulationGrid = std::make_unique<ModulationGrid>();
    addAndMakeVisible(modulationGrid.get());

    // Setup master controls (limited in params mode)
    setupMasterControls();

    // Setup keyboard (disabled in params mode - no engine)
    setupKeyboard();
    keyboard->setEnabled(false);

    // Setup preset UI
    setupPresetUI();

    // Setup wavetable loading UI
    setupWavetableUI();

    // Connect UI to shared params
    connectUIToSharedParams();
    if (sharedParams)
        sharedParams->addListener(this);

    // Start timer for UI updates (30 fps)
    startTimerHz(30);

    // Default size
    setSize(1050, 800);
}

WavetableSynthEditor::~WavetableSynthEditor()
{
    if (sharedParams)
        sharedParams->removeListener(this);
    stopTimer();
}

void WavetableSynthEditor::setSharedParams(std::shared_ptr<WavetableParams> newParams)
{
    if (sharedParams)
        sharedParams->removeListener(this);

    sharedParams = newParams;

    if (sharedParams)
    {
        sharedParams->addListener(this);
        connectUIToSharedParams();
        updateUIFromParams();
    }
}

void WavetableSynthEditor::setupMasterControls()
{
    // Master volume label
    addAndMakeVisible(masterVolumeLabel);
    masterVolumeLabel.setText("Master", juce::dontSendNotification);
    masterVolumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterVolumeLabel.attachToComponent(&masterVolumeSlider, true);

    // Master volume slider
    addAndMakeVisible(masterVolumeSlider);
    masterVolumeSlider.setRange(0.0, 1.0, 0.01);
    masterVolumeSlider.setValue(0.8);
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    masterVolumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    masterVolumeSlider.onValueChange = [this] {
        if (isEngineMode && engine)
            engine->setMasterVolume(static_cast<float>(masterVolumeSlider.getValue()));
        else if (sharedParams)
            sharedParams->setMasterLevel(static_cast<float>(masterVolumeSlider.getValue()));
    };

    // Mode toggle buttons (only in engine mode)
    addAndMakeVisible(standaloneModeButton);
    standaloneModeButton.setButtonText("Standalone");
    standaloneModeButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    standaloneModeButton.setColour(juce::ToggleButton::tickColourId, getNeonPurple());
    standaloneModeButton.setToggleState(true, juce::dontSendNotification);
    standaloneModeButton.setEnabled(isEngineMode);
    standaloneModeButton.onClick = [this] {
        if (standaloneModeButton.getToggleState() && engine)
        {
            modulatorModeButton.setToggleState(false, juce::dontSendNotification);
            engine->setMode(WavetableEngine::Mode::Standalone);
        }
    };

    addAndMakeVisible(modulatorModeButton);
    modulatorModeButton.setButtonText("Modulator");
    modulatorModeButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    modulatorModeButton.setColour(juce::ToggleButton::tickColourId, getNeonCyan());
    modulatorModeButton.setEnabled(isEngineMode);
    modulatorModeButton.onClick = [this] {
        if (modulatorModeButton.getToggleState() && engine)
        {
            standaloneModeButton.setToggleState(false, juce::dontSendNotification);
            engine->setMode(WavetableEngine::Mode::Modulator);
        }
    };
}

void WavetableSynthEditor::setupKeyboard()
{
    keyboard = std::make_unique<juce::MidiKeyboardComponent>(
        keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard);
    keyboard->setKeyWidth(25);
    keyboard->setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
    keyboard->setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colours::black);
    keyboard->setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colours::grey);
    keyboard->setOctaveForMiddleC(4);
    addAndMakeVisible(keyboard.get());

    // Connect keyboard to engine
    keyboardState.addListener(this);
}

void WavetableSynthEditor::connectUIToEngine()
{
    if (!engine)
        return;

    // Connect oscillator sections to engine parameters
    auto& params = engine->getSynthesiser().getParams();

    osc1Section->connectToParams(params, 0);
    osc2Section->connectToParams(params, 1);
    osc3Section->connectToParams(params, 2);

    // Connect filter section
    filterSection->connectToParams(params);

    // Connect envelope section
    envelopeSection->connectToParams(params);

    // Connect wavetable display to params and wavetable data
    wavetableDisplay->connectToParams(params);
    wavetableDisplay->setWavetable(engine->getSynthesiser().getWavetable());

    // Connect modulation grid
    modulationGrid->connectToMatrix(&engine->getSynthesiser().getModulationMatrix());
}

void WavetableSynthEditor::paint(juce::Graphics& g)
{
    // Background gradient
    auto gradient = juce::ColourGradient::vertical(
        getDarkBackground().brighter(0.02f), 0,
        getDarkBackground(), getHeight());
    g.setGradientFill(gradient);
    g.fillAll();

    // Border
    g.setColour(getNeonPurple().withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5, 2);

    // Title
    g.setColour(getNeonPurple());
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("WAVETABLE SYNTH", getLocalBounds().removeFromTop(30),
               juce::Justification::centred, false);
}

void WavetableSynthEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);  // Title

    // Wavetable display at top
    wavetableDisplay->setBounds(bounds.removeFromTop(100));

    // Oscillator sections in a row
    auto oscRow = bounds.removeFromTop(150);
    int oscWidth = oscRow.getWidth() / 3;
    osc1Section->setBounds(oscRow.removeFromLeft(oscWidth).reduced(2));
    osc2Section->setBounds(oscRow.removeFromLeft(oscWidth).reduced(2));
    osc3Section->setBounds(oscRow.reduced(2));

    // Filter and Envelope row
    auto filterEnvRow = bounds.removeFromTop(120);
    filterSection->setBounds(filterEnvRow.removeFromLeft(filterEnvRow.getWidth() / 2).reduced(2));
    envelopeSection->setBounds(filterEnvRow.reduced(2));

    // Modulation grid and oscilloscope row
    auto modOscRow = bounds.removeFromTop(150);
    modulationGrid->setBounds(modOscRow.removeFromLeft(modOscRow.getWidth() * 2 / 3).reduced(2));
    oscilloscope->setBounds(modOscRow.reduced(2));

    // Master controls row
    auto masterRow = bounds.removeFromTop(40);
    masterVolumeLabel.setBounds(masterRow.removeFromLeft(50));
    masterVolumeSlider.setBounds(masterRow.removeFromLeft(200));
    masterRow.removeFromLeft(20);
    standaloneModeButton.setBounds(masterRow.removeFromLeft(100));
    modulatorModeButton.setBounds(masterRow.removeFromLeft(100));

    // Wavetable loading row
    auto wtRow = bounds.removeFromTop(35);
    loadWavetableButton.setBounds(wtRow.removeFromLeft(120));
    wtRow.removeFromLeft(10);
    wavetableNameLabel.setBounds(wtRow.removeFromLeft(200));

    // Preset controls row
    auto presetRow = bounds.removeFromTop(35);
    presetLabel.setBounds(presetRow.removeFromLeft(45));
    presetComboBox.setBounds(presetRow.removeFromLeft(180));
    presetRow.removeFromLeft(10);
    savePresetButton.setBounds(presetRow.removeFromLeft(60));
    presetRow.removeFromLeft(5);
    saveAsPresetButton.setBounds(presetRow.removeFromLeft(70));
    presetRow.removeFromLeft(5);
    loadPresetButton.setBounds(presetRow.removeFromLeft(60));

    // Keyboard at bottom
    keyboard->setBounds(bounds.removeFromBottom(80));
}

void WavetableSynthEditor::timerCallback()
{
    // Update wavetable display for morph animation
    wavetableDisplay->repaint();
}

void WavetableSynthEditor::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    if (engine)
        engine->noteOn(midiNoteNumber, velocity);
}

void WavetableSynthEditor::handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    if (engine)
        engine->noteOff(midiNoteNumber);
}

void WavetableSynthEditor::setupPresetUI()
{
    // Preset label
    addAndMakeVisible(presetLabel);
    presetLabel.setText("Preset:", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    presetLabel.attachToComponent(&presetComboBox, true);

    // Preset combo box
    addAndMakeVisible(presetComboBox);
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground().brighter(0.1f));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetComboBox.setColour(juce::ComboBox::outlineColourId, getNeonPurple());
    presetComboBox.onChange = [this] {
        loadSelectedPreset();
    };

    // Save button
    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("Save");
    savePresetButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    savePresetButton.setColour(juce::TextButton::textColourOnId, getNeonGreen());
    savePresetButton.setColour(juce::TextButton::textColourOffId, getNeonGreen());
    savePresetButton.onClick = [this] {
        saveCurrentPreset();
    };

    // Save As button
    addAndMakeVisible(saveAsPresetButton);
    saveAsPresetButton.setButtonText("Save As");
    saveAsPresetButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    saveAsPresetButton.setColour(juce::TextButton::textColourOnId, getNeonCyan());
    saveAsPresetButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    saveAsPresetButton.onClick = [this] {
        savePresetAs();
    };

    // Load button
    addAndMakeVisible(loadPresetButton);
    loadPresetButton.setButtonText("Load");
    loadPresetButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    loadPresetButton.setColour(juce::TextButton::textColourOnId, getNeonOrange());
    loadPresetButton.setColour(juce::TextButton::textColourOffId, getNeonOrange());
    loadPresetButton.onClick = [this] {
        // File browser for loading external presets (async in JUCE 8)
        auto chooser = std::make_shared<juce::FileChooser>("Load Preset",
            presetManager->getUserPresetDirectory(),
            "*.wtpreset");

        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                if (fc.getResults().size() > 0)
                {
                    auto file = fc.getResult();
                    currentPreset = presetManager->loadPresetFromFile(file);
                    if (engine)
                        presetManager->applyPresetToSynth(currentPreset, engine->getSynthesiser());
                    applyPresetToUI();
                }
            });
    };

    // Scan for presets
    scanPresets();

    // Initialize with init preset
    currentPreset = WavetablePreset::createInitPreset();
}

void WavetableSynthEditor::scanPresets()
{
    presetFiles = presetManager->getAllPresetFiles();
    updatePresetComboBox();
}

void WavetableSynthEditor::updatePresetComboBox()
{
    presetComboBox.clear();

    int index = 1;
    for (const auto& file : presetFiles)
    {
        presetComboBox.addItem(presetManager->getPresetNameFromFile(file), index);
        ++index;
    }

    // Add separator and init
    presetComboBox.addSeparator();
    presetComboBox.addItem("Init", 999);
}

void WavetableSynthEditor::loadSelectedPreset()
{
    int selectedId = presetComboBox.getSelectedId();

    if (selectedId == 999)
    {
        // Init preset
        currentPreset = WavetablePreset::createInitPreset();
    }
    else if (selectedId > 0 && selectedId <= presetFiles.size())
    {
        const auto& file = presetFiles[selectedId - 1];
        currentPreset = presetManager->loadPresetFromFile(file);
    }
    else
    {
        return;
    }

    // Apply preset based on mode
    if (isEngineMode && engine)
    {
        presetManager->applyPresetToSynth(currentPreset, engine->getSynthesiser());
    }
    else if (sharedParams)
    {
        // Apply preset to shared params (track mode)
        for (int i = 0; i < 3; ++i)
        {
            sharedParams->setOscLevel(i, currentPreset.oscParams[i].level);
            sharedParams->setOscMorph(i, currentPreset.oscParams[i].morph);
            sharedParams->setOscDetune(i, currentPreset.oscParams[i].detune);
            sharedParams->setOscUnisonCount(i, currentPreset.oscParams[i].unisonCount);
            sharedParams->setOscPanSpread(i, currentPreset.oscParams[i].panSpread);
            sharedParams->setOscPan(i, currentPreset.oscParams[i].pan);
            sharedParams->setOscPitchOffset(i, currentPreset.oscParams[i].pitchOffset);
        }

        sharedParams->setSubLevel(currentPreset.subOscParams.level);
        sharedParams->setSubOctave(currentPreset.subOscParams.octave);
        sharedParams->setSubWaveform(currentPreset.subOscParams.waveform);

        sharedParams->setFilterCutoff(currentPreset.filterParams.cutoff);
        sharedParams->setFilterResonance(currentPreset.filterParams.resonance);
        sharedParams->setFilterDrive(currentPreset.filterParams.drive);
        sharedParams->setFilterMode(currentPreset.filterParams.type);

        sharedParams->setEnvAttack(currentPreset.ampEnvelope.attack);
        sharedParams->setEnvDecay(currentPreset.ampEnvelope.decay);
        sharedParams->setEnvSustain(currentPreset.ampEnvelope.sustain);
        sharedParams->setEnvRelease(currentPreset.ampEnvelope.release);

        sharedParams->setMasterLevel(currentPreset.masterVolume);
    }

    applyPresetToUI();
}

void WavetableSynthEditor::saveCurrentPreset()
{
    if (!engine)
        return;

    if (currentPreset.name.isEmpty() || currentPreset.name == "Init")
    {
        savePresetAs();
        return;
    }

    // Extract current state from synth
    currentPreset = presetManager->extractPresetFromSynth(engine->getSynthesiser(), currentPreset.name);

    if (presetManager->saveUserPreset(currentPreset))
    {
        scanPresets();
    }
}

void WavetableSynthEditor::savePresetAs()
{
    if (!engine)
        return;

    // Show alert window for name input
    auto* alertWindow = new juce::AlertWindow("Save Preset",
        "Enter a name for the preset:",
        juce::AlertWindow::NoIcon);

    alertWindow->addTextEditor("presetName", currentPreset.name, "Preset Name:");
    alertWindow->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alertWindow->enterModalState(true,
        juce::ModalCallbackFunction::create([this, alertWindow](int result) {
        if (result == 1 && engine)
        {
            juce::String name = alertWindow->getTextEditorContents("presetName");
            if (name.isNotEmpty())
            {
                currentPreset = presetManager->extractPresetFromSynth(engine->getSynthesiser(), name);
                if (presetManager->saveUserPreset(currentPreset))
                {
                    scanPresets();
                }
            }
        }
        delete alertWindow;
    }), false);
}

void WavetableSynthEditor::applyPresetToUI()
{
    // Update oscillator sections
    if (isEngineMode && engine)
    {
        osc1Section->connectToParams(engine->getSynthesiser().getParams(), 0);
        osc2Section->connectToParams(engine->getSynthesiser().getParams(), 1);
        osc3Section->connectToParams(engine->getSynthesiser().getParams(), 2);

        // Update filter section
        filterSection->connectToParams(engine->getSynthesiser().getParams());

        // Update envelope section
        envelopeSection->connectToParams(engine->getSynthesiser().getParams());
    }
    else if (sharedParams)
    {
        // Update UI from preset values
        osc1Section->updateFromParams(
            currentPreset.oscParams[0].level,
            currentPreset.oscParams[0].morph,
            currentPreset.oscParams[0].detune,
            currentPreset.oscParams[0].unisonCount,
            currentPreset.oscParams[0].panSpread,
            currentPreset.oscParams[0].pan,
            currentPreset.oscParams[0].pitchOffset
        );

        osc2Section->updateFromParams(
            currentPreset.oscParams[1].level,
            currentPreset.oscParams[1].morph,
            currentPreset.oscParams[1].detune,
            currentPreset.oscParams[1].unisonCount,
            currentPreset.oscParams[1].panSpread,
            currentPreset.oscParams[1].pan,
            currentPreset.oscParams[1].pitchOffset
        );

        osc3Section->updateFromParams(
            currentPreset.oscParams[2].level,
            currentPreset.oscParams[2].morph,
            currentPreset.oscParams[2].detune,
            currentPreset.oscParams[2].unisonCount,
            currentPreset.oscParams[2].panSpread,
            currentPreset.oscParams[2].pan,
            currentPreset.oscParams[2].pitchOffset
        );

        // Update filter section
        filterSection->updateFromParams(
            currentPreset.filterParams.cutoff,
            currentPreset.filterParams.resonance,
            currentPreset.filterParams.drive,
            currentPreset.filterParams.type
        );

        // Update envelope section
        envelopeSection->updateFromParams(
            currentPreset.ampEnvelope.attack,
            currentPreset.ampEnvelope.decay,
            currentPreset.ampEnvelope.sustain,
            currentPreset.ampEnvelope.release
        );

        // Update master volume slider
        masterVolumeSlider.setValue(currentPreset.masterVolume, juce::dontSendNotification);
    }

    // Refresh wavetable display to show updated morph positions
    if (wavetableDisplay)
    {
        wavetableDisplay->refresh();
    }
}

void WavetableSynthEditor::onParamsChanged()
{
    // Called when shared params change - update UI
    triggerAsyncUpdate();
}

void WavetableSynthEditor::onOscParamChanged(int oscIndex, int paramId)
{
    // Called when a specific oscillator param changes
    (void)oscIndex;
    (void)paramId;
    triggerAsyncUpdate();
}

void WavetableSynthEditor::onFilterParamChanged()
{
    // Called when filter params change
    triggerAsyncUpdate();
}

void WavetableSynthEditor::onEnvelopeParamChanged()
{
    // Called when envelope params change
    triggerAsyncUpdate();
}

void WavetableSynthEditor::connectUIToSharedParams()
{
    if (!sharedParams)
        return;

    // Update sliders to use shared params directly
    masterVolumeSlider.onValueChange = [this] {
        if (sharedParams)
            sharedParams->setMasterLevel(static_cast<float>(masterVolumeSlider.getValue()));
    };

    // Connect oscillator sections to shared params
    osc1Section->connectToSharedParams(sharedParams, 0);
    osc2Section->connectToSharedParams(sharedParams, 1);
    osc3Section->connectToSharedParams(sharedParams, 2);

    // Connect filter section to shared params
    filterSection->connectToSharedParams(sharedParams);

    // Connect envelope section to shared params
    envelopeSection->connectToSharedParams(sharedParams);

    // If we have a loaded wavetable, update the display
    if (wavetableDisplay && loadedWavetable)
    {
        wavetableDisplay->setWavetable(loadedWavetable);
    }
}

void WavetableSynthEditor::updateUIFromParams()
{
    if (!sharedParams)
        return;

    // Update master volume slider to reflect current param value
    masterVolumeSlider.setValue(sharedParams->getMasterLevel(), juce::dontSendNotification);

    // Note: OscillatorSection, FilterSection, EnvelopeSection don't have
    // a method to update their sliders from params yet. The callbacks
    // are set up in connectUIToSharedParams() and will work for future changes.
}

void WavetableSynthEditor::triggerAsyncUpdate()
{
    // Use MessageManager for thread-safe UI updates
    juce::MessageManager::callAsync([this]() {
        updateUIFromParams();
    });
}

void WavetableSynthEditor::setupWavetableUI()
{
    // Load Wavetable button
    addAndMakeVisible(loadWavetableButton);
    loadWavetableButton.setButtonText("Load Wavetable");
    loadWavetableButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    loadWavetableButton.setColour(juce::TextButton::textColourOnId, getNeonPink());
    loadWavetableButton.setColour(juce::TextButton::textColourOffId, getNeonPink());
    loadWavetableButton.onClick = [this] {
        loadWavetableFromFile();
    };

    // Wavetable name label
    addAndMakeVisible(wavetableNameLabel);
    wavetableNameLabel.setText("Basic Waveforms", juce::dontSendNotification);
    wavetableNameLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    wavetableNameLabel.setFont(juce::Font(12.0f));
}

void WavetableSynthEditor::loadWavetableFromFile()
{
    // Create file chooser for audio files
    auto chooser = std::make_shared<juce::FileChooser>(
        "Load Wavetable",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.wav;*.aiff;*.flac;*.ogg");

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().size() > 0)
            {
                auto file = fc.getResult();

                // Create new wavetable and load
                auto newWavetable = std::make_shared<WavetableData>();
                if (newWavetable->loadFromFile(file))
                {
                    loadedWavetable = newWavetable;
                    applyLoadedWavetable();

                    // Update label with wavetable name
                    wavetableNameLabel.setText(newWavetable->getName(), juce::dontSendNotification);
                }
                else
                {
                    // Show error message
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Load Error",
                        "Could not load wavetable:\n" + newWavetable->getLastError(),
                        "OK");
                }
            }
        });
}

void WavetableSynthEditor::applyLoadedWavetable()
{
    if (!loadedWavetable)
        return;

    // Apply to engine's synth if in engine mode
    if (isEngineMode && engine)
    {
        engine->getSynthesiser().setWavetable(loadedWavetable);
    }

    // Always update wavetable display - this is critical for visualization
    if (wavetableDisplay)
    {
        wavetableDisplay->setWavetable(loadedWavetable);
    }
}
