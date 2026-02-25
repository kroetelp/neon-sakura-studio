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

        tracks[i]->getComboBox().onChange = [this, i] {
            juce::String category = tracks[i]->getSelectedCategory();
            if (category.isNotEmpty() && sampleDirectory.exists())
            {
                tracks[i]->loadSampleForCategory(category, sampleDirectory);
            }
        };

        // Bind collapse callback to trigger resized()
        tracks[i]->onStateChange = [this] { resized(); };

        // Bind wavetable editor callback
        tracks[i]->onOpenWavetableEditor = [this](int trackIndex, std::shared_ptr<WavetableParams> params) {
            openTrackWavetableEditor(trackIndex, params);
        };
    }

    // Auto-detect sample directory on startup
    autoDetectSampleDirectory();

    // Create AudioEngine with track references
    audioEngine = std::make_unique<AudioEngine>(tracks);

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
    addAndMakeVisible(folderLabel);

    // Clear All button
    clearAllButton.setButtonText("Clear All");
    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 30, 30));
    clearAllButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(clearAllButton);
    clearAllButton.onClick = [this] {
        for (int i = 0; i < numTracks; ++i)
        {
            tracks[i]->clearAllSteps();
            tracks[i]->setMuted(false);
            tracks[i]->setSolo(false);
        }
    };

    // Audio Settings button - opens device selector dialog
    audioSettingsButton.setButtonText("Audio");
    audioSettingsButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    addAndMakeVisible(audioSettingsButton);
    audioSettingsButton.onClick = [this] {
        showAudioSettingsDialog();
    };

    // BPM slider
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 36);
    bpmSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    bpmSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bpmSlider);
    bpmSlider.onValueChange = [this] {
        bpm.store(bpmSlider.getValue());
        audioEngine->setBPM(bpmSlider.getValue());
    };

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(bpmLabel);
    bpmLabel.attachToComponent(&bpmSlider, true);

    // Master volume slider
    masterVolumeSlider.setRange(0.0, 1.0, 0.01);
    masterVolumeSlider.setValue(0.8);
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 36);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    masterVolumeSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(masterVolumeSlider);
    masterVolumeSlider.onValueChange = [this] {
        audioEngine->setMasterVolume(static_cast<float>(masterVolumeSlider.getValue()));
    };

    addAndMakeVisible(masterVolumeLabel);
    masterVolumeLabel.setText("Master", juce::dontSendNotification);
    masterVolumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterVolumeLabel.attachToComponent(&masterVolumeSlider, true);

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
        audioEngine->setLoopLength(loopLengthComboBox.getSelectedId());
    };

    addAndMakeVisible(loopLengthLabel);
    loopLengthLabel.setText("Loop", juce::dontSendNotification);
    loopLengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Genre combo box for algorithmic generation
    genreComboBox.addItem("Techno", 1);
    genreComboBox.addItem("House", 2);
    genreComboBox.addItem("Trap", 3);
    genreComboBox.addItem("DnB", 4);
    genreComboBox.addItem("Ambient", 5);
    genreComboBox.addItem("Garage", 6);
    genreComboBox.setSelectedId(1);
    genreComboBox.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    genreComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    genreComboBox.setColour(juce::ComboBox::arrowColourId, getNeonPink());
    addAndMakeVisible(genreComboBox);

    // Generate button
    generateButton.setButtonText("Generate");
    generateButton.setColour(juce::TextButton::buttonColourId, getNeonPink());
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(generateButton);
    generateButton.onClick = [this] {
        if (audioEngine->isPlaying())
            togglePlay();
        patternGenerator->generateSong(static_cast<PatternGenerator::Genre>(genreComboBox.getSelectedId()));
    };

    // Swing slider
    swingSlider.setRange(0.0, 0.75, 0.01);
    swingSlider.setValue(0.0);
    swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 28);
    swingSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    swingSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(swingSlider);
    swingSlider.onValueChange = [this] {
        audioEngine->setSwingAmount(static_cast<float>(swingSlider.getValue()));
    };

    addAndMakeVisible(swingLabel);
    swingLabel.setText("Swing", juce::dontSendNotification);
    swingLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(swingLabel);
    swingLabel.attachToComponent(&swingSlider, true);

    // Reverb slider
    reverbSlider.setRange(0.0, 1.0, 0.01);
    reverbSlider.setValue(0.3);
    reverbSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 28);
    reverbSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    reverbSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(reverbSlider);
    reverbSlider.onValueChange = [this] {
        audioEngine->setReverbWetLevel(static_cast<float>(reverbSlider.getValue()));
    };

    addAndMakeVisible(reverbLabel);
    reverbLabel.setText("Reverb", juce::dontSendNotification);
    reverbLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(reverbLabel);
    reverbLabel.attachToComponent(&reverbSlider, true);

    // Rhythm Explorer toggle button
    rhythmExplorerButton.setButtonText("Rhythm Explorer");
    rhythmExplorerButton.setColour(juce::TextButton::buttonColourId, getNeonPink().withAlpha(0.7f));
    rhythmExplorerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    rhythmExplorerButton.setClickingTogglesState(true);
    addAndMakeVisible(rhythmExplorerButton);
    rhythmExplorerButton.onClick = [this] {
        rhythmExplorerVisible = rhythmExplorerButton.getToggleState();
        if (rhythmExplorerVisible)
        {
            rhythmExplorer->setVisible(true);
            addAndMakeVisible(rhythmExplorer.get());
        }
        else
        {
            rhythmExplorer->setVisible(false);
            removeChildComponent(rhythmExplorer.get());
        }
        resized();
    };

    // Create Rhythm Explorer panel
    rhythmExplorer = std::make_unique<RhythmExplorer>();
    rhythmExplorer->setVisible(false);

    // Connect RhythmExplorer to tracks
    rhythmExplorer->onApplyPattern = [this](int trackIndex, const std::vector<int>& steps, bool clearFirst) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            if (clearFirst)
                tracks[trackIndex]->clearAllSteps();

            for (int step : steps)
            {
                tracks[trackIndex]->setStepActive(step, true);
            }
        }
    };

    rhythmExplorer->onApplyFill = [this](int trackIndex, const std::vector<int>& steps) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            // Apply fill without clearing existing pattern
            for (int step : steps)
            {
                if (step >= 0 && step < 64)
                    tracks[trackIndex]->setStepActive(step, true);
            }
        }
    };

    // Track selection - click on track's combo box to select for RhythmExplorer
    for (int i = 0; i < numTracks; ++i)
    {
        // Store capture by value
        const int capturedIdx = i;
        tracks[i]->getComboBox().onChange = [this, capturedIdx] {
            selectedTrackForRhythm = capturedIdx;
            if (rhythmExplorer)
                rhythmExplorer->setTargetTrack(capturedIdx);
            if (melodyPanel)
                melodyPanel->setTargetTrack(capturedIdx);

            // Also load the sample
            juce::String category = tracks[capturedIdx]->getSelectedCategory();
            if (category.isNotEmpty() && sampleDirectory.exists())
            {
                tracks[capturedIdx]->loadSampleForCategory(category, sampleDirectory);
            }
        };
    }

    // Melody Workstation toggle button
    melodyWorkstationButton.setButtonText("Melody WS");
    melodyWorkstationButton.setColour(juce::TextButton::buttonColourId, getNeonPink().withAlpha(0.7f));
    melodyWorkstationButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    melodyWorkstationButton.setClickingTogglesState(true);
    addAndMakeVisible(melodyWorkstationButton);
    melodyWorkstationButton.onClick = [this] {
        melodyPanelVisible = melodyWorkstationButton.getToggleState();
        if (melodyPanelVisible)
        {
            melodyPanel->setVisible(true);
            addAndMakeVisible(melodyPanel.get());
        }
        else
        {
            melodyPanel->setVisible(false);
            removeChildComponent(melodyPanel.get());
        }
        resized();
    };

    // Create Melody Workstation panel
    melodyPanel = std::make_unique<MelodyPanel>();
    melodyPanel->setVisible(false);

    // Connect MelodyPanel to tracks
    melodyPanel->onApplyMelody = [this](int trackIndex, const std::vector<std::pair<int, int>>& stepPitches) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            // Clear existing steps first
            tracks[trackIndex]->clearAllSteps();

            // Apply melody steps with pitch offsets
            for (const auto& [step, pitchOffset] : stepPitches)
            {
                if (step >= 0 && step < 64)
                {
                    StepModifierState state;
                    state.active = true;
                    state.hasPitchLock = (pitchOffset != 0);
                    state.pitchLock = pitchOffset;
                    tracks[trackIndex]->setStepState(step, state);
                }
            }
        }
    };

    // Wavetable Synth toggle button (opens in separate window)
    wavetableSynthButton.setButtonText("Wavetable Synth");
    wavetableSynthButton.setColour(juce::TextButton::buttonColourId, juce::Colour(180, 0, 255).withAlpha(0.7f));
    wavetableSynthButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    wavetableSynthButton.setClickingTogglesState(true);
    addAndMakeVisible(wavetableSynthButton);
    wavetableSynthButton.onClick = [this] {
        wavetableSynthVisible = wavetableSynthButton.getToggleState();

        if (wavetableSynthVisible)
        {
            if (!wavetableSynthEditor)
            {
                // Create editor with reference to AudioEngine's WavetableEngine
                wavetableSynthEditor = std::make_unique<WavetableSynthEditor>(audioEngine->getWavetableEngine());
            }

            if (!wavetableSynthWindow)
            {
                wavetableSynthWindow = std::make_unique<juce::DocumentWindow>(
                    "Wavetable Synth",
                    getDarkBackground(),
                    juce::DocumentWindow::allButtons,
                    true
                );
                wavetableSynthWindow->setContentOwned(wavetableSynthEditor.get(), true);
                wavetableSynthWindow->setCentrePosition(600, 400);
                wavetableSynthWindow->setSize(1050, 750);
            }

            wavetableSynthWindow->setVisible(true);
            wavetableSynthWindow->toFront(true);
        }
        else
        {
            if (wavetableSynthWindow)
            {
                wavetableSynthWindow->setVisible(false);
            }
        }
    };

    // Start GUI timer for playhead updates (15 FPS - sufficient for visual feedback)
    startTimerHz(15);

    // Initialize pattern generator
    patternGenerator = std::make_unique<PatternGenerator>(
        tracks,
        sampleCategories,
        sampleDirectory,
        bpm,
        bpmSlider,
        [this] {
            bpm.store(bpmSlider.getValue());
            audioEngine->setBPM(bpmSlider.getValue());
        }
    );

    // Set window size optimized for 1440p (2560x1440)
    // Using large size that fills most of screen
    setSize(2400, 1300);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    audioEngine->prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    audioEngine->getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    audioEngine->releaseResources();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());
}

