#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Register audio formats first
    formatManager.registerBasicFormats();

    // Create track components first, before audio init
    for (int i = 0; i < numTracks; ++i)
    {
        tracks[i] = std::make_unique<TrackComponent>(i, formatManager);
        addAndMakeVisible(tracks[i].get());

        // Add category change handler to load samples
        tracks[i]->getComboBox().onChange = [this, i] {
            juce::String category = tracks[i]->getSelectedCategory();
            if (category.isNotEmpty() && sampleDirectory.exists())
            {
                tracks[i]->loadSampleForCategory(category, sampleDirectory);
            }
        };
    }

    // Auto-detect sample directory on startup
    autoDetectSampleDirectory();

    // Then set audio channels
    setAudioChannels(2, 2);

    // Play button
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, getNeonPink());
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(playButton);
    playButton.onClick = [this] { togglePlay(); };

    // Stop button
    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, getNeonCyan());
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(stopButton);
    stopButton.onClick = [this] { stopPlayback(); };

    // Set folder button
    setFolderButton.setButtonText("Set SuperDirt Folder");
    setFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 50, 70));
    setFolderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(setFolderButton);
    setFolderButton.onClick = [this] { openFolderChooser(); };

    // Folder label
    folderLabel.setText("No folder selected", juce::dontSendNotification);
    folderLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    folderLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(folderLabel);

    // BPM slider
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 30);
    bpmSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    bpmSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bpmSlider);
    bpmSlider.onValueChange = [this] {
        bpm.store(bpmSlider.getValue());
        calculateSamplesPerStep();
    };

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(bpmLabel);
    bpmLabel.attachToComponent(&bpmSlider, true);

    // Loop length combo box
    loopLengthComboBox.addItem("16 Steps", 16);
    loopLengthComboBox.addItem("32 Steps", 32);
    loopLengthComboBox.addItem("48 Steps", 48);
    loopLengthComboBox.addItem("64 Steps", 64);
    loopLengthComboBox.setSelectedId(16);
    loopLengthComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    loopLengthComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    loopLengthComboBox.setColour(juce::ComboBox::arrowColourId, getNeonCyan());
    addAndMakeVisible(loopLengthComboBox);
    loopLengthComboBox.onChange = [this] {
        loopLength.store(loopLengthComboBox.getSelectedId());
        lastPlayedStep = -1;
    };

    addAndMakeVisible(loopLengthLabel);
    loopLengthLabel.setText("Loop", juce::dontSendNotification);
    loopLengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    loopLengthLabel.setFont(juce::Font(10.0f));

    // Start GUI timer for playhead updates (30 FPS)
    startTimerHz(30);

    calculateSamplesPerStep();

    setSize(1000, 600);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    currentSampleRate = newSampleRate;

    // Prepare all track synthesizers and DSP filters
    for (auto& track : tracks)
    {
        track->getSynthesiser().setCurrentPlaybackSampleRate(newSampleRate);
        track->prepareAudio(newSampleRate, samplesPerBlockExpected);
    }
    calculateSamplesPerStep();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    const int samplesPerStep = this->samplesPerStep.load();
    const double sampleRateValue = currentSampleRate;
    const bool localIsPlaying = this->isPlaying;

    // Separate MIDI buffer for each track to prevent cross-talk
    std::array<juce::MidiBuffer, numTracks> trackMidiBuffers;

    if (localIsPlaying && samplesPerStep > 0)
    {
        samplePosition.fetch_add(bufferToFill.numSamples);

        // Use dynamic loop length (16, 32, 48, or 64 steps)
        const int loopLen = loopLength.load();
        const int calculatedCurrentStep = (int)(samplePosition.load() / samplesPerStep) % loopLen;

        const bool isNewStep = (calculatedCurrentStep != lastPlayedStep);

        if (isNewStep)
        {
            // Increment global loop counter for Slow (/) modifier
            if (calculatedCurrentStep == 0)
                globalLoopCounter++;

            for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
            {
                if (tracks[trackIdx]->isStepActive(calculatedCurrentStep))
                {
                    // Get step state with modifiers
                    const StepModifierState stepState = tracks[trackIdx]->getStepState(calculatedCurrentStep);

                    // Get control values from track
                    const float trackVolume = tracks[trackIdx]->getVolume();
                    const int trackPitch = tracks[trackIdx]->getPitch();

                    // Calculate velocity with volume
                    const juce::uint8 velocity = static_cast<juce::uint8>(trackVolume * 127);

                    // Calculate MIDI note with pitch offset
                    const int midiNote = 60 + trackPitch;

                    // Handle different modifier types
                    if (stepState.modifierType == '*')
                    {
                        // Speed (*): Ratcheting logic - trigger modifierValue times within step
                        const int samplesPerRatchetStep = samplesPerStep / stepState.modifierValue;
                        for (int ratchetIndex = 0; ratchetIndex < stepState.modifierValue; ++ratchetIndex)
                        {
                            const int sampleOffset = ratchetIndex * samplesPerRatchetStep;
                            // Add event at correct offset within this buffer
                            juce::MidiMessage midiMessage =
                                juce::MidiMessage::noteOn(1, midiNote, velocity);
                            trackMidiBuffers[trackIdx].addEvent(midiMessage, sampleOffset);
                        }
                    }
                    else if (stepState.modifierType == '/')
                    {
                        // Slow (/): Only trigger on every Nth cycle
                        const int cycleCheck = (globalLoopCounter > 0) ? (globalLoopCounter - 1) : 0;
                        if (cycleCheck % stepState.modifierValue == 0)
                        {
                            juce::MidiMessage midiMessage =
                                juce::MidiMessage::noteOn(1, midiNote, velocity);
                            trackMidiBuffers[trackIdx].addEvent(midiMessage, 0);
                        }
                    }
                    else if (stepState.modifierType == '@')
                    {
                        // Elongate (@): Trigger normally with elongated release/decay
                        juce::MidiMessage midiMessage =
                            juce::MidiMessage::noteOn(1, midiNote, velocity);
                        trackMidiBuffers[trackIdx].addEvent(midiMessage, 0);
                        // Note: ADSR elongation handled in TrackComponent ADSR
                    }
                    else if (stepState.modifierType == '!')
                    {
                        // Replicate (!): Trigger and force repeat on next (modifierValue - 1) steps
                        juce::MidiMessage midiMessage =
                            juce::MidiMessage::noteOn(1, midiNote, velocity);
                        trackMidiBuffers[trackIdx].addEvent(midiMessage, 0);

                        // Also trigger on upcoming steps
                        const int repOffset = stepState.modifierValue - 1;
                        if (repOffset > 0)
                        {
                            for (int repStep = 1; repStep <= repOffset; ++repStep)
                            {
                                const int targetStep = (calculatedCurrentStep + repStep) % loopLen;
                                const float trackVolume = tracks[trackIdx]->getVolume();
                                const int trackPitch = tracks[trackIdx]->getPitch();
                                const juce::uint8 repVelocity = static_cast<juce::uint8>(trackVolume * 127);
                                const int midiNoteRep = 60 + trackPitch;

                                // Calculate when in the future this trigger should happen
                                const int samplesInFuture = repStep * samplesPerStep;
                                if (samplesInFuture < bufferToFill.numSamples)
                                {
                                    juce::MidiMessage repMessage =
                                        juce::MidiMessage::noteOn(1, midiNoteRep, repVelocity);
                                    trackMidiBuffers[trackIdx].addEvent(repMessage, samplesInFuture);
                                }
                            }
                        }
                    }
                    else
                    {
                        // Normal: Single trigger
                        juce::MidiMessage midiMessage =
                            juce::MidiMessage::noteOn(1, midiNote, velocity);
                        trackMidiBuffers[trackIdx].addEvent(midiMessage, 0);
                    }
                }
            }

            lastPlayedStep = calculatedCurrentStep;
        }
    }

    // Render each track with its own MIDI buffer only
    // Process through DSP filters
    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        // Create temporary buffer for this track
        juce::AudioBuffer<float> tempBuffer(
            bufferToFill.buffer->getNumChannels(),
            bufferToFill.numSamples);
        tempBuffer.clear();

        // Render synth to temp buffer
        tracks[trackIdx]->getSynthesiser().renderNextBlock(tempBuffer,
            trackMidiBuffers[trackIdx], 0, bufferToFill.numSamples);

        // Apply DSP filter to temp buffer
        tracks[trackIdx]->processAudioBlock(tempBuffer);

        // Add to main output buffer
        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            bufferToFill.buffer->addFrom(channel, 0, tempBuffer, channel, 0, bufferToFill.numSamples);
        }
    }
}

