// Temporäre Datei für sauberen Code
#include "MainComponent.h"
#include "TrackManager.h"
#include "SampleManager.h"
#include "PlaybackController.h"
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

//==============================================================================
// FloatingToolWindow - Helper class for auto-closing windows
//==============================================================================
class FloatingToolWindow : public juce::DocumentWindow
{
public:
    FloatingToolWindow(const juce::String& name, juce::Colour bg, std::function<void()> onCloseCb)
        : DocumentWindow(name, bg, juce::DocumentWindow::allButtons), onClose(std::move(onCloseCb)) {}

    void closeButtonPressed() override { if (onClose) onClose(); }
private:
    std::function<void()> onClose;
};

MainComponent::MainComponent()
{
    setLookAndFeel(&customLookAndFeel);

    // ThemeManager Singleton garantiert initialisieren
    auto& themeManager = ThemeManager::getInstance();
    juce::ignoreUnused(themeManager);

    formatManager.registerBasicFormats();
    initializeManagers();
    initializeUI();
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

void MainComponent::initializeUI()
{
    auto& theme = ThemeManager::getInstance();

    // === TRANSPORT BAR ===
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

    // === GLOBAL CONTROLS BAR ===
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

    // === TRACK TOOLS BAR ===
    trackToolsBar = std::make_unique<TrackToolsBar>();
    trackToolsBar->onLoopLengthChanged = [this](int length) {
        const int steps = length * 16;
        playbackController->setLoopLength(steps);
    };
    trackToolsBar->onGenerateClicked = [this]() {
        int targetTrack = trackToolsBar->getTargetTrackCombo()->getSelectedId() - 1;
        int genreId = trackToolsBar->getGenreCombo()->getSelectedId();
        PatternGenerator::Genre genre = static_cast<PatternGenerator::Genre>(genreId - 1);
        if (targetTrack == -1)
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
    addAndMakeVisible(trackToolsBar.get());

    // === MENU BAR ===
    menuBar = std::make_unique<juce::MenuBarComponent>(this);
    addAndMakeVisible(menuBar.get());

    // === WORKSPACE TABS ===
    workspaceTabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    workspaceTabs->setTabBarDepth(32);
    workspaceTabs->setColour(juce::TabbedComponent::backgroundColourId, theme.getBackgroundColor());
    workspaceTabs->setColour(juce::TabbedComponent::outlineColourId, theme.getPanelBorderColor());

    // Timeline Tab (Index 0)
    auto timelinePanel = std::make_unique<TimelinePanel>();
    timelinePanel->setDependencies(&audioEngine->getTimelineData(),
                                   &audioEngine->getRecordingManager());
    timelinePanel->setSampleManager(sampleManager.get());
    workspaceTabs->addTab("Timeline", theme.getInfoColor(), timelinePanel.release(), false);

    // Step Sequencer Tab (Index 1)
    auto stepSequencerPanel = std::make_unique<StepSequencerPanel>();
    stepSequencerPanel->setTrackManager(trackManager.get());
    workspaceTabs->addTab("Step Sequencer", theme.getAccentColor(), stepSequencerPanel.release(), false);

    workspaceTabs->setCurrentTabIndex(0);
    addAndMakeVisible(workspaceTabs.get());
}




void MainComponent::connectTrackCallbacks()
{
    trackManager->forEachTrack([this](int i, TrackComponent& track) {
        addAndMakeVisible(&track);

        track.getComboBox().onChange = [this, i] {
            selectedTrackForRhythm = i;

            juce::String category = trackManager->getTrack(i).getSelectedCategory();
            if (category.isNotEmpty() && sampleManager->getSampleDirectory().exists())
            {
                sampleManager->loadSampleForCategory(i, category);
            }
        };

        track.onStateChange = [this] { resized(); };

        // TODO: Implement Wavetable Editor window opening for individual tracks
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
    auto bounds = getLocalBounds();

    // 1. MENU BAR
    if (menuBar)
        menuBar->setBounds(bounds.removeFromTop(menuBarHeight));

    // 2. CONTROLS ROW 1
    auto controls1Area = bounds.removeFromTop(controlsRowHeight);
    if (transportBar)
        transportBar->setBounds(controls1Area.removeFromLeft(150));
    if (globalControlsBar)
        globalControlsBar->setBounds(controls1Area);

    // 3. CONTROLS ROW 2
    if (trackToolsBar)
        trackToolsBar->setBounds(bounds.removeFromTop(controlsRowHeight));

    // 4. WORKSPACE TABS
    if (workspaceTabs)
        workspaceTabs->setBounds(bounds);
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

//==============================================================================
// MENU BAR IMPLEMENTATION
//==============================================================================

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File", "View", "Tools", "Options" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 1) // View Menu
    {
        menu.addItem(menuWavetableSynth, "Wavetable Synth...", true, false);
        menu.addItem(menuRhythmExplorer, "Rhythm Explorer...", true, false);
        menu.addItem(menuMelodyPanel, "Melody Panel...", true, false);
        menu.addItem(menuPluginBrowser, "Plugin Browser...", true, false);
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    switch (menuItemID)
    {
        case menuWavetableSynth:
            openWavetableSynthWindow();
            break;
        case menuRhythmExplorer:
            openRhythmExplorerWindow();
            break;
        case menuMelodyPanel:
            openMelodyPanelWindow();
            break;
        case menuPluginBrowser:
            openPluginBrowserWindow();
            break;
        default:
            break;
    }
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Keyboard shortcuts for opening windows (when MainComponent has focus)
    if (key.getModifiers().isCommandDown())
    {
        switch (key.getKeyCode())
        {
            case '1':
                openWavetableSynthWindow();
                return true;
            case '2':
                openRhythmExplorerWindow();
                return true;
            case '3':
                openMelodyPanelWindow();
                return true;
            case '4':
                openPluginBrowserWindow();
                return true;
            default:
                break;
        }
    }
    return false;
}

//==============================================================================
// FLOATING WINDOW MANAGEMENT
//==============================================================================

void MainComponent::openWavetableSynthWindow()
{
    // Check if window already exists
    if (floatingWindows.count(menuWavetableSynth) > 0)
    {
        floatingWindows[menuWavetableSynth]->toFront(true);
        return;
    }

    // Create panel if not exists
    if (!wavetablePanelForWindow)
    {
        wavetablePanelForWindow = std::make_unique<WavetablePanel>();
        wavetablePanelForWindow->setEngine(&audioEngine->getWavetableEngine());
    }

    // Create window
    auto* window = new FloatingToolWindow(
        "Wavetable Synth",
        getDarkBackground(),
        [this]() { closeFloatingWindow(menuWavetableSynth); }
    );
    window->setContentNonOwned(wavetablePanelForWindow.get(), true);
    window->setResizable(true, true);
    window->setSize(600, 500);
    window->centreWithSize(600, 500);
    window->setVisible(true);

    floatingWindows[menuWavetableSynth] = std::unique_ptr<juce::DocumentWindow>(window);
}

void MainComponent::openRhythmExplorerWindow()
{
    // Check if window already exists
    if (floatingWindows.count(menuRhythmExplorer) > 0)
    {
        floatingWindows[menuRhythmExplorer]->toFront(true);
        return;
    }

    // Create panel if not exists
    if (!rhythmExplorerPanelForWindow)
    {
        rhythmExplorerPanelForWindow = std::make_unique<RhythmExplorerPanel>();
        rhythmExplorerPanelForWindow->setTargetTrack(selectedTrackForRhythm);
        rhythmExplorerPanelForWindow->onApplyPattern = [this](int trackIndex, const std::vector<int>& steps, bool clearFirst) {
            if (trackIndex >= 0 && trackIndex < numTracks)
            {
                if (clearFirst)
                    trackManager->getTrack(trackIndex).clearAllSteps();
                for (int step : steps)
                    trackManager->getTrack(trackIndex).setStepActive(step, true);
            }
        };
        rhythmExplorerPanelForWindow->onApplyFill = [this](int trackIndex, const std::vector<int>& steps) {
            if (trackIndex >= 0 && trackIndex < numTracks)
            {
                for (int step : steps)
                    if (step >= 0 && step < 64)
                        trackManager->getTrack(trackIndex).setStepActive(step, true);
            }
        };
    }

    // Create window
    auto* window = new FloatingToolWindow(
        "Rhythm Explorer",
        getDarkBackground(),
        [this]() { closeFloatingWindow(menuRhythmExplorer); }
    );
    window->setContentNonOwned(rhythmExplorerPanelForWindow.get(), true);
    window->setResizable(true, true);
    window->setSize(500, 450);
    window->centreWithSize(500, 450);
    window->setVisible(true);

    floatingWindows[menuRhythmExplorer] = std::unique_ptr<juce::DocumentWindow>(window);
}

void MainComponent::openMelodyPanelWindow()
{
    // Check if window already exists
    if (floatingWindows.count(menuMelodyPanel) > 0)
    {
        floatingWindows[menuMelodyPanel]->toFront(true);
        return;
    }

    // Create panel if not exists
    if (!melodyPanelPanelForWindow)
    {
        melodyPanelPanelForWindow = std::make_unique<MelodyPanelPanel>();
        melodyPanelPanelForWindow->setTargetTrack(selectedTrackForRhythm);
        melodyPanelPanelForWindow->onApplyMelody = [this](int trackIndex, const std::vector<std::pair<int, int>>& stepPitches) {
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

    // Create window
    auto* window = new FloatingToolWindow(
        "Melody Panel",
        getDarkBackground(),
        [this]() { closeFloatingWindow(menuMelodyPanel); }
    );
    window->setContentNonOwned(melodyPanelPanelForWindow.get(), true);
    window->setResizable(true, true);
    window->setSize(500, 450);
    window->centreWithSize(500, 450);
    window->setVisible(true);

    floatingWindows[menuMelodyPanel] = std::unique_ptr<juce::DocumentWindow>(window);
}

void MainComponent::openPluginBrowserWindow()
{
    // Check if window already exists
    if (floatingWindows.count(menuPluginBrowser) > 0)
    {
        floatingWindows[menuPluginBrowser]->toFront(true);
        return;
    }

    // Initialize VST hosting if not already done
    if (!vstPluginManager)
    {
        initializeVSTHosting();
    }

    if (!pluginBrowserComponent)
    {
        return;
    }

    // Create window
    auto* window = new FloatingToolWindow(
        "Plugin Browser",
        getDarkBackground(),
        [this]() { closeFloatingWindow(menuPluginBrowser); }
    );
    window->setContentNonOwned(pluginBrowserComponent.get(), true);
    window->setResizable(true, true);
    window->setSize(400, 500);
    window->centreWithSize(400, 500);
    window->setVisible(true);

    floatingWindows[menuPluginBrowser] = std::unique_ptr<juce::DocumentWindow>(window);
}

void MainComponent::closeFloatingWindow(int windowID)
{
    floatingWindows.erase(windowID);
}

