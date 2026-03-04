#include "MainComponent.h"
#include "TrackManager.h"
#include "SampleManager.h"
#include "PlaybackController.h"
#include "PanelManager.h"
#include "DockingManager.h"
#include "Timeline/TimelinePanel.h"
#include "WavetableUI/WavetablePanel.h"
#include "WavetableUI/WootingSettingsPanel.h"
#include "StepSequencer/StepSequencerPanel.h"
#include "AudioEngine.h"
#include "PatternGenerator.h"
#include "RhythmExplorer/RhythmExplorer.h"
#include "RhythmExplorer/RhythmExplorerPanel.h"
#include "MelodyPanel/MelodyPanel.h"
#include "MelodyPanel/MelodyPanelPanel.h"
#include "TrackComponent.h"
#include "WavetableSynth/WavetableData.h"
#include "WavetableSynth/WavetableEngine.h"
#include "Timeline/TimelineComponent.h"
#include "Theme/WorkspaceManager.h"

// VST Hosting
#include "VSTHost/VSTPluginManager.h"
#include "VSTHost/PluginWindow.h"
#include "VSTHost/PluginBrowserComponent.h"
#include "VSTHost/PluginInstance.h"
#include "VSTHost/PluginLoadingCoordinator.h"

// Audio Routing
#include "AudioRouting/CPUProfiler.h"