void MainComponent::releaseResources()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(15);

    auto controlArea = area.removeFromTop(60);

    auto buttonArea = controlArea.removeFromLeft(150);
    playButton.setBounds(buttonArea.removeFromTop(30).reduced(2, 2));
    stopButton.setBounds(buttonArea.removeFromTop(30).reduced(2, 2));

    auto folderArea = controlArea.removeFromLeft(200);
    setFolderButton.setBounds(folderArea.removeFromTop(30).reduced(2, 2));
    folderLabel.setBounds(folderArea.removeFromTop(30).reduced(2, 2));

    auto sliderArea = controlArea;
    bpmSlider.setBounds(sliderArea.removeFromLeft(120).reduced(5, 10));
    sliderArea.removeFromLeft(5);
    loopLengthComboBox.setBounds(sliderArea.removeFromLeft(100).reduced(5, 10));
    sliderArea.removeFromLeft(5);
    loopLengthLabel.setBounds(sliderArea.removeFromLeft(35).reduced(2, 10));

    const int trackHeight = (area.getHeight() - (numTracks - 1) * 10) / numTracks;
    for (int i = 0; i < numTracks; ++i)
    {
        tracks[i]->setBounds(area.removeFromTop(trackHeight));
        if (i < numTracks - 1)
            area.removeFromTop(10);
    }
}

void MainComponent::timerCallback()
{
    updatePlayhead();
}

