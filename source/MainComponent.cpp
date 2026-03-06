// Temporäre Datei für sauberen Code
#include "MainComponent.h"
#include "MainComponentExtension.h"
#include "TrackManager.h"
#include "SampleManager.h"
#include "PlaybackController.h"
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

    // ThemeManager Singleton garantiert initialisieren
    auto& themeManager = ThemeManager::getInstance();
    juce::ignoreUnused(themeManager);

    formatManager.registerBasicFormats();
    initializeManagers();
    initializeDockingPanels();
    // Track-Selection Callbacks wieder aktivieren
    connectTrackCallbacks();

    // Initialize WorkspaceManager and connect
    // TODO: TEMPORÄR DEAKTIVIERT FÜR DEBUGGING
    /*
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
    */

    // Auto-Detect Sample Directory
    juce::File detectedDir = sampleManager->autoDetectSampleDirectory();
    if (detectedDir.exists() && globalControlsBar && globalControlsBar->getFolderLabel())
    {
        globalControlsBar->getFolderLabel()->setText(detectedDir.getFileName() + " (Auto)", juce::dontSendNotification);
        globalControlsBar->getFolderButton()->setButtonText("Change Folder");
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

    // === DockingManager für Single-Window Workspace ===
    dockingManager = std::make_unique<DockingManager>();
    dockingManager->setMainComponent(this);

    patternGenerator = std::make_unique<PatternGenerator>(
        *trackManager,
        *sampleManager,
        [this](double newBpm) {
            audioEngine->setBPM(newBpm);
        }
    );

    wootingManager = std::make_unique<WootingManager>();
    audioEngine->getWavetableEngine().setMidiEventQueue(&wootingManager->getMidiQueue());

    // === VST Plugin Hosting initialisieren ===
    // TODO: TEMPORÄR DEAKTIVIERT FÜR DEBUGGING
    // initializeVSTHosting();
}

void MainComponent::initializeLayoutManagers()
{
    // === Main Layout Manager (TopBar + MainArea) ===
    mainResizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(
        &mainLayoutManager, topBarLayoutId, false);
    addAndMakeVisible(mainResizerBar.get());

    mainLayoutManager.setItemLayout(topBarLayoutId,
                                  topBarHeight,
                                  topBarHeight,
                                  topBarHeight);

    mainLayoutManager.setItemLayout(mainAreaLayoutId,
                                  minPanelHeight,
                                  -1,
                                  -1.0);

    // === Workspace Layout Manager (PluginBrowser + Workspace) ===
    workspaceResizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(
        &workspaceLayoutManager, pluginBrowserLayoutId, true);  // true = horizontal
    addAndMakeVisible(workspaceResizerBar.get());

    workspaceLayoutManager.setItemLayout(pluginBrowserLayoutId,
                                        minSidebarWidth,
                                        600,
                                        0);

    workspaceLayoutManager.setItemLayout(workspaceLayoutId,
                                        300,
                                        -1,
                                        1.0);

    // === Content Layout Manager (MainContent + BottomTabs) ===
    contentResizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(
        &contentLayoutManager, mainContentLayoutId, false);  // false = vertical
    addAndMakeVisible(contentResizerBar.get());

    contentLayoutManager.setItemLayout(mainContentLayoutId,
                                      minPanelHeight,
                                      -1,
                                      1.0);

    contentLayoutManager.setItemLayout(bottomTabsLayoutId,
                                      minPanelHeight,
                                      500,
                                      defaultBottomHeight);
}

