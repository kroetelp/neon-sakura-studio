#include "MainComponent.h"
#include "TrackManager.h"
#include "SampleManager.h"
#include "PlaybackController.h"
#include "PanelManager.h"
#include "AudioEngine.h"
#include "PatternGenerator.h"
#include "RhythmExplorer.h"
#include "MelodyPanel.h"
#include "TrackComponent.h"
#include "WavetableSynth/WavetableData.h"

MainComponent::MainComponent()
{
    // --- NEU: Aktiviere unser eigenes Design für das gesamte Hauptfenster ---
    setLookAndFeel(&customLookAndFeel);

    // Register audio formats first
    formatManager.registerBasicFormats();

    // Initialize managers
    initializeManagers();

    // Initialize UI
    initializeUI();

    // Connect callbacks
    connectTrackCallbacks();
    connectPanelCallbacks();
    connectUICallbacks();

    // Auto-detect sample directory
    juce::File detectedDir = sampleManager->autoDetectSampleDirectory();
    if (detectedDir.exists())
    {
        folderLabel.setText(detectedDir.getFileName() + " (Auto)", juce::dontSendNotification);
        setFolderButton.setButtonText("Change Folder");
    }

    // Set audio channels
    setAudioChannels(2, 2);

    // Start GUI timer for playhead updates
    startTimerHz(15);

    // Set window size
    setSize(2400, 1300);
}

MainComponent::~MainComponent()
{
    // --- NEU: Deaktiviere das Design, bevor die Komponente zerstört wird ---
    setLookAndFeel(nullptr);

    shutdownAudio();
}

void MainComponent::initializeManagers()
{
    // 1. Create SampleManager
    sampleManager = std::make_unique<SampleManager>();

    // 2. Create TrackManager
    trackManager = std::make_unique<TrackManager>(formatManager);

    // 3. Create PlaybackController
    playbackController = std::make_unique<PlaybackController>();

    // 4. Create AudioEngine with interfaces
    audioEngine = std::make_unique<AudioEngine>(
        trackManager.get(),
        playbackController.get()
    );

    // 5. Create PanelManager
    panelManager = std::make_unique<PanelManager>();

    // 6. Create PatternGenerator
    patternGenerator = std::make_unique<PatternGenerator>(
        *trackManager,
        *sampleManager,
        [this](double newBpm) {
            bpmSlider.setValue(newBpm, juce::dontSendNotification);
            audioEngine->setBPM(newBpm);
        }
    );
}

void MainComponent::initializeUI()
{
    // Play button
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, getNeonPink());
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(playButton);

    // Stop button
    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, getNeonCyan());
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(stopButton);

    // Set folder button
    setFolderButton.setButtonText("Set SuperDirt Folder");
    setFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 50, 70));
    setFolderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(setFolderButton);

    // Folder label
    folderLabel.setText("No folder selected", juce::dontSendNotification);
    folderLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(folderLabel);

    // Clear All button
    clearAllButton.setButtonText("Clear All");
    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 30, 30));
    clearAllButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(clearAllButton);

    // Audio Settings button
    audioSettingsButton.setButtonText("Audio");
    audioSettingsButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    addAndMakeVisible(audioSettingsButton);

    // BPM slider
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 36);
    bpmSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    bpmSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bpmSlider);

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

    masterVolumeLabel.setText("Master", juce::dontSendNotification);
    masterVolumeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(masterVolumeLabel);
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

    loopLengthLabel.setText("Loop", juce::dontSendNotification);
    loopLengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(loopLengthLabel);

    // Genre combo box
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

    // Swing slider
    swingSlider.setRange(0.0, 0.75, 0.01);
    swingSlider.setValue(0.0);
    swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 28);
    swingSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    swingSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(swingSlider);

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

    // Melody Workstation toggle button
    melodyWorkstationButton.setButtonText("Melody WS");
    melodyWorkstationButton.setColour(juce::TextButton::buttonColourId, getNeonPink().withAlpha(0.7f));
    melodyWorkstationButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    melodyWorkstationButton.setClickingTogglesState(true);
    addAndMakeVisible(melodyWorkstationButton);

    // Wavetable Synth toggle button
    wavetableSynthButton.setButtonText("Wavetable Synth");
    wavetableSynthButton.setColour(juce::TextButton::buttonColourId, getNeonPurple().withAlpha(0.7f));
    wavetableSynthButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    wavetableSynthButton.setClickingTogglesState(true);
    addAndMakeVisible(wavetableSynthButton);
}