MainComponent::MainComponent()
{
    setLookAndFeel(&customLookAndFeel);

    formatManager.registerBasicFormats();
    initializeManagers();
    initializeUI();
    initializeDockingPanels();  // NEU: Panels registrieren und layouten
    connectTrackCallbacks();
    connectUICallbacks();

    // Initialize WorkspaceManager and connect
    auto& workspaceManager = WorkspaceManager::getInstance();
    workspaceManager.setMainComponent(this);
    workspaceManager.setAudioRoutingGraph(audioEngine->getAudioRoutingGraph());
    workspaceManager.onPresetLoaded = [this](const WorkspacePreset& preset) {
        // Update UI when preset is loaded
        if (trackToolsBar && trackToolsBar->getWorkspacePresetCombo())
        {
            auto* combo = trackToolsBar->getWorkspacePresetCombo();
            for (int i = 0; i < combo->getNumItems(); ++i)
            {
                if (combo->getItemText(i) == preset.name)
                {
                    combo->setSelectedId(i + 1, juce::dontSendNotification);
                    break;
                }
            }
        }
    };

    // Populate workspace combo
    if (trackToolsBar && trackToolsBar->getWorkspacePresetCombo())
    {
        auto* combo = trackToolsBar->getWorkspacePresetCombo();
        combo->clear();
        auto names = workspaceManager.getPresetNames();
        for (int i = 0; i < names.size(); ++i)
            combo->addItem(names[i], i + 1);
        combo->setSelectedId(1);
    }

    juce::File detectedDir = sampleManager->autoDetectSampleDirectory();
    if (detectedDir.exists())
    {
        folderLabel.setText(detectedDir.getFileName() + " (Auto)", juce::dontSendNotification);
        setFolderButton.setButtonText("Change Folder");
    }

    setAudioChannels(2, 2);
    startTimerHz(60);  // 60 Hz for smooth playhead animation (repaints only on change)
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

    // === VST Plugin Hosting initialisieren ===
    initializeVSTHosting();
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

    // === RhythmExplorerPanel erstellen und registrieren ===
    auto rhythmExplorerPanel = std::make_unique<RhythmExplorerPanel>();
    rhythmExplorerPanel->setTargetTrack(selectedTrackForRhythm);
    rhythmExplorerPanel->onApplyPattern = [this](int trackIndex, const std::vector<int>& steps, bool clearFirst) {
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
    rhythmExplorerPanel->onApplyFill = [this](int trackIndex, const std::vector<int>& steps) {
        if (trackIndex >= 0 && trackIndex < numTracks)
        {
            for (int step : steps)
            {
                if (step >= 0 && step < 64)
                    trackManager->getTrack(trackIndex).setStepActive(step, true);
            }
        }
    };
    dockingManager->registerPanel(std::move(rhythmExplorerPanel));

    // === MelodyPanelPanel erstellen und registrieren ===
    auto melodyPanelPanel = std::make_unique<MelodyPanelPanel>();
    melodyPanelPanel->setTargetTrack(selectedTrackForRhythm);
    melodyPanelPanel->onApplyMelody = [this](int trackIndex, const std::vector<std::pair<int, int>>& stepPitches) {
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
    dockingManager->registerPanel(std::move(melodyPanelPanel));

    // === PanelTogglesBar mit DockingManager verbinden ===
    panelTogglesBar->setDockingManager(dockingManager.get());

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
    auto& theme = ThemeManager::getInstance();

    // Tab-Component erstellen mit ThemeManager Styling
    bottomTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);

    // Tab-Bar Styling
    bottomTabs->setTabBarDepth(tabBarHeight);
    bottomTabs->setColour(juce::TabbedComponent::backgroundColourId, theme.getBackgroundColor());
    bottomTabs->setColour(juce::TabbedComponent::outlineColourId, theme.getPanelBorderColor());

    // Tab-Buttons Styling
    auto& tabBar = bottomTabs->getTabbedButtonBar();
    tabBar.setColour(juce::TabbedButtonBar::tabOutlineColourId, theme.getPanelBorderColor());
    tabBar.setColour(juce::TabbedButtonBar::frontOutlineColourId, theme.getAccentColor());

    // Timeline Tab hinzufügen
    auto* timelinePanel = dockingManager->getPanelAs<TimelinePanel>(PanelType::Timeline);
    if (timelinePanel)
    {
        // ACHTUNG: Wir fügen das Panel NICHT als Ownership hinzu,
        // sondern zeigen nur seinen Content an.
        // Das Panel bleibt im Besitz des DockingManagers.
        bottomTabs->addTab("Timeline", theme.getInfoColor(), timelinePanel, false);
    }

    // Step Sequencer Tab hinzufügen
    auto* stepSequencerPanel = dockingManager->getPanelAs<StepSequencerPanel>(PanelType::StepSequencer);
    if (stepSequencerPanel)
    {
        bottomTabs->addTab("Step Sequencer", theme.getAccentColor(), stepSequencerPanel, false);
    }

    // Standardmäßig Timeline Tab anzeigen
    bottomTabs->setCurrentTabIndex(timelineTabIndex);

    addAndMakeVisible(bottomTabs.get());
}

void MainComponent::initializeUI()
{
    auto& theme = ThemeManager::getInstance();

    // === NEW MODULAR TOP BAR COMPONENTS ===
    transportBar = std::make_unique<TransportBar>();
    transportBar->onPlay = [this]() { togglePlay(); };
    transportBar->onStop = [this]() { stopPlayback(); };
    addAndMakeVisible(transportBar.get());

    globalControlsBar = std::make_unique<GlobalControlsBar>();
    globalControlsBar->onBpmChanged = [this](double bpm) {
        audioEngine->setBPM(bpm);
    };
    globalControlsBar->onMasterVolumeChanged = [this](float volume) {
        audioEngine->setMasterVolume(volume);
    };
    globalControlsBar->onFolderButtonClicked = [this]() {
        openFolderChooser();
    };
    addAndMakeVisible(globalControlsBar.get());

    trackToolsBar = std::make_unique<TrackToolsBar>();
    trackToolsBar->onLoopLengthChanged = [this](int length) {
        playbackController->setLoopLength(length);
    };
    trackToolsBar->onGenerateClicked = [this]() {
        int targetTrack = trackToolsBar->getTargetTrackCombo()->getSelectedId();
        int genreId = trackToolsBar->getGenreCombo()->getSelectedId();
        // Convert combo ID to Genre enum
        PatternGenerator::Genre genre = static_cast<PatternGenerator::Genre>(genreId - 1);
        if (targetTrack == 0)
            patternGenerator->generateSong(genre);
        else
            patternGenerator->generateSongForTrack(genre, targetTrack - 1);
    };
    trackToolsBar->onWorkspacePresetChanged = [this](const juce::String& presetName) {
        WorkspaceManager::getInstance().loadPreset(presetName);
    };
    addAndMakeVisible(trackToolsBar.get());

    panelTogglesBar = std::make_unique<PanelTogglesBar>();
    panelTogglesBar->onPluginBrowserToggled = [this](bool show) {
        togglePluginBrowser();
    };
    addAndMakeVisible(panelTogglesBar.get());

    // === LEGACY CONTROLS (kept for compatibility) ===
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor());
    playButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    addAndMakeVisible(playButton);

    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor());
    stopButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    addAndMakeVisible(stopButton);

    setFolderButton.setButtonText("Set SuperDirt Folder");
    setFolderButton.setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    setFolderButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    addAndMakeVisible(setFolderButton);

    folderLabel.setText("No folder selected", juce::dontSendNotification);
    folderLabel.setColour(juce::Label::textColourId, theme.getTextSecondaryColor());
    addAndMakeVisible(folderLabel);

    clearAllButton.setButtonText("Clear All");
    clearAllButton.setColour(juce::TextButton::buttonColourId, theme.getErrorColor().darker(0.3f));
    clearAllButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    addAndMakeVisible(clearAllButton);

    audioSettingsButton.setButtonText("Audio");
    audioSettingsButton.setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    audioSettingsButton.setColour(juce::TextButton::textColourOffId, theme.getAccentColor());
    addAndMakeVisible(audioSettingsButton);

    wootingSettingsButton.setButtonText("Wooting");
    wootingSettingsButton.setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    wootingSettingsButton.setColour(juce::TextButton::textColourOffId, theme.getSuccessColor());
    addAndMakeVisible(wootingSettingsButton);

    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 36);
    bpmSlider.setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());
    bpmSlider.setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    addAndMakeVisible(bpmSlider);

    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, theme.getTextPrimaryColor());
    addAndMakeVisible(bpmLabel);
    bpmLabel.attachToComponent(&bpmSlider, true);

    masterVolumeSlider.setRange(0.0, 1.0, 0.01);
    masterVolumeSlider.setValue(0.8);
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 36);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());
    masterVolumeSlider.setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    addAndMakeVisible(masterVolumeSlider);

    masterVolumeLabel.setText("Master", juce::dontSendNotification);
    masterVolumeLabel.setColour(juce::Label::textColourId, theme.getTextPrimaryColor());
    addAndMakeVisible(masterVolumeLabel);
    masterVolumeLabel.attachToComponent(&masterVolumeSlider, true);

    loopLengthComboBox.addItem("16 Steps", 16);
    loopLengthComboBox.addItem("32 Steps", 32);
    loopLengthComboBox.addItem("48 Steps", 48);
    loopLengthComboBox.addItem("64 Steps", 64);
    loopLengthComboBox.setSelectedId(16);
    loopLengthComboBox.setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    loopLengthComboBox.setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    loopLengthComboBox.setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());
    addAndMakeVisible(loopLengthComboBox);

    loopLengthLabel.setText("Loop", juce::dontSendNotification);
    loopLengthLabel.setColour(juce::Label::textColourId, theme.getTextPrimaryColor());
    addAndMakeVisible(loopLengthLabel);

    genreComboBox.addItem("Techno", 1);
    genreComboBox.addItem("House", 2);
    genreComboBox.addItem("Trap", 3);
    genreComboBox.addItem("DnB", 4);
    genreComboBox.addItem("Ambient", 5);
    genreComboBox.addItem("Garage", 6);
    genreComboBox.setSelectedId(1);
    genreComboBox.setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    genreComboBox.setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    genreComboBox.setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());
    addAndMakeVisible(genreComboBox);

    drumTargetLabel.setText("Track:", juce::dontSendNotification);
    drumTargetLabel.setColour(juce::Label::textColourId, theme.getTextPrimaryColor());
    addAndMakeVisible(drumTargetLabel);

    drumTargetTrackCombo.addItem("All Tracks", 0);
    for (int i = 0; i < numTracks; ++i)
        drumTargetTrackCombo.addItem("Track " + juce::String(i + 1), i + 1);
    drumTargetTrackCombo.setSelectedId(0);
    drumTargetTrackCombo.setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    drumTargetTrackCombo.setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    drumTargetTrackCombo.setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());
    addAndMakeVisible(drumTargetTrackCombo);

    generateButton.setButtonText("Generate");
    generateButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor());
    generateButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    addAndMakeVisible(generateButton);

    swingSlider.setRange(0.0, 0.75, 0.01);
    swingSlider.setValue(0.0);
    swingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    swingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 28);
    swingSlider.setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());
    swingSlider.setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    addAndMakeVisible(swingSlider);

    swingLabel.setText("Swing", juce::dontSendNotification);
    swingLabel.setColour(juce::Label::textColourId, theme.getTextPrimaryColor());
    addAndMakeVisible(swingLabel);
    swingLabel.attachToComponent(&swingSlider, true);

    reverbSlider.setRange(0.0, 1.0, 0.01);
    reverbSlider.setValue(0.3);
    reverbSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    reverbSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 28);
    reverbSlider.setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());
    reverbSlider.setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    addAndMakeVisible(reverbSlider);

    reverbLabel.setText("Reverb", juce::dontSendNotification);
    reverbLabel.setColour(juce::Label::textColourId, theme.getTextPrimaryColor());
    addAndMakeVisible(reverbLabel);
    reverbLabel.attachToComponent(&reverbSlider, true);

    rhythmExplorerButton.setButtonText("Rhythm Explorer");
    rhythmExplorerButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor().withAlpha(0.7f));
    rhythmExplorerButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    rhythmExplorerButton.setClickingTogglesState(true);
    addAndMakeVisible(rhythmExplorerButton);

    melodyWorkstationButton.setButtonText("Melody WS");
    melodyWorkstationButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor().withAlpha(0.7f));
    melodyWorkstationButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    melodyWorkstationButton.setClickingTogglesState(true);
    addAndMakeVisible(melodyWorkstationButton);

    wavetableSynthButton.setButtonText("Wavetable Synth");
    wavetableSynthButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor().withHue(0.8f));
    wavetableSynthButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    wavetableSynthButton.setClickingTogglesState(true);
    wavetableSynthButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(wavetableSynthButton);

    // Timeline button
    timelineButton.setButtonText("Timeline");
    timelineButton.setColour(juce::TextButton::buttonColourId, theme.getSuccessColor().darker(0.3f));
    timelineButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    timelineButton.setClickingTogglesState(true);
    timelineButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(timelineButton);

    // Plugin Browser button
    pluginBrowserButton.setButtonText("Plugins");
    pluginBrowserButton.setColour(juce::TextButton::buttonColourId, theme.getAccentColor().withHue(0.9f));
    pluginBrowserButton.setColour(juce::TextButton::textColourOffId, theme.getTextPrimaryColor());
    pluginBrowserButton.setClickingTogglesState(true);
    pluginBrowserButton.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(pluginBrowserButton);
}

