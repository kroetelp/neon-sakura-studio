#include "MainComponent.h"
#include "TrackManager.h"
#include "SampleManager.h"
#include "PlaybackController.h"
#include "PanelManager.h"
#include "DockingManager.h"
#include "Timeline/TimelinePanel.h"
#include "WavetableUI/WavetablePanel.h"
#include "StepSequencer/StepSequencerPanel.h"
#include "AudioEngine.h"
#include "PatternGenerator.h"
#include "RhythmExplorer.h"
#include "MelodyPanel.h"
#include "TrackComponent.h"
#include "WavetableSynth/WavetableData.h"
#include "WavetableSynth/WavetableEngine.h"
#include "Timeline/TimelineComponent.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&customLookAndFeel);

    formatManager.registerBasicFormats();
    initializeManagers();
    initializeUI();
    initializeDockingPanels();  // NEU: Panels registrieren und layouten
    connectTrackCallbacks();
    connectPanelCallbacks();
    connectUICallbacks();

    juce::File detectedDir = sampleManager->autoDetectSampleDirectory();
    if (detectedDir.exists())
    {
        folderLabel.setText(detectedDir.getFileName() + " (Auto)", juce::dontSendNotification);
        setFolderButton.setButtonText("Change Folder");
    }

    setAudioChannels(2, 2);
    startTimerHz(15);
    setSize(1600, 900);  // Kompakter für Single-Window
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
    shutdownAudio();
}

void MainComponent::initializeManagers()
{
    sampleManager = std::make_unique<SampleManager>();
    trackManager = std::make_unique<TrackManager>(formatManager);
    playbackController = std::make_unique<PlaybackController>();

    audioEngine = std::make_unique<AudioEngine>(
        trackManager.get(),
        playbackController.get()
    );

    panelManager = std::make_unique<PanelManager>();

    // === DockingManager für Single-Window Workspace ===
    dockingManager = std::make_unique<DockingManager>();
    dockingManager->setMainComponent(this);

    patternGenerator = std::make_unique<PatternGenerator>(
        *trackManager,
        *sampleManager,
        [this](double newBpm) {
            bpmSlider.setValue(newBpm, juce::dontSendNotification);
            audioEngine->setBPM(newBpm);
        }
    );

    wootingManager = std::make_unique<WootingManager>();
    audioEngine->getWavetableEngine().setMidiEventQueue(&wootingManager->getMidiQueue());
}

void MainComponent::initializeDockingPanels()
{
    // === WavetablePanel erstellen und registrieren ===
    auto wavetablePanel = std::make_unique<WavetablePanel>();
    wavetablePanel->setEngine(&audioEngine->getWavetableEngine());
    dockingManager->registerPanel(std::move(wavetablePanel));

    // === TimelinePanel erstellen und registrieren ===
    auto timelinePanel = std::make_unique<TimelinePanel>();
    timelinePanel->setDependencies(&audioEngine->getTimelineData(),
                                    &audioEngine->getRecordingManager());
    timelinePanel->setSampleManager(sampleManager.get());
    dockingManager->registerPanel(std::move(timelinePanel));

    // === StepSequencerPanel erstellen und registrieren ===
    auto stepSequencerPanel = std::make_unique<StepSequencerPanel>();
    stepSequencerPanel->setTrackManager(trackManager.get());
    dockingManager->registerPanel(std::move(stepSequencerPanel));

    // === Stretchable Resizer Bar erstellen ===
    verticalResizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(
        &stretchableManager, resizerLayoutId, false);  // false = vertical
    addAndMakeVisible(verticalResizerBar.get());

    // === Stretchable Layout konfigurieren ===
    // Layout-Items: Synth (oben), Resizer, Tabs (unten)
    stretchableManager.setItemLayout(synthLayoutId,
                                      minPanelHeight,
                                      -1,
                                      defaultSynthHeight);

    stretchableManager.setItemLayout(resizerLayoutId,
                                      resizerBarHeight,
                                      resizerBarHeight,
                                      resizerBarHeight);

    stretchableManager.setItemLayout(bottomTabsLayoutId,
                                      minPanelHeight,
                                      -1,
                                      defaultBottomTabsHeight);

    // === Tab-Component für unteren Bereich initialisieren ===
    initializeBottomTabs();

    // === Panels SICHTBAR machen (als Docked) ===
    dockingManager->dockPanel(PanelType::WavetableSynth, DockPosition::Center);
    // Timeline und StepSequencer werden über Tabs verwaltet, nicht einzeln sichtbar
}