void MainComponent::connectTrackCallbacks()
{
    // Connect track callbacks
    trackManager->forEachTrack([this](int i, TrackComponent& track) {
        addAndMakeVisible(&track);

        // Category selection callback
        track.getComboBox().onChange = [this, i] {
            selectedTrackForRhythm = i;

            if (panelManager)
            {
                panelManager->getRhythmExplorer().setTargetTrack(i);
                panelManager->getMelodyPanel().setTargetTrack(i);
            }

            // Load sample
            juce::String category = trackManager->getTrack(i).getSelectedCategory();
            if (category.isNotEmpty() && sampleManager->getSampleDirectory().exists())
            {
                sampleManager->loadSampleForCategory(i, category);
            }
        };

        // Collapse callback
        track.onStateChange = [this] { resized(); };

        // Wavetable editor callback
        track.onOpenWavetableEditor = [this](int trackIndex, std::shared_ptr<WavetableParams> params, std::shared_ptr<WavetableData> wavetable) {
            panelManager->openTrackWavetableEditor(trackIndex, params, wavetable);
        };
    });

    // Set sample categories on all tracks
    trackManager->forEachTrack([this](int, TrackComponent& track) {
        track.setSampleCategories(sampleManager->getSampleCategories());
    });

    // Connect sample manager callback to update tracks
    sampleManager->onCategoriesChanged = [this](const juce::StringArray& categories) {
        trackManager->forEachTrack([&categories](int, TrackComponent& track) {
            track.setSampleCategories(categories);
        });
    };

    // Connect sample load callback
    sampleManager->setSampleLoadCallback([this](int trackIndex, const juce::String& category, const juce::File& directory) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            trackManager->getTrack(trackIndex).loadSampleForCategory(category, directory);
        }
    });
}

void MainComponent::connectPanelCallbacks()
{
    // Rhythm Explorer callbacks
    panelManager->getRhythmExplorer().onApplyPattern = [this](int trackIndex, const std::vector<int>& steps, bool clearFirst) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            if (clearFirst)
                trackManager->getTrack(trackIndex).clearAllSteps();

            for (int step : steps)
            {
                trackManager->getTrack(trackIndex).setStepActive(step, true);
            }
        }
    };

    panelManager->getRhythmExplorer().onApplyFill = [this](int trackIndex, const std::vector<int>& steps) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            for (int step : steps)
            {
                if (step >= 0 && step < 64)
                    trackManager->getTrack(trackIndex).setStepActive(step, true);
            }
        }
    };

    // Melody Panel callbacks
    panelManager->getMelodyPanel().onApplyMelody = [this](int trackIndex, const std::vector<std::pair<int, int>>& stepPitches) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            trackManager->getTrack(trackIndex).clearAllSteps();

            for (const auto& [step, pitchOffset] : stepPitches)
            {
                if (step >= 0 && step < 64)
                {
                    StepModifierState state;
                    state.active = true;
                    state.hasPitchLock = (pitchOffset != 0);
                    state.pitchLock = pitchOffset;
                    trackManager->getTrack(trackIndex).setStepState(step, state);
                }
            }
        }
    };
}