void MainComponent::initializeDockingPanels()
{
    // === MODULAR TOP BAR COMPONENTS ===
    transportBar = std::make_unique<TransportBar>();
    transportBar->onPlay = [this]() {
        if (audioEngine->isPlaying())
            audioEngine->stopPlayback();
        else
            audioEngine->startPlayback();
    };
    transportBar->onStop = [this]() {
        audioEngine->stopPlayback();
    };
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
    globalControlsBar->onAudioSettingsClicked = [this]() {
        showAudioSettingsDialog();
    };
    globalControlsBar->onWootingSettingsClicked = [this]() {
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
    addAndMakeVisible(globalControlsBar.get());

    trackToolsBar = std::make_unique<TrackToolsBar>();
    trackToolsBar->onLoopLengthChanged = [this](int length) {
        // Convert combo ID to steps
        const int steps = length * 16;
        playbackController->setLoopLength(steps);
    };
    trackToolsBar->onGenerateClicked = [this]() {
        int targetTrack = trackToolsBar->getTargetTrackCombo()->getSelectedId() - 1;  // 0-based
        int genreId = trackToolsBar->getGenreCombo()->getSelectedId();
        // Convert combo ID to Genre enum
        PatternGenerator::Genre genre = static_cast<PatternGenerator::Genre>(genreId - 1);
        if (targetTrack == -1)  // All Tracks
            patternGenerator->generateSong(genre);
        else
            patternGenerator->generateSongForTrack(genre, targetTrack);
    };
    trackToolsBar->onClearAllClicked = [this]() {
        trackManager->clearAllTracks();
    };
    trackToolsBar->onSwingChanged = [this](float swing) {
        audioEngine->setSwingAmount(swing);
    };
    trackToolsBar->onReverbChanged = [this](float reverb) {
        audioEngine->setReverbWetLevel(reverb);
    };
    trackToolsBar->onWorkspacePresetChanged = [this](const juce::String& presetName) {
        WorkspaceManager::getInstance().loadPreset(presetName);
    };
    addAndMakeVisible(trackToolsBar.get());

    panelTogglesBar = std::make_unique<PanelTogglesBar>();
    panelTogglesBar->setDockingManager(dockingManager.get());
    panelTogglesBar->onPluginBrowserToggled = [this](bool show) {
        togglePluginBrowser();
    };
    addAndMakeVisible(panelTogglesBar.get());

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

    // === Layout-Manager initialisieren ===
    initializeLayoutManagers();

    // === Tab-Component für unteren Bereich initialisieren ===
    initializeBottomTabs();

    // === MainComponentExtension initialisieren ===
    // WICHTIG: Dies MUSS VOR dem docken der Panels erfolgen,
    // da die Extension die neuen Panels (RoutingMatrix, MasterBus) erstellt
    mainComponentExtension = std::make_unique<MainComponentExtension>();
    mainComponentExtension->initialize(this, audioEngine.get(), dockingManager.get());
    addAndMakeVisible(mainComponentExtension.get());

    // === Panels SICHTBAR machen (als Docked) ===
    // Wavetable und Timeline standardmäßig anzeigen (wie in PanelTogglesBar default)
    dockingManager->dockPanel(PanelType::WavetableSynth, DockPosition::Center);
    dockingManager->dockPanel(PanelType::Timeline, DockPosition::Center);
    // Rhythm, Melody und Step Sequencer initial versteckt
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

void MainComponent::connectTrackCallbacks()
{
    trackManager->forEachTrack([this](int i, TrackComponent& track) {
        addAndMakeVisible(&track);

        track.getComboBox().onChange = [this, i] {
            selectedTrackForRhythm = i;

            // Neue Panels über DockingManager updaten
            if (dockingManager)
            {
                dockingManager->updatePanelsForTrack(i);
            }

            juce::String category = trackManager->getTrack(i).getSelectedCategory();
            if (category.isNotEmpty() && sampleManager->getSampleDirectory().exists())
            {
                sampleManager->loadSampleForCategory(i, category);
            }
        };

        track.onStateChange = [this] { resized(); };

        track.onOpenWavetableEditor = [this](int trackIndex, std::shared_ptr<WavetableParams> params, std::shared_ptr<WavetableData> wavetable) {
            dockingManager->openTrackWavetableEditor(trackIndex, params, wavetable);
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

    // === 1. HAUPT-LAYOUT: TopBar + MainArea (vertikal resizbar) ===
    juce::Rectangle<int> topBarArea, mainArea;

    // TopBar hat fixe Höhe, berechne restlichen Bereich
    topBarArea = bounds.removeFromTop(topBarHeight);
    mainArea = bounds;

    // === 1a. TOP BAR (Transport Controls) ===
    layoutTopBar(topBarArea);

    // === 1b. MAIN RESIZER BAR ===
    mainResizerBar->setBounds({ topBarArea.getX(), topBarArea.getBottom(),
                                  topBarArea.getWidth(), resizerBarHeight });
    mainArea.removeFromTop(resizerBarHeight);

    // === 2. WORKSPACE LAYOUT: PluginBrowser + Workspace (horizontal resizbar) ===
    juce::Rectangle<int> pluginBrowserArea, workspaceArea;

    // Workspace ist der gesamte verbleibende Bereich
    workspaceArea = mainArea;

    // Plugin Browser Links, Workspace rechts
    if (pluginBrowserVisible)
    {
        pluginBrowserArea = workspaceArea.removeFromLeft(workspaceArea.getWidth() / 3);
    }
    else
    {
        pluginBrowserArea = workspaceArea.removeFromLeft(0);
    }

    // === 2a. PLUGIN BROWSER (Links, wenn sichtbar) ===
    if (pluginBrowserVisible)
    {
        layoutPluginBrowser(pluginBrowserArea);
    }
    else if (pluginBrowserComponent)
    {
        pluginBrowserComponent->setVisible(false);
    }

    // === 2b. WORKSPACE RESIZER BAR ===
    if (pluginBrowserVisible)
    {
        workspaceResizerBar->setBounds({ pluginBrowserArea.getRight(), workspaceArea.getY(),
                                         resizerBarHeight, workspaceArea.getHeight() });
        workspaceResizerBar->setVisible(true);
    }
    else
    {
        workspaceResizerBar->setVisible(false);
    }

    // === 3. CONTENT LAYOUT: MainContent + BottomTabs (vertikal resizbar) ===
    juce::Rectangle<int> mainContentArea, bottomTabsArea;

    // MainContent ist der verbleibende Workspace-Bereich
    mainContentArea = workspaceArea;

    // BottomTabs hat fixe Höhe, berechne restlichen Bereich
    bottomTabsArea = mainContentArea.removeFromTop(defaultBottomHeight);
    // mainContentArea ist jetzt der obere Teil (ohne BottomTabs)

    // === 3a. CONTENT RESIZER BAR ===
    contentResizerBar->setBounds({ mainContentArea.getX(), mainContentArea.getBottom(),
                                    mainContentArea.getWidth(), resizerBarHeight });

    // === 4. TRACKS layouten (Der eigentliche Step Sequencer) ===
    layoutTracks(mainContentArea);

    // === 5. DOCKING MANAGER LAYOUT (Rechts: alle Panels werden hier positioniert) ===
    dockingManager->updateDockedLayout(mainContentArea);

    // === 6. BOTTOM TABS layouten ===
    if (bottomTabs && bottomTabs->isVisible())
    {
        bottomTabs->setBounds(bottomTabsArea);
    }

    // === 7. MAIN COMPONENT EXTENSION (Floating Workspace Layout) ===
    // Die Extension wird über den gesamten Workspace-Bereich gelegt
    if (mainComponentExtension)
    {
        // Die Extension kann als Overlay funktionieren oder in einem eigenen Bereich liegen
        // Für den Moment platzieren wir sie im Workspace-Bereich
        mainComponentExtension->setBounds(mainContentArea);
    }

    // === 8. SICHERSTELLEN, DASS TOP-BARS ÜBER ALLEN PANELS LIEGEN ===
    // Die Top-Bars müssen nach vorne geholt werden, da sie in der addAndMakeVisible()
    // Reihenfolge zuerst hinzugefügt wurden, aber Panels später über ihnen liegen könnten
    if (transportBar) transportBar->toFront(false);
    if (globalControlsBar) globalControlsBar->toFront(false);
    if (trackToolsBar) trackToolsBar->toFront(false);
    if (panelTogglesBar) panelTogglesBar->toFront(false);

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
    // Neue modulare Bars layouten
    // TopBar wird in 2 Reihen unterteilt:
    // - Obere Reihe: TransportBar + GlobalControlsBar
    // - Untere Reihe: TrackToolsBar + PanelTogglesBar

    auto topRowArea = area.removeFromTop(area.getHeight() / 2);
    auto bottomRowArea = area;

    // Obere Reihe: TransportBar (links) + GlobalControlsBar (mitte/rechts)
    auto transportArea = topRowArea.removeFromLeft(150);
    auto globalControlsArea = topRowArea;

    transportBar->setBounds(transportArea);
    globalControlsBar->setBounds(globalControlsArea);

    // Untere Reihe: TrackToolsBar (links) + PanelTogglesBar (rechts)
    auto trackToolsArea = bottomRowArea.removeFromLeft(bottomRowArea.getWidth() * 0.7f);
    auto panelTogglesArea = bottomRowArea;

    trackToolsBar->setBounds(trackToolsArea);
    panelTogglesBar->setBounds(panelTogglesArea);
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
            if (globalControlsBar && globalControlsBar->getFolderLabel() && globalControlsBar->getFolderButton())
            {
                globalControlsBar->getFolderLabel()->setText(directory.getFileName(), juce::dontSendNotification);
                globalControlsBar->getFolderButton()->setButtonText("Change Folder");
            }
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

    // Plugin Browser Layout-Item dynamisch ändern
    if (pluginBrowserVisible)
    {
        workspaceLayoutManager.setItemLayout(pluginBrowserLayoutId,
                                          minSidebarWidth,
                                          600,
                                          defaultSidebarWidth);
    }
    else
    {
        workspaceLayoutManager.setItemLayout(pluginBrowserLayoutId,
                                          0,
                                          0,
                                          0);
    }

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
    if (!pluginBrowserVisible || !pluginBrowserComponent)
        return {};

    // Plugin Browser nutzt das übergebene Rechteck
    auto browserArea = mainArea.reduced(2);

    pluginBrowserComponent->setBounds(browserArea);
    pluginBrowserComponent->setVisible(true);

    return browserArea;
}