void MainComponent::initializeBottomTabs()
{
    // Tab-Component erstellen mit Neon Sakura Styling
    bottomTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);

    // Tab-Bar Styling
    bottomTabs->setTabBarDepth(tabBarHeight);
    bottomTabs->setColour(juce::TabbedComponent::backgroundColourId, getDarkBackground());
    bottomTabs->setColour(juce::TabbedComponent::outlineColourId, juce::Colour(40, 40, 60));

    // Tab-Buttons Styling
    auto& tabBar = bottomTabs->getTabbedButtonBar();
    tabBar.setColour(juce::TabbedButtonBar::tabOutlineColourId, juce::Colour(40, 40, 60));
    tabBar.setColour(juce::TabbedButtonBar::frontOutlineColourId, getNeonCyan());

    // Timeline Tab hinzufügen
    auto* timelinePanel = dockingManager->getPanelAs<TimelinePanel>(PanelType::Timeline);
    if (timelinePanel)
    {
        // ACHTUNG: Wir fügen das Panel NICHT als Ownership hinzu,
        // sondern zeigen nur seinen Content an.
        // Das Panel bleibt im Besitz des DockingManagers.
        bottomTabs->addTab("Timeline", getNeonCyan(), timelinePanel, false);
    }

    // Step Sequencer Tab hinzufügen
    auto* stepSequencerPanel = dockingManager->getPanelAs<StepSequencerPanel>(PanelType::StepSequencer);
    if (stepSequencerPanel)
    {
        bottomTabs->addTab("Step Sequencer", getNeonPink(), stepSequencerPanel, false);
    }

    // Standardmäßig Timeline Tab anzeigen
    bottomTabs->setCurrentTabIndex(timelineTabIndex);

    addAndMakeVisible(bottomTabs.get());
}

void MainComponent::initializeUI()
{
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, getNeonPink());
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(playButton);

    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, getNeonCyan());
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(stopButton);

    setFolderButton.setButtonText("Set SuperDirt Folder");
    setFolderButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 50, 70));
    setFolderButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(setFolderButton);

    folderLabel.setText("No folder selected", juce::dontSendNotification);
    folderLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(folderLabel);

    clearAllButton.setButtonText("Clear All");
    clearAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 30, 30));
    clearAllButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(clearAllButton);

    audioSettingsButton.setButtonText("Audio");
    audioSettingsButton.setColour(juce::TextButton::buttonColourId, getDarkBackground());
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, getNeonCyan());
    addAndMakeVisible(audioSettingsButton);

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

    drumTargetLabel.setText("Track:", juce::dontSendNotification);
    drumTargetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(drumTargetLabel);

    drumTargetTrackCombo.addItem("All Tracks", 0);
    for (int i = 0; i < numTracks; ++i)
        drumTargetTrackCombo.addItem("Track " + juce::String(i + 1), i + 1);
    drumTargetTrackCombo.setSelectedId(0);
    drumTargetTrackCombo.setColour(juce::ComboBox::backgroundColourId, getDarkBackground());
    drumTargetTrackCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    drumTargetTrackCombo.setColour(juce::ComboBox::outlineColourId, getNeonCyan());
    addAndMakeVisible(drumTargetTrackCombo);

    generateButton.setButtonText("Generate");
    generateButton.setColour(juce::TextButton::buttonColourId, getNeonPink());
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(generateButton);

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

    rhythmExplorerButton.setButtonText("Rhythm Explorer");
    rhythmExplorerButton.setColour(juce::TextButton::buttonColourId, getNeonPink().withAlpha(0.7f));
    rhythmExplorerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    rhythmExplorerButton.setClickingTogglesState(true);
    addAndMakeVisible(rhythmExplorerButton);

    melodyWorkstationButton.setButtonText("Melody WS");
    melodyWorkstationButton.setColour(juce::TextButton::buttonColourId, getNeonPink().withAlpha(0.7f));
    melodyWorkstationButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    melodyWorkstationButton.setClickingTogglesState(true);
    addAndMakeVisible(melodyWorkstationButton);

    wavetableSynthButton.setButtonText("Wavetable Synth");
    wavetableSynthButton.setColour(juce::TextButton::buttonColourId, getNeonPurple());
    wavetableSynthButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    wavetableSynthButton.setClickingTogglesState(true);
    wavetableSynthButton.setToggleState(true, juce::dontSendNotification);  // Standardmäßig AN
    addAndMakeVisible(wavetableSynthButton);

    // Timeline button - Teil des Single-Window Workspaces
    timelineButton.setButtonText("Timeline");
    timelineButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0, 100, 80));
    timelineButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    timelineButton.setClickingTogglesState(true);
    timelineButton.setToggleState(true, juce::dontSendNotification);  // Standardmäßig AN
    addAndMakeVisible(timelineButton);
}

