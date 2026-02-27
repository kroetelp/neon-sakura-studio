#include "WavetableSynthEditor.h"
#include "OscillatorSection.h"
#include "FilterSection.h"
#include "EnvelopeSection.h"
#include "WavetableDisplay.h"
#include "Oscilloscope.h"
#include "ModulationGrid.h"
#include "../WavetableSynth/WavetablePreset.h"
#include <algorithm>

WavetableSynthEditor::WavetableSynthEditor(WavetableEngine& externalEngine)
    : engine(&externalEngine), isEngineMode(true)
{
    setLookAndFeel(&customLookAndFeel); 

    sharedParams = engine->getSynthesiser().getSharedParams();

    presetManager = std::make_unique<WavetablePresetManager>();
    wavetableDisplay = std::make_unique<WavetableDisplay>();
    addAndMakeVisible(wavetableDisplay.get());

    osc1Section = std::make_unique<OscillatorSection>(0, "OSC 1");
    addAndMakeVisible(osc1Section.get());

    osc2Section = std::make_unique<OscillatorSection>(1, "OSC 2");
    addAndMakeVisible(osc2Section.get());

    osc3Section = std::make_unique<OscillatorSection>(2, "OSC 3");
    addAndMakeVisible(osc3Section.get());

    filterSection = std::make_unique<FilterSection>();
    addAndMakeVisible(filterSection.get());

    envelopeSection = std::make_unique<EnvelopeSection>();
    addAndMakeVisible(envelopeSection.get());

    oscilloscope = std::make_unique<Oscilloscope>();
    addAndMakeVisible(oscilloscope.get());

    modulationGrid = std::make_unique<ModulationGrid>();
    addAndMakeVisible(modulationGrid.get());

    setupMasterControls();
    setupKeyboard();
    setupPresetUI();
    setupWavetableUI();

    connectUIToEngine();
    if (sharedParams)
        sharedParams->addListener(this);

    startTimerHz(30);
    setSize(1050, 800);
}

WavetableSynthEditor::WavetableSynthEditor(std::shared_ptr<WavetableParams> params)
    : engine(nullptr), sharedParams(params), isEngineMode(false)
{
    setLookAndFeel(&customLookAndFeel);

    presetManager = std::make_unique<WavetablePresetManager>();
    wavetableDisplay = std::make_unique<WavetableDisplay>();
    addAndMakeVisible(wavetableDisplay.get());

    osc1Section = std::make_unique<OscillatorSection>(0, "OSC 1");
    addAndMakeVisible(osc1Section.get());

    osc2Section = std::make_unique<OscillatorSection>(1, "OSC 2");
    addAndMakeVisible(osc2Section.get());

    osc3Section = std::make_unique<OscillatorSection>(2, "OSC 3");
    addAndMakeVisible(osc3Section.get());

    filterSection = std::make_unique<FilterSection>();
    addAndMakeVisible(filterSection.get());

    envelopeSection = std::make_unique<EnvelopeSection>();
    addAndMakeVisible(envelopeSection.get());

    oscilloscope = std::make_unique<Oscilloscope>();
    addAndMakeVisible(oscilloscope.get());

    modulationGrid = std::make_unique<ModulationGrid>();
    addAndMakeVisible(modulationGrid.get());

    setupMasterControls();
    
    setupKeyboard();
    keyboard->setEnabled(false);

    setupPresetUI();
    setupWavetableUI();

    connectUIToSharedParams();
    if (sharedParams)
        sharedParams->addListener(this);

    startTimerHz(30);
    setSize(1050, 800);
}

WavetableSynthEditor::~WavetableSynthEditor()
{
    setLookAndFeel(nullptr);

    if (sharedParams)
        sharedParams->removeListener(this);
    stopTimer();
}

void WavetableSynthEditor::setWavetableData(std::shared_ptr<WavetableData> newWavetable)
{
    loadedWavetable = newWavetable;
    if (wavetableDisplay)
    {
        wavetableDisplay->setWavetable(loadedWavetable);
    }
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
    addAndMakeVisible(masterVolumeLabel);
    masterVolumeLabel.setText("Master", juce::dontSendNotification);
    masterVolumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterVolumeLabel.attachToComponent(&masterVolumeSlider, true);

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

    keyboardState.addListener(this);
}

void WavetableSynthEditor::connectUIToEngine()
{
    if (!engine)
        return;

    auto& params = engine->getSynthesiser().getParams();

    osc1Section->connectToParams(params, 0);
    osc2Section->connectToParams(params, 1);
    osc3Section->connectToParams(params, 2);

    filterSection->connectToParams(params);
    envelopeSection->connectToParams(params);

    wavetableDisplay->connectToParams(params);
    wavetableDisplay->setWavetable(engine->getSynthesiser().getWavetable());

    modulationGrid->connectToMatrix(&engine->getSynthesiser().getModulationMatrix());
}