void MainComponent::resized()
{
    // Set flag to prevent timer updates during resize
    isResizing = true;

    auto bounds = getLocalBounds();

    // Early return for invalid bounds
    if (bounds.getWidth() <= 100 || bounds.getHeight() <= 100)
    {
        isResizing = false;
        return;
    }

    auto area = bounds.reduced(10);

    // Reserve space for side panels (RhythmExplorer and/or MelodyPanel)
    const int rhythmExplorerWidth = 280;
    const int melodyPanelWidth = 350;
    juce::Rectangle<int> rhythmExplorerArea;
    juce::Rectangle<int> melodyPanelArea;

    if (rhythmExplorerVisible && melodyPanelVisible)
    {
        // Both visible - split the right side
        int totalWidth = rhythmExplorerWidth + melodyPanelWidth + 20;
        auto rightArea = area.removeFromRight(totalWidth);
        melodyPanelArea = rightArea.removeFromRight(melodyPanelWidth);
        rightArea.removeFromRight(10); // Gap
        rhythmExplorerArea = rightArea;
    }
    else if (rhythmExplorerVisible)
    {
        rhythmExplorerArea = area.removeFromRight(rhythmExplorerWidth);
        area.removeFromRight(10); // Gap
    }
    else if (melodyPanelVisible)
    {
        melodyPanelArea = area.removeFromRight(melodyPanelWidth);
        area.removeFromRight(10); // Gap
    }

    // Control area - TWO ROWS for better spacing
    const int controlHeight = 90;
    auto controlArea = area.removeFromTop(controlHeight);

    // Split into two rows
    auto topRow = controlArea.removeFromTop(45);
    auto bottomRow = controlArea;

    // === TOP ROW: Play, Stop, Clear, Folder, BPM, Master ===
    const int rowY = 5;
    const int btnHeight = 35;
    const int sliderHeight = 30;
    int xPos = 10;

    // Play/Stop buttons
    playButton.setBounds(xPos, rowY, 70, btnHeight);
    xPos += 75;
    stopButton.setBounds(xPos, rowY, 70, btnHeight);
    xPos += 75;

    // Clear All button
    clearAllButton.setBounds(xPos, rowY, 80, btnHeight);
    xPos += 85;

    // Folder button and label
    setFolderButton.setBounds(xPos, rowY, 160, btnHeight);
    xPos += 165;
    folderLabel.setBounds(xPos, rowY + 8, 150, 20);
    xPos += 160;

    // BPM slider with generous width
    bpmLabel.setBounds(xPos, rowY + 5, 35, 20);
    xPos += 35;
    bpmSlider.setBounds(xPos, rowY + 2, 140, sliderHeight);
    xPos += 150;

    // Master Volume slider
    masterVolumeLabel.setBounds(xPos, rowY + 5, 50, 20);
    xPos += 50;
    masterVolumeSlider.setBounds(xPos, rowY + 2, 120, sliderHeight);

    // Audio Settings button
    xPos += 130;
    audioSettingsButton.setBounds(xPos, rowY, 60, btnHeight);

    // === BOTTOM ROW: Loop, Genre, Generate, Swing, Reverb, RhythmExplorer ===
    xPos = 10;
    const int bottomY = 50;

    // Loop length combo with label
    loopLengthLabel.setBounds(xPos, bottomY + 5, 35, 20);
    xPos += 35;
    loopLengthComboBox.setBounds(xPos, bottomY + 2, 100, sliderHeight);
    xPos += 110;

    // Genre combo
    genreComboBox.setBounds(xPos, bottomY + 2, 120, sliderHeight);
    xPos += 130;

    // Generate button
    generateButton.setBounds(xPos, bottomY, 130, btnHeight);
    xPos += 140;

    // Swing slider
    swingLabel.setBounds(xPos, bottomY + 5, 40, 20);
    xPos += 40;
    swingSlider.setBounds(xPos, bottomY + 2, 120, sliderHeight);
    xPos += 130;

    // Reverb slider
    reverbLabel.setBounds(xPos, bottomY + 5, 45, 20);
    xPos += 45;
    reverbSlider.setBounds(xPos, bottomY + 2, 120, sliderHeight);

    // Rhythm Explorer toggle button (if space allows)
    xPos += 130;
    if (xPos + 120 < area.getWidth())
    {
        rhythmExplorerButton.setBounds(xPos, bottomY, 120, btnHeight);
    }

    // Melody Workstation toggle button
    xPos += 125;
    if (xPos + 90 < area.getWidth())
    {
        melodyWorkstationButton.setBounds(xPos, bottomY, 90, btnHeight);
    }

    // Wavetable Synth toggle button
    xPos += 95;
    if (xPos + 110 < area.getWidth())
    {
        wavetableSynthButton.setBounds(xPos, bottomY, 110, btnHeight);
    }

    // === DYNAMIC TRACK HEIGHTS based on expanded state ===
    const int trackGap = 5;
    const int expandedHeight = 165;  // Header(40) + Knobs(50) + StepRow1(35) + StepRow2(35) + margin
    const int collapsedHeight = 45;

    for (int i = 0; i < numTracks; ++i)
    {
        const int trackHeight = tracks[i]->getIsExpanded() ? expandedHeight : collapsedHeight;
        tracks[i]->setBounds(area.removeFromTop(trackHeight));
        area.removeFromTop(trackGap);
    }

    // === RHYTHM EXPLORER PANEL ===
    if (rhythmExplorerVisible && rhythmExplorerArea.getWidth() > 0)
    {
        rhythmExplorer->setBounds(rhythmExplorerArea.reduced(5));
    }

    // === MELODY WORKSTATION PANEL ===
    if (melodyPanelVisible && melodyPanelArea.getWidth() > 0)
    {
        melodyPanel->setBounds(melodyPanelArea.reduced(5));
    }

    isResizing = false;
}