void MainComponent::connectTrackCallbacks()
{
    trackManager->forEachTrack([this](int i, TrackComponent& track) {
        addAndMakeVisible(&track);

        track.getComboBox().onChange = [this, i] {
            selectedTrackForRhythm = i;

            if (panelManager)
            {
                panelManager->getRhythmExplorer().setTargetTrack(i);
                panelManager->getMelodyPanel().setTargetTrack(i);
            }

            juce::String category = trackManager->getTrack(i).getSelectedCategory();
            if (category.isNotEmpty() && sampleManager->getSampleDirectory().exists())
            {
                sampleManager->loadSampleForCategory(i, category);
            }
        };

        track.onStateChange = [this] { resized(); };

        track.onOpenWavetableEditor = [this](int trackIndex, std::shared_ptr<WavetableParams> params, std::shared_ptr<WavetableData> wavetable) {
            panelManager->openTrackWavetableEditor(trackIndex, params, wavetable);
        };
    });

    trackManager->forEachTrack([this](int, TrackComponent& track) {
        track.setSampleCategories(sampleManager->getSampleCategories());
    });

    sampleManager->onCategoriesChanged = [this](const juce::StringArray& categories) {
        trackManager->forEachTrack([&categories](int, TrackComponent& track) {
            track.setSampleCategories(categories);
        });
    };

    sampleManager->setSampleLoadCallback([this](int trackIndex, const juce::String& category, const juce::File& directory) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            trackManager->getTrack(trackIndex).loadSampleForCategory(category, directory);
        }
    });
}