void MainComponent::connectUICallbacks()
{
    // Play button
    playButton.onClick = [this] { togglePlay(); };

    // Stop button
    stopButton.onClick = [this] { stopPlayback(); };

    // Set folder button
    setFolderButton.onClick = [this] { openFolderChooser(); };

    // Clear All button
    clearAllButton.onClick = [this] {
        trackManager->clearAllTracks();
    };

    // Audio settings button
    audioSettingsButton.onClick = [this] { showAudioSettingsDialog(); };

    // BPM slider
    bpmSlider.onValueChange = [this] {
        playbackController->setBPM(bpmSlider.getValue());
        audioEngine->setBPM(bpmSlider.getValue());
    };

    // Master volume slider
    masterVolumeSlider.onValueChange = [this] {
        audioEngine->setMasterVolume(static_cast<float>(masterVolumeSlider.getValue()));
    };

    // Loop length combo box
    loopLengthComboBox.onChange = [this] {
        audioEngine->setLoopLength(loopLengthComboBox.getSelectedId());
    };

    // Generate button
    generateButton.onClick = [this] {
        if (audioEngine->isPlaying())
            togglePlay();
        patternGenerator->generateSong(static_cast<PatternGenerator::Genre>(genreComboBox.getSelectedId()));
    };

    // Swing slider
    swingSlider.onValueChange = [this] {
        audioEngine->setSwingAmount(static_cast<float>(swingSlider.getValue()));
    };

    // Reverb slider
    reverbSlider.onValueChange = [this] {
        audioEngine->setReverbWetLevel(static_cast<float>(reverbSlider.getValue()));
    };

    // Rhythm Explorer button
    rhythmExplorerButton.onClick = [this] {
        bool visible = rhythmExplorerButton.getToggleState();
        panelManager->setRhythmExplorerVisible(visible);

        if (visible)
        {
            addAndMakeVisible(panelManager->getRhythmExplorerComponent());
        }
        else
        {
            removeChildComponent(panelManager->getRhythmExplorerComponent());
        }
        resized();
    };

    // Melody Workstation button
    melodyWorkstationButton.onClick = [this] {
        bool visible = melodyWorkstationButton.getToggleState();
        panelManager->setMelodyPanelVisible(visible);

        if (visible)
        {
            addAndMakeVisible(panelManager->getMelodyPanelComponent());
        }
        else
        {
            removeChildComponent(panelManager->getMelodyPanelComponent());
        }
        resized();
    };

    // Wavetable Synth button
    wavetableSynthButton.onClick = [this] {
        bool visible = wavetableSynthButton.getToggleState();
        panelManager->setWavetableSynthVisible(visible, &audioEngine->getWavetableEngine());
    };
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    // Prepare TrackManager audio
    trackManager->prepareAudio(sampleRate, samplesPerBlockExpected);

    // Prepare AudioEngine
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
    isResizing = true;

    auto bounds = getLocalBounds();

    if (bounds.getWidth() <= 100 || bounds.getHeight() <= 100)
    {
        isResizing = false;
        return;
    }

    auto area = bounds.reduced(10);

    // Reserve space for side panels
    const int rhythmExplorerWidth = 280;
    const int melodyPanelWidth = 350;
    juce::Rectangle<int> rhythmExplorerArea;
    juce::Rectangle<int> melodyPanelArea;

    if (panelManager->isRhythmExplorerVisible() && panelManager->isMelodyPanelVisible())
    {
        int totalWidth = rhythmExplorerWidth + melodyPanelWidth + 20;
        auto rightArea = area.removeFromRight(totalWidth);
        melodyPanelArea = rightArea.removeFromRight(melodyPanelWidth);
        rightArea.removeFromRight(10);
        rhythmExplorerArea = rightArea;
    }
    else if (panelManager->isRhythmExplorerVisible())
    {
        rhythmExplorerArea = area.removeFromRight(rhythmExplorerWidth);
        area.removeFromRight(10);
    }
    else if (panelManager->isMelodyPanelVisible())
    {
        melodyPanelArea = area.removeFromRight(melodyPanelWidth);
        area.removeFromRight(10);
    }

    // Control area - TWO ROWS
    const int controlHeight = 90;
    auto controlArea = area.removeFromTop(controlHeight);

    auto topRow = controlArea.removeFromTop(45);
    auto bottomRow = controlArea;

    // TOP ROW: Play, Stop, Clear, Folder, BPM, Master
    const int rowY = 5;
    const int btnHeight = 35;
    const int sliderHeight = 30;
    int xPos = 10;

    playButton.setBounds(xPos, rowY, 70, btnHeight);
    xPos += 75;
    stopButton.setBounds(xPos, rowY, 70, btnHeight);
    xPos += 75;
    clearAllButton.setBounds(xPos, rowY, 80, btnHeight);
    xPos += 85;
    setFolderButton.setBounds(xPos, rowY, 160, btnHeight);
    xPos += 165;
    folderLabel.setBounds(xPos, rowY + 8, 150, 20);
    xPos += 160;
    bpmLabel.setBounds(xPos, rowY + 5, 35, 20);
    xPos += 35;
    bpmSlider.setBounds(xPos, rowY + 2, 140, sliderHeight);
    xPos += 150;
    masterVolumeLabel.setBounds(xPos, rowY + 5, 50, 20);
    xPos += 50;
    masterVolumeSlider.setBounds(xPos, rowY + 2, 120, sliderHeight);
    xPos += 130;
    audioSettingsButton.setBounds(xPos, rowY, 60, btnHeight);

    // BOTTOM ROW: Loop, Genre, Generate, Swing, Reverb, Panels
    xPos = 10;
    const int bottomY = 50;

    loopLengthLabel.setBounds(xPos, bottomY + 5, 35, 20);
    xPos += 35;
    loopLengthComboBox.setBounds(xPos, bottomY + 2, 100, sliderHeight);
    xPos += 110;
    genreComboBox.setBounds(xPos, bottomY + 2, 120, sliderHeight);
    xPos += 130;
    generateButton.setBounds(xPos, bottomY, 130, btnHeight);
    xPos += 140;
    swingLabel.setBounds(xPos, bottomY + 5, 40, 20);
    xPos += 40;
    swingSlider.setBounds(xPos, bottomY + 2, 120, sliderHeight);
    xPos += 130;
    reverbLabel.setBounds(xPos, bottomY + 5, 45, 20);
    xPos += 45;
    reverbSlider.setBounds(xPos, bottomY + 2, 120, sliderHeight);

    xPos += 130;
    if (xPos + 120 < area.getWidth())
        rhythmExplorerButton.setBounds(xPos, bottomY, 120, btnHeight);

    xPos += 125;
    if (xPos + 90 < area.getWidth())
        melodyWorkstationButton.setBounds(xPos, bottomY, 90, btnHeight);

    xPos += 95;
    if (xPos + 110 < area.getWidth())
        wavetableSynthButton.setBounds(xPos, bottomY, 110, btnHeight);

    // DYNAMIC TRACK HEIGHTS
    const int trackGap = 5;
    const int expandedHeight = 165;
    const int collapsedHeight = 45;

    trackManager->forEachTrack([&](int i, TrackComponent& track) {
        const int trackHeight = track.getIsExpanded() ? expandedHeight : collapsedHeight;
        track.setBounds(area.removeFromTop(trackHeight));
        area.removeFromTop(trackGap);
    });

    // SIDE PANELS
    if (panelManager->isRhythmExplorerVisible() && rhythmExplorerArea.getWidth() > 0)
    {
        panelManager->getRhythmExplorerComponent()->setBounds(rhythmExplorerArea.reduced(5));
    }

    if (panelManager->isMelodyPanelVisible() && melodyPanelArea.getWidth() > 0)
    {
        panelManager->getMelodyPanelComponent()->setBounds(melodyPanelArea.reduced(5));
    }

    isResizing = false;
}

void MainComponent::timerCallback()
{
    if (!isResizing)
        updatePlayhead();
}

void MainComponent::updatePlayhead()
{
    const int steps = audioEngine->getSamplesPerStep();

    trackManager->forEachTrack([&](int i, TrackComponent& track) {
        const int trackLoopLen = track.getTrackLoopLength();
        const int step = (steps > 0) ? (int)(audioEngine->getSamplePosition() / steps) % trackLoopLen : 0;
        track.updatePlayhead(step, audioEngine->isPlaying());
    });
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
            juce::File directory = fc.getResult();
            sampleManager->scanSampleDirectory(directory);
            folderLabel.setText(directory.getFileName(), juce::dontSendNotification);
            setFolderButton.setButtonText("Change Folder");
        }
        chooser.reset();
    });
}

void MainComponent::showAudioSettingsDialog()
{
    auto* deviceSelector = new juce::AudioDeviceSelectorComponent(
        deviceManager,
        0, 2,
        0, 2,
        false, false, false, false
    );

    deviceSelector->setSize(450, 280);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(deviceSelector);
    options.dialogTitle = "Audio Settings - Select Output Device";
    options.dialogBackgroundColour = getDarkBackground();
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}