void MainComponent::connectTrackCallbacks()
{
    trackManager->forEachTrack([this](int i, TrackComponent& track) {
        addAndMakeVisible(&track);

        track.getComboBox().onChange = [this, i] {
            selectedTrackForRhythm = i;

            // Neue Panels über DockingManager updaten
            if (dockingManager)
            {
                auto* rhythmPanel = dockingManager->getPanelAs<RhythmExplorerPanel>(PanelType::RhythmExplorer);
                if (rhythmPanel)
                    rhythmPanel->setTargetTrack(i);

                auto* melodyPanel = dockingManager->getPanelAs<MelodyPanelPanel>(PanelType::MelodyPanel);
                if (melodyPanel)
                    melodyPanel->setTargetTrack(i);
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

// connectPanelCallbacks() ist nicht mehr benötigt - Callbacks werden jetzt direkt in initializeDockingPanels() gesetzt

void MainComponent::connectUICallbacks()
{
    playButton.onClick = [this] { togglePlay(); };
    stopButton.onClick = [this] { stopPlayback(); };
    setFolderButton.onClick = [this] { openFolderChooser(); };
    clearAllButton.onClick = [this] { trackManager->clearAllTracks(); };
    audioSettingsButton.onClick = [this] { showAudioSettingsDialog(); };

    wootingSettingsButton.onClick = [this] {
        auto* panel = new WootingSettingsPanel(*wootingManager);
        panel->setSize(300, 280);

        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(panel);
        options.dialogTitle = "Wooting Keyboard Configuration";
        options.dialogBackgroundColour = getDarkBackground();
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;

        options.launchAsync();
    };

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

    // RhythmExplorer und MelodyPanel werden jetzt über PanelTogglesBar und DockingManager verwaltet
    // Keine manuellen Button-Callbacks mehr nötig

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

    // Plugin Browser button - VST Hosting Sidebar
    pluginBrowserButton.onClick = [this] {
        togglePluginBrowser();
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

    // === 2. HAUPTBEREICH: Tracks (Links) + Wavetable/Timeline (Rechts) ===
    // Sidebar-Panels (RhythmExplorer, MelodyPanel) werden jetzt über DockingManager verwaltet
    // Sie können rechts oder links im DockingManager positioniert werden

    auto mainArea = bounds.reduced(5, 5);

    // === 2a. PLUGIN BROWSER (Links, wenn sichtbar) ===
    layoutPluginBrowser(mainArea);

    // Breite für Tracks (ca. 60% des Bereichs)
    const int trackAreaWidth = juce::jmax(500, mainArea.getWidth() * 6 / 10);
    auto trackArea = mainArea.removeFromLeft(trackAreaWidth);
    mainArea.removeFromLeft(5);  // Gap

    // === 2b. TRACKS layouten (Der eigentliche Step Sequencer) ===
    layoutTracks(trackArea);

    // === 2c. DOCKING MANAGER LAYOUT (Rechts: alle Panels werden hier positioniert) ===
    // DockingManager kümmert sich um RhythmExplorer, MelodyPanel, Wavetable, etc.
    dockingManager->updateDockedLayout(mainArea);

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

// layoutOldSidebarPanels ist nicht mehr benötigt - Panels werden über DockingManager verwaltet

void MainComponent::layoutTopBar(juce::Rectangle<int>& area)
{
    // Standard-Abstand (Margin) für alle UI-Elemente: oben 0, rechts 5, unten 0, links 5
    juce::FlexItem::Margin margin(0, 5, 0, 5);

    // ==========================================
    // 1. OBERE REIHE (Haupt-Controls)
    // ==========================================
    juce::FlexBox topFlexBox;
    topFlexBox.flexDirection = juce::FlexBox::Direction::row;         // Elemente nebeneinander
    topFlexBox.alignItems = juce::FlexBox::AlignItems::center;        // Vertikal zentrieren
    topFlexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart; // Links ausrichten

    // Elemente zur oberen Reihe hinzufügen
    topFlexBox.items.add(juce::FlexItem(playButton).withWidth(70).withHeight(35).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(stopButton).withWidth(70).withHeight(35).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(clearAllButton).withWidth(80).withHeight(35).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(setFolderButton).withWidth(160).withHeight(35).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(folderLabel).withWidth(150).withHeight(20).withMargin(margin));

    topFlexBox.items.add(juce::FlexItem(bpmLabel).withWidth(35).withHeight(20).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(bpmSlider).withWidth(140).withHeight(30).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(masterVolumeLabel).withWidth(50).withHeight(20).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(masterVolumeSlider).withWidth(120).withHeight(30).withMargin(margin));

    // Ein flexibler "Spacer" (Platzhalter), der den restlichen Platz einnimmt
    // und den folgenden Audio-Button ganz nach rechts schiebt.
    topFlexBox.items.add(juce::FlexItem().withFlex(1.0f));
    topFlexBox.items.add(juce::FlexItem(wootingSettingsButton).withWidth(70).withHeight(35).withMargin(margin));
    topFlexBox.items.add(juce::FlexItem(audioSettingsButton).withWidth(60).withHeight(35).withMargin(margin));

    // ==========================================
    // 2. UNTERE REIHE (Generator & Panels)
    // ==========================================
    juce::FlexBox bottomFlexBox;
    bottomFlexBox.flexDirection = juce::FlexBox::Direction::row;
    bottomFlexBox.alignItems = juce::FlexBox::AlignItems::center;
    bottomFlexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;

    // Elemente zur unteren Reihe hinzufügen
    bottomFlexBox.items.add(juce::FlexItem(loopLengthLabel).withWidth(35).withHeight(20).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(loopLengthComboBox).withWidth(100).withHeight(30).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(genreComboBox).withWidth(90).withHeight(30).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(drumTargetLabel).withWidth(40).withHeight(20).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(drumTargetTrackCombo).withWidth(100).withHeight(30).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(generateButton).withWidth(100).withHeight(35).withMargin(margin));

    bottomFlexBox.items.add(juce::FlexItem(swingLabel).withWidth(40).withHeight(20).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(swingSlider).withWidth(120).withHeight(30).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(reverbLabel).withWidth(45).withHeight(20).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(reverbSlider).withWidth(120).withHeight(30).withMargin(margin));

    // Wieder ein flexibler Spacer, um die Panel-Buttons nach rechts zu drücken
    bottomFlexBox.items.add(juce::FlexItem().withFlex(1.0f));

    bottomFlexBox.items.add(juce::FlexItem(rhythmExplorerButton).withWidth(120).withHeight(35).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(melodyWorkstationButton).withWidth(90).withHeight(35).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(wavetableSynthButton).withWidth(110).withHeight(35).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(timelineButton).withWidth(80).withHeight(35).withMargin(margin));
    bottomFlexBox.items.add(juce::FlexItem(pluginBrowserButton).withWidth(80).withHeight(35).withMargin(margin));

    // ==========================================
    // 3. LAYOUT ANWENDEN
    // ==========================================
    // Wir teilen den Bereich für die TopBar exakt in eine obere und untere Hälfte auf
    auto topRowArea = area.removeFromTop(area.getHeight() / 2);

    // Die Flexboxen auf die errechneten Rechtecke anwenden (mit etwas innerem Abstand)
    topFlexBox.performLayout(topRowArea.reduced(5, 5));
    bottomFlexBox.performLayout(area.reduced(5, 5));
}

void MainComponent::timerCallback()
{
    if (!isResizing)
        updatePlayhead();
}

void MainComponent::updatePlayhead()
{
    // Early exit if not playing and no state change needed
    const bool isPlaying = audioEngine->isPlaying();
    const int steps = audioEngine->getSamplesPerStep();

    trackManager->forEachTrack([&](int i, TrackComponent& track) {
        const int trackLoopLen = track.getTrackLoopLength();

        // Safely calculate step (prevent division by zero)
        int step = 0;
        if (steps > 0 && trackLoopLen > 0) {
            step = (int)(audioEngine->getSamplePosition() / steps) % trackLoopLen;
        }

        track.updatePlayhead(step, isPlaying);

        // Update audio meter level (lock-free read from atomic)
        track.updateLevel(audioEngine->getTrackLevel(i));
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

//==============================================================================
// VST HOSTING METHODS
//==============================================================================

void MainComponent::initializeVSTHosting()
{
    // VST Plugin Manager erstellen
    vstPluginManager = std::make_unique<VSTPluginManager>();
    vstPluginManager->initialize();

    // Phase 6.1: Plugin Loading Coordinator erstellen
    pluginLoadingCoordinator = std::make_unique<PluginLoadingCoordinator>();
    pluginLoadingCoordinator->initialize();

    // Plugin Loading Coordinator an AudioEngine übergeben
    audioEngine->setPluginLoadingCoordinator(pluginLoadingCoordinator.get());

    // Callbacks für Plugin Loading Coordinator konfigurieren
    pluginLoadingCoordinator->setPluginInsertedCallback(
        [this](uint32_t nodeId, int trackIndex)
        {
            DBG("Plugin inserted - NodeID: " + juce::String(nodeId) + ", Track: " + juce::String(trackIndex));

            // Plugin Window öffnen wenn Plugin eingefügt wurde
            auto plugin = audioEngine->getAudioRoutingGraph()->getPluginInstance(nodeId);
            if (plugin)
            {
                if (pluginWindowManager)
                    pluginWindowManager->openWindowForPlugin(*plugin);
            }
        }
    );

    pluginLoadingCoordinator->setPluginRemovedCallback(
        [this](uint32_t nodeId)
        {
            DBG("Plugin removed - NodeID: " + juce::String(nodeId));

            // Note: Plugin window will be automatically closed by timer callback
            // when it detects the plugin is no longer valid
            // We cannot close directly here because we only have nodeId, not PluginInstance
        }
    );

    pluginLoadingCoordinator->setLoadFailureCallback(
        [this](uint32_t requestId, const juce::String& error)
        {
            DBG("Plugin load failed - RequestID: " + juce::String(requestId) + ", Error: " + error);

            // UI-Feedback für Fehler anzeigen (z.B. Alert Window)
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Plugin Loading Failed",
                "Failed to load plugin: " + error,
                "OK");
        }
    );

    // Phase 6.2: Plugin Sandbox Manager (Optional) - Disabled for now
    // pluginSandboxManager = std::make_unique<PluginSandboxManager>();
    // pluginSandboxManager->initialize();
    // pluginSandboxManager->setCrashCallback(...);
    // pluginSandboxManager->setStateChangeCallback(...);
    // pluginSandboxManager->setCPUWarningCallback(...);

    // Plugin Window Manager erstellen
    pluginWindowManager = std::make_unique<PluginWindowManager>();

    // Plugin Browser Component erstellen
    pluginBrowserComponent = std::make_unique<PluginBrowserComponent>(*vstPluginManager, pluginWindowManager.get());
    addChildComponent(pluginBrowserComponent.get());
    pluginBrowserComponent->setVisible(false);  // Standardmäßig ausgeblendet

    // Callbacks verbinden
    connectVSTCallbacks();

    // Prüfen ob Cache existiert, sonst scannen
    if (!vstPluginManager->hasCachedPluginList())
    {
        vstPluginManager->scanForPlugins();
    }

    DBG("VST Hosting initialized - " + juce::String(vstPluginManager->getAvailableFormats().joinIntoString(", ")) + " formats available");
    DBG("Plugin Loading Coordinator initialized (Phase 6.1: Lock-Free Loading)");
    // DBG("Plugin Sandbox Manager initialized (Phase 6.2: Plugin Isolation) - Disabled for now");

    // Phase 6.3: CPU Profiler initialisieren (Vereinfachte Version)
    cpuProfiler = std::make_unique<CPUProfiler>();

    // Callbacks für CPU-Warnungen konfigurieren
    cpuProfiler->setWarningCallback(
        [this](ProfilingComponent type, int index, float cpuUsage)
        {
            juce::String componentName = toString(type) + " " + juce::String(index);
            DBG("CPU Warning - Component: " + componentName +
                ", CPU: " + juce::String(cpuUsage * 100.0f, 1) + "%");

            // UI-Feedback anzeigen (nur bei kritischer Last)
            if (cpuUsage > 0.95f)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "CPU Usage Critical",
                    "Component: " + componentName + "\n" +
                    "CPU Usage: " + juce::String(cpuUsage * 100.0f, 1) + "%\n\n" +
                    "Consider reducing effects or plugins to maintain stability.",
                    "OK");
            }
        }
    );

    // CPU Profiler an AudioEngine übergeben
    audioEngine->setCPUProfiler(cpuProfiler.get());

    DBG("CPU Profiler initialized (Phase 6.3: Performance Metrics) - Simplified version");
}

void MainComponent::connectVSTCallbacks()
{
    // Callback wenn ein Plugin geladen wird
    pluginBrowserComponent->setOnPluginLoaded([this](std::unique_ptr<PluginInstance> instance)
    {
        if (instance)
        {
            loadPluginToSelectedTrack(std::move(instance));
        }
    });

    // Track-Auswahl-Callbacks
    trackManager->forEachTrack([this](int i, TrackComponent& track)
    {
        // Bei Track-Auswahl: ausgewählten Track aktualisieren
        track.onSelect = [this, i]
        {
            selectedTrackIndex = i;
            pluginBrowserComponent->setTargetTrack(i);
        };
    });
}

void MainComponent::togglePluginBrowser()
{
    pluginBrowserVisible = !pluginBrowserVisible;
    pluginBrowserButton.setToggleState(pluginBrowserVisible, juce::dontSendNotification);
    pluginBrowserComponent->setVisible(pluginBrowserVisible);

    resized();
}

void MainComponent::loadPluginToSelectedTrack(std::unique_ptr<PluginInstance> instance)
{
    if (!instance)
        return;

    // Ziel-Track bestimmen (0-basiert = Track 1)
    int targetTrack = selectedTrackIndex;
    if (targetTrack < 0 || targetTrack >= numTracks)
        targetTrack = 0;  // Fallback auf Track 1

    // Get the AudioRoutingGraph from AudioEngine
    auto* graph = audioEngine->getAudioRoutingGraph();
    if (!graph)
    {
        DBG("AudioRoutingGraph not available");
        return;
    }

    // Get the raw AudioPluginInstance from the wrapper for verification
    auto* audioPlugin = instance->getAudioPluginInstance();
    if (!audioPlugin)
    {
        DBG("No AudioPluginInstance in wrapper");
        return;
    }

    // Convert unique_ptr to shared_ptr for shared ownership
    // The AudioRoutingGraph and LoadedPlugin will share ownership
    auto sharedInstance = std::shared_ptr<PluginInstance>(instance.release());

    // Insert the plugin into the track's plugin chain using shared ownership
    // This stores the PluginInstance wrapper in the graph for processing
    NodeID nodeId = graph->insertPluginInstanceInTrack(targetTrack, -1, sharedInstance);

    if (nodeId == 0)
    {
        DBG("Failed to insert plugin into track " + juce::String(targetTrack + 1));
        return;
    }

    // Store the loaded plugin info for window management and state access
    LoadedPlugin loadedPlugin;
    loadedPlugin.nodeId = nodeId;
    loadedPlugin.trackIndex = targetTrack;
    loadedPlugin.positionInChain = static_cast<int>(graph->getTrackPluginChain(targetTrack)->size()) - 1;
    loadedPlugin.pluginInstanceWrapper = sharedInstance;  // Shared ownership
    loadedPlugin.pluginInstance = audioPlugin;            // Convenience pointer
    loadedPlugins.push_back(loadedPlugin);

    // Open the plugin window for UI interaction
    auto* window = pluginWindowManager->openWindowForPlugin(*sharedInstance);

    if (window)
    {
        DBG("Loaded plugin '" + sharedInstance->getName() + "' on Track " + juce::String(targetTrack + 1)
            + " with NodeID " + juce::String(nodeId));
    }
    else
    {
        DBG("Plugin loaded but failed to create window: " + sharedInstance->getName());
    }

    // Update latency compensation after adding the plugin
    graph->updateLatencyCompensation();
}

juce::Rectangle<int> MainComponent::layoutPluginBrowser(juce::Rectangle<int>& mainArea)
{
    if (!pluginBrowserVisible)
        return {};

    // Plugin Browser auf der linken Seite
    auto browserArea = mainArea.removeFromLeft(pluginBrowserWidth);
    mainArea.removeFromLeft(5);  // Gap

    pluginBrowserComponent->setBounds(browserArea.reduced(2));

    return browserArea;
}