void MainComponent::timerCallback()
{
    // Skip updates during resize to prevent UI freeze
    if (!isResizing)
        updatePlayhead();
}

void MainComponent::updatePlayhead()
{
    const int steps = audioEngine->getSamplesPerStep();

    // Per-track playhead calculation for polyrhythms
    for (int i = 0; i < numTracks; ++i)
    {
        const int trackLoopLen = tracks[i]->getTrackLoopLength();
        const int step = (steps > 0) ? (int)(audioEngine->getSamplePosition() / steps) % trackLoopLen : 0;
        tracks[i]->updatePlayhead(step, audioEngine->isPlaying());
    }
}

void MainComponent::togglePlay()
{
    if (audioEngine->isPlaying())
    {
        audioEngine->stopPlayback();
        playButton.setButtonText("Play");
    }
    else
    {
        audioEngine->startPlayback();
        playButton.setButtonText("Pause");
    }
}

void MainComponent::stopPlayback()
{
    audioEngine->stopPlayback();
    playButton.setButtonText("Play");
    updatePlayhead();
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
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    juce::StringArray searchDirs = {
        "Dirt-Samples-master",
        "Dirt-Samples",
        "samples"
    };

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

void MainComponent::showAudioSettingsDialog()
{
    // Create audio device selector component
    auto* deviceSelector = new juce::AudioDeviceSelectorComponent(
        deviceManager,           // AudioDeviceManager from AudioAppComponent
        0,                       // min audio input channels
        2,                       // max audio input channels
        0,                       // min audio output channels
        2,                       // max audio output channels
        false,                   // show MIDI input options
        false,                   // show MIDI output options
        false,                   // show muted input options
        false                    // only show current device's input channels
    );

    deviceSelector->setSize(450, 280);

    // Create and show dialog
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(deviceSelector);
    options.dialogTitle = "Audio Settings - Select Output Device";
    options.dialogBackgroundColour = getDarkBackground();
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}

void MainComponent::openTrackWavetableEditor(int trackIndex, std::shared_ptr<WavetableParams> params)
{
    if (!params)
        return;

    currentEditingTrack = trackIndex;

    // Create or update the editor
    if (!trackWavetableEditor)
    {
        trackWavetableEditor = std::make_unique<WavetableSynthEditor>(params);
    }
    else
    {
        trackWavetableEditor->setSharedParams(params);
    }

    // Create window if needed
    if (!trackWavetableWindow)
    {
        trackWavetableWindow = std::make_unique<juce::DocumentWindow>(
            "Track " + juce::String(trackIndex + 1) + " Wavetable",
            getDarkBackground(),
            juce::DocumentWindow::allButtons,
            true
        );
        trackWavetableWindow->setContentOwned(trackWavetableEditor.get(), true);
        trackWavetableWindow->setCentrePosition(650, 400);
        trackWavetableWindow->setSize(1050, 750);
    }
    else
    {
        // Update window title for new track
        trackWavetableWindow->setName("Track " + juce::String(trackIndex + 1) + " Wavetable");
    }

    trackWavetableWindow->setVisible(true);
    trackWavetableWindow->toFront(true);
}