void WavetableSynthEditor::paint(juce::Graphics& g)
{
    auto gradient = juce::ColourGradient::vertical(
        getDarkBackground().brighter(0.02f), 0,
        getDarkBackground(), getHeight());
    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(getNeonPurple().withAlpha(0.5f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2), 5, 2);

    g.setColour(getNeonPurple());
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("WAVETABLE SYNTH", getLocalBounds().removeFromTop(30),
               juce::Justification::centred, false);
}

void WavetableSynthEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(25);

    wavetableDisplay->setBounds(bounds.removeFromTop(100));

    auto oscRow = bounds.removeFromTop(150);
    int oscWidth = oscRow.getWidth() / 3;
    osc1Section->setBounds(oscRow.removeFromLeft(oscWidth).reduced(2));
    osc2Section->setBounds(oscRow.removeFromLeft(oscWidth).reduced(2));
    osc3Section->setBounds(oscRow.reduced(2));

    auto filterEnvRow = bounds.removeFromTop(120);
    filterSection->setBounds(filterEnvRow.removeFromLeft(filterEnvRow.getWidth() / 2).reduced(2));
    envelopeSection->setBounds(filterEnvRow.reduced(2));

    auto modOscRow = bounds.removeFromTop(150);
    modulationGrid->setBounds(modOscRow.removeFromLeft(modOscRow.getWidth() * 2 / 3).reduced(2));
    oscilloscope->setBounds(modOscRow.reduced(2));

    auto masterRow = bounds.removeFromTop(40);
    masterVolumeLabel.setBounds(masterRow.removeFromLeft(50));
    masterVolumeSlider.setBounds(masterRow.removeFromLeft(200));
    masterRow.removeFromLeft(20);
    standaloneModeButton.setBounds(masterRow.removeFromLeft(100));
    modulatorModeButton.setBounds(masterRow.removeFromLeft(100));

    auto wtRow = bounds.removeFromTop(35);
    loadWavetableButton.setBounds(wtRow.removeFromLeft(120));
    wtRow.removeFromLeft(10);
    wavetableComboBox.setBounds(wtRow.removeFromLeft(300));

    auto presetRow = bounds.removeFromTop(35);
    presetLabel.setBounds(presetRow.removeFromLeft(45));
    presetComboBox.setBounds(presetRow.removeFromLeft(180));
    presetRow.removeFromLeft(10);
    savePresetButton.setBounds(presetRow.removeFromLeft(60));
    presetRow.removeFromLeft(5);
    saveAsPresetButton.setBounds(presetRow.removeFromLeft(70));
    presetRow.removeFromLeft(5);
    loadPresetButton.setBounds(presetRow.removeFromLeft(60));

    keyboard->setBounds(bounds.removeFromBottom(80));
}

void WavetableSynthEditor::timerCallback()
{
    wavetableDisplay->repaint();
}

void WavetableSynthEditor::handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    if (engine)
        engine->noteOn(midiChannel, midiNoteNumber, velocity);
}

void WavetableSynthEditor::handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity)
{
    if (engine)
        engine->noteOff(midiChannel, midiNoteNumber);
}