void MainComponent::connectPanelCallbacks()
{
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
    playButton.onClick = [this] { togglePlay(); };
    stopButton.onClick = [this] { stopPlayback(); };
    setFolderButton.onClick = [this] { openFolderChooser(); };
    clearAllButton.onClick = [this] { trackManager->clearAllTracks(); };
    audioSettingsButton.onClick = [this] { showAudioSettingsDialog(); };

    bpmSlider.onValueChange = [this] {
        playbackController->setBPM(bpmSlider.getValue());
        audioEngine->setBPM(bpmSlider.getValue());
    };

    masterVolumeSlider.onValueChange = [this] {
        audioEngine->setMasterVolume(static_cast<float>(masterVolumeSlider.getValue()));
    };

    loopLengthComboBox.onChange = [this] {
        audioEngine->setLoopLength(loopLengthComboBox.getSelectedId());
    };

    generateButton.onClick = [this] {
        if (audioEngine->isPlaying())
            togglePlay();

        int targetTrack = drumTargetTrackCombo.getSelectedId() - 1; // -1 means all tracks (ID 0 = "All Tracks")
        if (targetTrack == -1)
            patternGenerator->generateSong(static_cast<PatternGenerator::Genre>(genreComboBox.getSelectedId()));
        else
            patternGenerator->generateSongForTrack(static_cast<PatternGenerator::Genre>(genreComboBox.getSelectedId()), targetTrack);
    };

    swingSlider.onValueChange = [this] {
        audioEngine->setSwingAmount(static_cast<float>(swingSlider.getValue()));
    };

    reverbSlider.onValueChange = [this] {
        audioEngine->setReverbWetLevel(static_cast<float>(reverbSlider.getValue()));
    };

    rhythmExplorerButton.onClick = [this] {
        bool visible = rhythmExplorerButton.getToggleState();
        panelManager->setRhythmExplorerVisible(visible);

        if (visible)
            addAndMakeVisible(panelManager->getRhythmExplorerComponent());
        else
            removeChildComponent(panelManager->getRhythmExplorerComponent());
        resized();
    };

    melodyWorkstationButton.onClick = [this] {
        bool visible = melodyWorkstationButton.getToggleState();
        panelManager->setMelodyPanelVisible(visible);

        if (visible)
            addAndMakeVisible(panelManager->getMelodyPanelComponent());
        else
            removeChildComponent(panelManager->getMelodyPanelComponent());
        resized();
    };

    // Wavetable button - verwendet NEUES Docking-System
    wavetableSynthButton.onClick = [this] {
        bool visible = wavetableSynthButton.getToggleState();
        dockingManager->setPanelVisible(PanelType::WavetableSynth, visible);
    };

    // Timeline button - verwendet NEUES Docking-System
    timelineButton.onClick = [this] {
        bool visible = timelineButton.getToggleState();
        dockingManager->setPanelVisible(PanelType::Timeline, visible);
    };
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    trackManager->prepareAudio(sampleRate, samplesPerBlockExpected);
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

    // === 1. TOP BAR (Transport Controls) ===
    auto topBarArea = bounds.removeFromTop(topBarHeight);
    layoutTopBar(topBarArea);

    // === 2. SIDEBAR (RhythmExplorer/MelodyPanel) ===
    bool hasOldSidebar = panelManager->isRhythmExplorerVisible() || panelManager->isMelodyPanelVisible();
    juce::Rectangle<int> rhythmExplorerArea;
    juce::Rectangle<int> melodyPanelArea;

    if (hasOldSidebar)
    {
        const int rhythmExplorerWidth = 280;
        const int melodyPanelWidth = 350;

        if (panelManager->isRhythmExplorerVisible() && panelManager->isMelodyPanelVisible())
        {
            int totalWidth = rhythmExplorerWidth + melodyPanelWidth + 20;
            auto rightArea = bounds.removeFromRight(totalWidth);
            melodyPanelArea = rightArea.removeFromRight(melodyPanelWidth);
            rightArea.removeFromRight(10);
            rhythmExplorerArea = rightArea;
        }
        else if (panelManager->isRhythmExplorerVisible())
        {
            rhythmExplorerArea = bounds.removeFromRight(rhythmExplorerWidth);
            bounds.removeFromRight(10);
        }
        else if (panelManager->isMelodyPanelVisible())
        {
            melodyPanelArea = bounds.removeFromRight(melodyPanelWidth);
            bounds.removeFromRight(10);
        }
    }

    // === 3. HAUPTBEREICH: Tracks (Links) + Wavetable/Timeline (Rechts) ===
    auto mainArea = bounds.reduced(5, 5);

    // Breite für Tracks (ca. 60% des Bereichs)
    const int trackAreaWidth = juce::jmax(500, mainArea.getWidth() * 6 / 10);
    auto trackArea = mainArea.removeFromLeft(trackAreaWidth);
    mainArea.removeFromLeft(5);  // Gap

    // === 3a. TRACKS layouten (Der eigentliche Step Sequencer) ===
    layoutTracks(trackArea);

    // === 3b. WAVETABLE + TIMELINE (Rechts mit StretchableLayout) ===
    auto* synthPanel = dockingManager->getPanel(PanelType::WavetableSynth);

    if (synthPanel && bottomTabs && verticalResizerBar)
    {
        juce::Component* comps[] = { synthPanel, verticalResizerBar.get(), bottomTabs.get() };
        int ids[] = { synthLayoutId, resizerLayoutId, bottomTabsLayoutId };
        int numComps = 3;

        stretchableManager.layOutComponents(comps, numComps,
                                             mainArea.getX(),
                                             mainArea.getY(),
                                             mainArea.getWidth(),
                                             mainArea.getHeight(),
                                             true, true);
    }

    // === 4. SIDEBAR PANELS layouten ===
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

void MainComponent::layoutTracks(juce::Rectangle<int> area)
{
    // Tracks vertikal anordnen
    const int trackHeight = area.getHeight() / numTracks;

    trackManager->forEachTrack([&, this](int i, TrackComponent& track)
    {
        int y = area.getY() + i * trackHeight;
        track.setBounds(area.getX(), y, area.getWidth(), trackHeight);
    });
}

void MainComponent::layoutTopBar(juce::Rectangle<int>& area)
{
    auto topRow = area.removeFromTop(45);
    auto bottomRow = area;

    const int rowY = 5;
    const int btnHeight = 35;
    const int sliderHeight = 30;
    int xPos = 10;

    // Top Row - Haupt-Controls
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

    // Bottom Row - Generator Controls
    xPos = 10;
    const int bottomY = 50;

    loopLengthLabel.setBounds(xPos, bottomY + 5, 35, 20);
    xPos += 35;
    loopLengthComboBox.setBounds(xPos, bottomY + 2, 100, sliderHeight);
    xPos += 110;
    genreComboBox.setBounds(xPos, bottomY + 2, 90, sliderHeight);
    xPos += 95;
    drumTargetLabel.setBounds(xPos, bottomY + 5, 40, 20);
    xPos += 42;
    drumTargetTrackCombo.setBounds(xPos, bottomY + 2, 100, sliderHeight);
    xPos += 105;
    generateButton.setBounds(xPos, bottomY, 100, btnHeight);
    xPos += 110;
    swingLabel.setBounds(xPos, bottomY + 5, 40, 20);
    xPos += 40;
    swingSlider.setBounds(xPos, bottomY + 2, 120, sliderHeight);
    xPos += 130;
    reverbLabel.setBounds(xPos, bottomY + 5, 45, 20);
    xPos += 45;
    reverbSlider.setBounds(xPos, bottomY + 2, 120, sliderHeight);

    // Sidebar Toggle Buttons
    xPos += 130;
    if (xPos + 120 < area.getWidth())
        rhythmExplorerButton.setBounds(xPos, bottomY, 120, btnHeight);

    xPos += 125;
    if (xPos + 90 < area.getWidth())
        melodyWorkstationButton.setBounds(xPos, bottomY, 90, btnHeight);

    xPos += 95;
    if (xPos + 110 < area.getWidth())
        wavetableSynthButton.setBounds(xPos, bottomY, 110, btnHeight);

    xPos += 115;
    if (xPos + 80 < area.getWidth())
        timelineButton.setBounds(xPos, bottomY, 80, btnHeight);
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