void MainComponent::updatePlayhead()
{
    const int steps = samplesPerStep.load();
    const int loopLen = loopLength.load();
    const int step = (steps > 0) ? (int)(samplePosition.load() / steps) % loopLen : 0;

    for (auto& track : tracks)
    {
        track->updatePlayhead(step, isPlaying);
    }
}

void MainComponent::togglePlay()
{
    isPlaying = !isPlaying;
    playButton.setButtonText(isPlaying ? "Pause" : "Play");

    if (!isPlaying)
    {
        // Reset global loop counter when stopping
        globalLoopCounter = 0;

        // Send allNotesOff to stop any currently playing sounds
        for (auto& track : tracks)
        {
            track->getSynthesiser().allNotesOff(0, false);
        }

        samplePosition.store(0);
        lastPlayedStep = -1;
        updatePlayhead();
    }
}

void MainComponent::stopPlayback()
{
    isPlaying = false;

    // Reset global loop counter when stopping
    globalLoopCounter = 0;

    // Send allNotesOff to stop any currently playing sounds
    for (auto& track : tracks)
    {
        track->getSynthesiser().allNotesOff(0, false);
    }

    samplePosition.store(0);
    lastPlayedStep = -1;
    playButton.setButtonText("Play");
    updatePlayhead();
}

void MainComponent::calculateSamplesPerStep()
{
    const double bpmValue = bpm.load();
    const double secondsPerBeat = 60.0 / bpmValue;
    const double secondsPerStep = secondsPerBeat / 4.0;
    samplesPerStep.store(static_cast<int>(currentSampleRate * secondsPerStep));
}

void MainComponent::openFolderChooser()
{
    chooser = std::make_unique<juce::FileChooser>("Select SuperDirt Samples Folder",
                                                  juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));

    auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

    chooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& fc)
    {
        if (fc.getResults().size() > 0)
        {
            sampleDirectory = fc.getResult();
            scanSampleDirectory(sampleDirectory);
            folderLabel.setText(sampleDirectory.getFileName(), juce::dontSendNotification);
            setFolderButton.setButtonText("Change Folder");
        }
        chooser.reset();
    });
}

void MainComponent::autoDetectSampleDirectory()
{
    // Get the directory where the executable resides
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    // Try multiple search locations
    juce::StringArray searchDirs = {
        "Dirt-Samples-master",
        "Dirt-Samples",
        "samples"
    };

    // Try current directory first
    for (const auto& dirName : searchDirs)
    {
        juce::File candidate = exeDir.getChildFile(dirName);
        if (candidate.exists() && candidate.isDirectory())
        {
            sampleDirectory = candidate;
            scanSampleDirectory(sampleDirectory);
            folderLabel.setText(sampleDirectory.getFileName() + " (Auto)", juce::dontSendNotification);
            setFolderButton.setButtonText("Change Folder");
            return;
        }
    }

    // Try two directories up (for build/Release/ structure)
    juce::File parentDir = exeDir.getParentDirectory();
    if (parentDir.exists())
    {
        for (const auto& dirName : searchDirs)
        {
            juce::File candidate = parentDir.getChildFile(dirName);
            if (candidate.exists() && candidate.isDirectory())
            {
                sampleDirectory = candidate;
                scanSampleDirectory(sampleDirectory);
                folderLabel.setText(sampleDirectory.getFileName() + " (Auto)", juce::dontSendNotification);
                setFolderButton.setButtonText("Change Folder");
                return;
            }
        }
    }

    // Try three directories up
    juce::File grandParentDir = parentDir.getParentDirectory();
    if (grandParentDir.exists())
    {
        for (const auto& dirName : searchDirs)
        {
            juce::File candidate = grandParentDir.getChildFile(dirName);
            if (candidate.exists() && candidate.isDirectory())
            {
                sampleDirectory = candidate;
                scanSampleDirectory(sampleDirectory);
                folderLabel.setText(sampleDirectory.getFileName() + " (Auto)", juce::dontSendNotification);
                setFolderButton.setButtonText("Change Folder");
                return;
            }
        }
    }

    // No sample directory found - keep manual button as fallback
    folderLabel.setText("No folder found - Click 'Set SuperDirt Folder'", juce::dontSendNotification);
}

void MainComponent::scanSampleDirectory(const juce::File& directory)
{
    juce::ScopedLock lock(directoryLock);

    sampleCategories.clear();
    juce::Array<juce::File> subdirectories;
    directory.findChildFiles(subdirectories, juce::File::findDirectories, false);

    for (auto& dir : subdirectories)
    {
        sampleCategories.add(dir.getFileName());
    }

    for (auto& track : tracks)
    {
        track->setSampleCategories(sampleCategories);
    }
}

void MainComponent::loadPendingSamples()
{
    juce::ScopedLock lock(pendingLoadLock);

    for (auto& pending : pendingSampleLoads)
    {
        if (pending.trackIndex >= 0 && pending.trackIndex < numTracks)
        {
            tracks[pending.trackIndex]->loadSampleForCategory(pending.category, sampleDirectory);
        }
    }
    pendingSampleLoads.clear();
}