void WavetableSynthEditor::setupPresetUI()
{
    addAndMakeVisible(presetLabel);
    presetLabel.setText("Preset:", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    presetLabel.attachToComponent(&presetComboBox, true);

    addAndMakeVisible(presetComboBox);
    presetComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground().brighter(0.1f));
    presetComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetComboBox.setColour(juce::ComboBox::outlineColourId, getNeonPurple());
    presetComboBox.onChange = [this] {
        loadSelectedPreset();
    };

    addAndMakeVisible(savePresetButton);
    savePresetButton.setButtonText("Save");
    savePresetButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    savePresetButton.setColour(juce::TextButton::textColourOnId, getNeonGreen());
    savePresetButton.setColour(juce::TextButton::textColourOffId, getNeonGreen());
    savePresetButton.onClick = [this] {
        saveCurrentPreset();
    };

    addAndMakeVisible(saveAsPresetButton);
    saveAsPresetButton.setButtonText("Save As");
    saveAsPresetButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    saveAsPresetButton.setColour(juce::TextButton::textColourOnId, getNeonCyan());
    saveAsPresetButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    saveAsPresetButton.onClick = [this] {
        savePresetAs();
    };

    addAndMakeVisible(loadPresetButton);
    loadPresetButton.setButtonText("Load");
    loadPresetButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    loadPresetButton.setColour(juce::TextButton::textColourOnId, getNeonOrange());
    loadPresetButton.setColour(juce::TextButton::textColourOffId, getNeonOrange());
    loadPresetButton.onClick = [this] {
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

    scanPresets();
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

    presetComboBox.addSeparator();
    presetComboBox.addItem("Init", 999);
}

void WavetableSynthEditor::loadSelectedPreset()
{
    int selectedId = presetComboBox.getSelectedId();

    if (selectedId == 999)
    {
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

    if (isEngineMode && engine)
    {
        presetManager->applyPresetToSynth(currentPreset, engine->getSynthesiser());
    }
    else if (sharedParams)
    {
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
    if (isEngineMode && engine)
    {
        osc1Section->connectToParams(engine->getSynthesiser().getParams(), 0);
        osc2Section->connectToParams(engine->getSynthesiser().getParams(), 1);
        osc3Section->connectToParams(engine->getSynthesiser().getParams(), 2);

        filterSection->connectToParams(engine->getSynthesiser().getParams());
        envelopeSection->connectToParams(engine->getSynthesiser().getParams());
    }
    else if (sharedParams)
    {
        osc1Section->updateFromParams(
            currentPreset.oscParams[0].level, currentPreset.oscParams[0].morph,
            currentPreset.oscParams[0].detune, currentPreset.oscParams[0].unisonCount,
            currentPreset.oscParams[0].panSpread, currentPreset.oscParams[0].pan,
            currentPreset.oscParams[0].pitchOffset
        );

        osc2Section->updateFromParams(
            currentPreset.oscParams[1].level, currentPreset.oscParams[1].morph,
            currentPreset.oscParams[1].detune, currentPreset.oscParams[1].unisonCount,
            currentPreset.oscParams[1].panSpread, currentPreset.oscParams[1].pan,
            currentPreset.oscParams[1].pitchOffset
        );

        osc3Section->updateFromParams(
            currentPreset.oscParams[2].level, currentPreset.oscParams[2].morph,
            currentPreset.oscParams[2].detune, currentPreset.oscParams[2].unisonCount,
            currentPreset.oscParams[2].panSpread, currentPreset.oscParams[2].pan,
            currentPreset.oscParams[2].pitchOffset
        );

        filterSection->updateFromParams(
            currentPreset.filterParams.cutoff, currentPreset.filterParams.resonance,
            currentPreset.filterParams.drive, currentPreset.filterParams.type
        );

        envelopeSection->updateFromParams(
            currentPreset.ampEnvelope.attack, currentPreset.ampEnvelope.decay,
            currentPreset.ampEnvelope.sustain, currentPreset.ampEnvelope.release
        );

        masterVolumeSlider.setValue(currentPreset.masterVolume, juce::dontSendNotification);
    }

    if (wavetableDisplay)
    {
        wavetableDisplay->refresh();
    }
}

void WavetableSynthEditor::onParamsChanged()
{
    triggerAsyncUpdate();
}

void WavetableSynthEditor::onOscParamChanged(int oscIndex, int paramId)
{
    (void)oscIndex;
    (void)paramId;
    triggerAsyncUpdate();
}

void WavetableSynthEditor::onFilterParamChanged()
{
    triggerAsyncUpdate();
}

void WavetableSynthEditor::onEnvelopeParamChanged()
{
    triggerAsyncUpdate();
}

void WavetableSynthEditor::connectUIToSharedParams()
{
    if (!sharedParams)
        return;

    masterVolumeSlider.onValueChange = [this] {
        if (sharedParams)
            sharedParams->setMasterLevel(static_cast<float>(masterVolumeSlider.getValue()));
    };

    osc1Section->connectToSharedParams(sharedParams, 0);
    osc2Section->connectToSharedParams(sharedParams, 1);
    osc3Section->connectToSharedParams(sharedParams, 2);

    filterSection->connectToSharedParams(sharedParams);
    envelopeSection->connectToSharedParams(sharedParams);

    if (wavetableDisplay)
    {
        wavetableDisplay->connectToSharedParams(sharedParams);
        
        if (loadedWavetable)
        {
            wavetableDisplay->setWavetable(loadedWavetable);
        }
    }
}

void WavetableSynthEditor::updateUIFromParams()
{
    if (!sharedParams)
        return;

    masterVolumeSlider.setValue(sharedParams->getMasterLevel(), juce::dontSendNotification);

    for (int i = 0; i < 3; ++i)
    {
        auto* osc = (i == 0) ? osc1Section.get() : (i == 1) ? osc2Section.get() : osc3Section.get();
        osc->updateFromParams(
            sharedParams->getOscLevel(i),
            sharedParams->getOscMorph(i),
            sharedParams->getOscDetune(i),
            sharedParams->getOscUnisonCount(i),
            sharedParams->getOscPanSpread(i),
            sharedParams->getOscPan(i),
            sharedParams->getOscPitchOffset(i)
        );
    }

    filterSection->updateFromParams(
        sharedParams->getFilterCutoff(),
        sharedParams->getFilterResonance(),
        sharedParams->getFilterDrive(),
        sharedParams->getFilterMode()
    );

    envelopeSection->updateFromParams(
        sharedParams->getEnvAttack(),
        sharedParams->getEnvDecay(),
        sharedParams->getEnvSustain(),
        sharedParams->getEnvRelease()
    );

    // --- FIX: Sicherstellen, dass das Display sich beim Track-Wechsel anpasst ---
    if (wavetableDisplay)
    {
        wavetableDisplay->refresh();
    }
}

void WavetableSynthEditor::triggerAsyncUpdate()
{
    juce::MessageManager::callAsync([this]() {
        updateUIFromParams();
    });
}

void WavetableSynthEditor::setupWavetableUI()
{
    addAndMakeVisible(loadWavetableButton);
    loadWavetableButton.setButtonText("WT Folder...");
    loadWavetableButton.setColour(juce::TextButton::buttonColourId, getDarkBackground().brighter(0.1f));
    loadWavetableButton.setColour(juce::TextButton::textColourOnId, getNeonPink());
    loadWavetableButton.setColour(juce::TextButton::textColourOffId, getNeonPink());
    loadWavetableButton.onClick = [this] {
        openWavetableFolder();
    };

    addAndMakeVisible(wavetableComboBox);
    wavetableComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground().brighter(0.1f));
    wavetableComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    wavetableComboBox.setColour(juce::ComboBox::outlineColourId, getNeonPink());
    wavetableComboBox.setTextWhenNothingSelected("No Wavetable Selected");
    wavetableComboBox.onChange = [this] {
        loadSelectedWavetable();
    };
}

void WavetableSynthEditor::openWavetableFolder()
{
    auto startDir = currentWavetableDir.exists() ? currentWavetableDir
                                                  : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    auto chooser = std::make_shared<juce::FileChooser>(
        "Select Wavetable Folder",
        startDir);

    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getResults().size() > 0)
            {
                auto selectedDir = fc.getResult();
                if (selectedDir.isDirectory())
                {
                    currentWavetableDir = selectedDir;
                    scanWavetableFolder(selectedDir);
                }
            }
        });
}

void WavetableSynthEditor::scanWavetableFolder(const juce::File& dir)
{
    wavetableFiles.clear();
    wavetableComboBox.clear();

    // Rekursive Suche nach Audio-Dateien
    juce::Array<juce::File> foundFiles;
    dir.findChildFiles(foundFiles, juce::File::findFiles, true, "*.wav;*.aiff;*.flac;*.ogg");

    wavetableFiles.addArray(foundFiles);

    // Alphabetisch sortieren (natural sort)
    std::sort(wavetableFiles.begin(), wavetableFiles.end(),
        [](const juce::File& a, const juce::File& b)
        {
            return a.getFullPathName().compareNatural(b.getFullPathName()) < 0;
        });

    // ComboBox befüllen
    int itemId = 1;
    for (const auto& file : wavetableFiles)
    {
        // Relativen Pfad berechnen
        juce::String relativePath = file.getRelativePathFrom(dir);

        // Dateiendung entfernen
        juce::String displayText = relativePath.upToLastOccurrenceOf(".", false, false);

        // Slashes durch " / " ersetzen für bessere Lesbarkeit
        displayText = displayText.replace("\\", " / ").replace("/", " / ");

        wavetableComboBox.addItem(displayText, itemId);
        ++itemId;
    }

    // Erstes Item automatisch auswählen
    if (wavetableFiles.size() > 0)
    {
        wavetableComboBox.setSelectedId(1, juce::dontSendNotification);
        loadSelectedWavetable();
    }
}

void WavetableSynthEditor::loadSelectedWavetable()
{
    int selectedId = wavetableComboBox.getSelectedId();

    if (selectedId < 1 || selectedId > wavetableFiles.size())
        return;

    const auto& file = wavetableFiles.getReference(selectedId - 1);

    auto newWavetable = std::make_shared<WavetableData>();
    if (newWavetable->loadFromFile(file))
    {
        loadedWavetable = newWavetable;
        applyLoadedWavetable();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Load Error",
            "Could not load wavetable:\n" + newWavetable->getLastError(),
            "OK");
    }
}

void WavetableSynthEditor::applyLoadedWavetable()
{
    if (!loadedWavetable)
        return;

    if (isEngineMode && engine)
    {
        engine->getSynthesiser().setWavetable(loadedWavetable);
    }

    if (wavetableDisplay)
    {
        wavetableDisplay->setWavetable(loadedWavetable);
    }
}