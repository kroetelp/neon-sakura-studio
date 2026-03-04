cmake_minimum_required(VERSION 3.22)

project(NeonSakuraStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Fetch JUCE first
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(JUCE)

# Set JUCE module path
set(JUCE_MODULE_PATH "${juce_SOURCE_DIR}/modules" CACHE PATH "")

# Fetch melatonin_blur content manually without running its CMakeLists.txt
FetchContent_Declare(
    melatonin_blur
    GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git
    GIT_TAG main
    GIT_SHALLOW TRUE
)

# Only populate, don't make available (this skips CMakeLists.txt)
FetchContent_GetProperties(melatonin_blur)
if(NOT melatonin_blur_POPULATED)
    FetchContent_Populate(melatonin_blur)
endif()

# Create our own interface library for melatonin_blur
add_library(melatonin_blur STATIC
    ${melatonin_blur_SOURCE_DIR}/melatonin_blur.cpp
)
target_include_directories(melatonin_blur PUBLIC ${melatonin_blur_SOURCE_DIR})
target_link_libraries(melatonin_blur PUBLIC juce::juce_graphics juce::juce_gui_basics)

# =========================================================
# NEON SAKURA STUDIO - DAW APPLICATION
# =========================================================
# Neon Sakura Studio is a standalone DAW (Digital Audio Workstation)
# that can:
#   - Host external VST3/AU plugins via juce::AudioPluginFormatManager
#   - Run internal native synthesizers (WavetableEngine)
#   - Provide timeline-based audio editing and mixing
# =========================================================

juce_add_gui_app(NeonSakuraStudio
    INSTALL_EXPORTS NeonSakuraStudioTargets
    COMPANY_NAME "NeonSakura"
    PRODUCT_NAME "Neon Sakura Studio"
    VERSION ${PROJECT_VERSION}

    # DAW needs comprehensive audio capabilities
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
)

# =========================================================
# DAW CORE SOURCE FILES
# =========================================================
target_sources(NeonSakuraStudio
    PRIVATE
    # Main Entry Point
    source/Main.cpp
    source/MainComponent.h
    source/MainComponent.cpp

    # Interfaces
    source/ITrackDataProvider.h
    source/ISampleProvider.h

    # =====================================================
    # THEME SYSTEM
    # =====================================================
    source/Theme/ThemeManager.h
    source/Theme/ThemeManager.cpp
    source/Theme/ProfessionalTheme.h
    source/Theme/ProfessionalLookAndFeel.h
    source/Theme/ProfessionalLookAndFeel.cpp
    source/Theme/WorkspacePreset.h
    source/Theme/WorkspacePreset.cpp
    source/Theme/WorkspaceManager.h
    source/Theme/WorkspaceManager.cpp

    # =====================================================
    # DAW ENGINE - Managers and Controllers
    # =====================================================
    source/TrackManager.h
    source/TrackManager.cpp
    source/SampleManager.h
    source/SampleManager.cpp
    source/PlaybackController.h
    source/PlaybackController.cpp
    source/PanelManager.h
    source/PanelManager.cpp
    source/DockablePanel.h
    source/DockablePanel.cpp
    source/DockingManager.h
    source/DockingManager.cpp

    # =====================================================
    # AUDIO ENGINE (Core DAW audio processing)
    # =====================================================
    source/AudioEngine.h
    source/AudioEngine.cpp
    source/TrackModel.h
    source/TrackModel.cpp
    source/TrackType.h
    source/TrackAudioProcessor.h
    source/TrackAudioProcessor.cpp
    source/TrackComponent.h
    source/TrackComponent.cpp

    # =====================================================
    # AUDIO ROUTING (AudioProcessorGraph-based signal flow)
    # =====================================================
    source/AudioRouting/IAudioSource.h
    source/AudioRouting/AudioRoutingGraph.h
    source/AudioRouting/AudioRoutingGraph.cpp
    source/AudioRouting/TrackProcessor.h
    source/AudioRouting/TrackProcessor.cpp
    source/AudioRouting/MasterOutputProcessor.h
    source/AudioRouting/MasterOutputProcessor.cpp
    source/AudioRouting/SidechainManager.h
    source/AudioRouting/SidechainManager.cpp
    source/AudioRouting/MIDIOutputHandler.h
    source/AudioRouting/MIDIOutputHandler.cpp
    source/AudioRouting/PluginParameterAutomation.h
    source/AudioRouting/PluginParameterAutomation.cpp
    source/AudioRouting/CPUProfiler.h
    source/AudioRouting/CPUProfiler.cpp

    # =====================================================
    # INTERNAL SYNTH (Native Wavetable Synthesizer)
    # =====================================================
    source/InternalSynth/InternalSynthProcessor.h
    source/InternalSynth/InternalSynthProcessor.cpp
    source/InternalSynth/InternalSynthEditor.h
    source/InternalSynth/InternalSynthEditor.cpp

    # =====================================================
    # VST HOSTING (External Plugin Support)
    # =====================================================
    source/VSTHost/PluginHostConfiguration.h
    source/VSTHost/VSTPluginManager.h
    source/VSTHost/VSTPluginManager.cpp
    source/VSTHost/PluginScanner.h
    source/VSTHost/PluginScanner.cpp
    source/VSTHost/PluginInstance.h
    source/VSTHost/PluginInstance.cpp
    source/VSTHost/PluginWindow.h
    source/VSTHost/PluginWindow.cpp
    source/VSTHost/PluginBrowserComponent.h
    source/VSTHost/PluginBrowserComponent.cpp
    source/VSTHost/PluginLoadingCoordinator.h
    source/VSTHost/PluginLoadingCoordinator.cpp
    # source/VSTHost/PluginSandbox.h  # TODO: Phase 6.2 - Disabled due to platform-specific issues
    # source/VSTHost/PluginSandbox.cpp  # TODO: Phase 6.2 - Disabled due to platform-specific issues

    # =====================================================
    # INTERNAL SYNTH - Native Wavetable Synthesizer
    # (Formerly PluginProcessor - now internal module)
    # =====================================================
    source/InternalSynth/InternalSynthProcessor.h
    source/InternalSynth/InternalSynthProcessor.cpp
    source/InternalSynth/InternalSynthEditor.h
    source/InternalSynth/InternalSynthEditor.cpp

    # Wavetable Synth Core
    source/WavetableSynth/WavetableData.h
    source/WavetableSynth/WavetableData.cpp
    source/WavetableSynth/WavetableOscillator.h
    source/WavetableSynth/WavetableOscillator.cpp
    source/WavetableSynth/SubOscillator.h
    source/WavetableSynth/SubOscillator.cpp
    source/WavetableSynth/WavetableFilter.h
    source/WavetableSynth/WavetableFilter.cpp
    source/WavetableSynth/Waveshaper.h
    source/WavetableSynth/Waveshaper.cpp
    source/WavetableSynth/WavetableVoice.h
    source/WavetableSynth/WavetableVoice.cpp
    source/WavetableSynth/WavetableSynth.h
    source/WavetableSynth/WavetableSynth.cpp
    source/WavetableSynth/WavetableEngine.h
    source/WavetableSynth/WavetableEngine.cpp
    source/WavetableSynth/WavetableParams.h
    source/WavetableSynth/WavetableParams.cpp
    source/WavetableSynth/WavetablePreset.h
    source/WavetableSynth/WavetablePresetManager.h
    source/WavetableSynth/WavetablePresetManager.cpp

    # Modulation System
    source/Modulation/ModulationSource.h
    source/Modulation/Modulator.h
    source/Modulation/Modulator.cpp
    source/Modulation/LFOModulator.h
    source/Modulation/LFOModulator.cpp
    source/Modulation/EnvelopeModulator.h
    source/Modulation/EnvelopeModulator.cpp
    source/Modulation/ModulationMatrix.h
    source/Modulation/ModulationMatrix.cpp

    # =====================================================
    # VST HOSTING - External Plugin Support
    # (For loading third-party VST3/AU plugins)
    # =====================================================
    source/VSTHost/PluginHostConfiguration.h
    source/VSTHost/VSTPluginManager.h
    source/VSTHost/VSTPluginManager.cpp
    source/VSTHost/PluginScanner.h
    source/VSTHost/PluginScanner.cpp
    source/VSTHost/PluginInstance.h
    source/VSTHost/PluginInstance.cpp

    # =====================================================
    # UI COMPONENTS
    # =====================================================
    # Synth UI
    source/WavetableUI/NeonSakuraLookAndFeel.h
    source/WavetableUI/NeonSakuraLookAndFeel.cpp
    source/WavetableUI/WavetableSynthEditor.h
    source/WavetableUI/WavetableSynthEditor.cpp
    source/WavetableUI/WavetablePanel.h
    source/WavetableUI/WavetablePanel.cpp
    source/WavetableUI/OscillatorSection.h
    source/WavetableUI/OscillatorSection.cpp
    source/WavetableUI/FilterSection.h
    source/WavetableUI/FilterSection.cpp
    source/WavetableUI/ShaperSection.h
    source/WavetableUI/ShaperSection.cpp
    source/WavetableUI/ModulationSection.h
    source/WavetableUI/ModulationSection.cpp
    source/WavetableUI/EnvelopeSection.h
    source/WavetableUI/EnvelopeSection.cpp
    source/WavetableUI/FXSection.h
    source/WavetableUI/FXSection.cpp
    source/WavetableUI/WavetableDisplay.h
    source/WavetableUI/WavetableDisplay.cpp
    source/WavetableUI/Oscilloscope.h
    source/WavetableUI/Oscilloscope.cpp
    source/WavetableUI/ModulationGrid.h
    source/WavetableUI/ModulationGrid.cpp
    source/WavetableUI/WootingSettingsPanel.h
    source/WavetableUI/WootingSettingsPanel.cpp

    # Timeline UI
    source/Timeline/TimelineClip.h
    source/Timeline/TimelineClip.cpp
    source/Timeline/TimelineTrack.h
    source/Timeline/TimelineData.h
    source/Timeline/TimelineData.cpp
    source/Timeline/TimelineTransport.h
    source/Timeline/TimelineTransport.cpp
    source/Timeline/TimelinePlayHead.h
    source/Timeline/TimelinePlayHead.cpp
    source/Timeline/TimelineRenderer.h
    source/Timeline/TimelineRenderer.cpp
    source/Timeline/RecordingManager.h
    source/Timeline/RecordingManager.cpp
    source/Timeline/PatternToClipConverter.h
    source/Timeline/PatternToClipConverter.cpp
    source/Timeline/TimelineComponent.h
    source/Timeline/TimelineComponent.cpp
    source/Timeline/TimelinePanel.h
    source/Timeline/TimelinePanel.cpp
    source/Timeline/AutomationClip.h
    source/Timeline/AutomationClip.cpp
    source/Timeline/AutomationLane.h
    source/Timeline/AutomationLane.cpp
    source/Timeline/TimelineViewport.h
    source/Timeline/TimelineViewport.cpp

    # Step Sequencer
    source/StepSequencer/NeonStepButton.h
    source/StepSequencer/NeonStepButton.cpp
    source/StepSequencer/StepSequencerPanel.h
    source/StepSequencer/StepSequencerPanel.cpp
    source/StepSequencer/StepSequencerClip.h
    source/StepSequencer/StepSequencerClip.cpp

    # Unified Sequencer (Multi-Mode Sequencer)
    source/Sequencer/UnifiedSequencerModel.h
    source/Sequencer/UnifiedSequencerModel.cpp
    source/Sequencer/UnifiedSequencerPanel.h
    source/Sequencer/UnifiedSequencerPanel.cpp
    source/Sequencer/TrackViewComponent.h
    source/Sequencer/TrackViewComponent.cpp
    source/Sequencer/PatternViewComponent.h
    source/Sequencer/PatternViewComponent.cpp
    source/Sequencer/TimelineViewComponent.h
    source/Sequencer/TimelineViewComponent.cpp

    # =====================================================
    # TOP BAR COMPONENTS (New Modular Design)
    # =====================================================
    source/UI/TransportBar.h
    source/UI/TransportBar.cpp
    source/UI/GlobalControlsBar.h
    source/UI/GlobalControlsBar.cpp
    source/UI/TrackToolsBar.h
    source/UI/TrackToolsBar.cpp
    source/UI/PanelTogglesBar.h
    source/UI/PanelTogglesBar.cpp
    source/UI/SidechainRoutingPanel.h
    source/UI/SidechainRoutingPanel.cpp
    source/UI/TrackHeader.h
    source/UI/TrackHeader.cpp
    source/UI/PluginChainEditor.h
    source/UI/PluginChainEditor.cpp

    # Music Generation
    source/PatternGenerator.h
    source/PatternGenerator.cpp
    source/RhythmExplorer/RhythmExplorer.h
    source/RhythmExplorer/RhythmExplorer.cpp
    source/RhythmExplorer/RhythmExplorerPanel.h
    source/RhythmExplorer/RhythmExplorerPanel.cpp
    source/MusicTheory.h
    source/MusicTheory.cpp
    source/MelodyGenerator.h
    source/MelodyGenerator.cpp
    source/MelodyPanel/MelodyPanel.h
    source/MelodyPanel/MelodyPanel.cpp
    source/MelodyPanel/MelodyPanelPanel.h
    source/MelodyPanel/MelodyPanelPanel.cpp

    # MIDI Event Queue
    source/MidiEventQueue.h
)

# =========================================================
# LINK JUCE MODULES
# =========================================================
# Note: juce_audio_processors is required for VST HOSTING
# (AudioPluginFormatManager, AudioProcessorGraph)
# =========================================================
target_link_libraries(NeonSakuraStudio
    PRIVATE
    # Core JUCE
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events

    # Audio
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors   # Required for VST hosting!
    juce::juce_audio_utils
    juce::juce_dsp

    # Note: LV2 Plugin Support is included in juce::juce_audio_processors (JUCE 8.0.4)
    # No separate juce::juce_lv2 module needed for hosting

    # Graphics/UI
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_opengl     # GPU acceleration

    # External
    melatonin_blur
)

# =========================================================
# ENABLE VST3 PLUGIN HOSTING
# =========================================================
# JUCE 8.0.4 requires explicit activation of plugin hosting features
# Without these flags, addDefaultFormats() won't add VST3 support
# =========================================================
if(WIN32)
    # Windows: Enable VST3 hosting (required for Serum2.vst3)
    target_compile_definitions(NeonSakuraStudio PRIVATE JUCE_PLUGINHOST_VST3=1)
    message(STATUS "VST3 Hosting: ENABLED for Windows")
elseif(APPLE)
    # macOS: Enable VST3 and AU hosting
    target_compile_definitions(NeonSakuraStudio PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_AU=1)
    message(STATUS "VST3 Hosting: ENABLED for macOS")
    message(STATUS "AU Hosting: ENABLED for macOS")
elseif(UNIX AND NOT APPLE)
    # Linux: Enable VST3 and LV2 hosting
    target_compile_definitions(NeonSakuraStudio PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_LV2=1)
    message(STATUS "VST3 Hosting: ENABLED for Linux")
    message(STATUS "LV2 Hosting: ENABLED for Linux")
endif()

# =========================================================
# COMPILER OPTIMIZATIONS (Windows)
# =========================================================
if(JUCE_WINDOWS)
    target_link_libraries(NeonSakuraStudio PRIVATE juce::juce_recommended_config_flags)
    target_link_libraries(NeonSakuraStudio PRIVATE juce::juce_recommended_lto_flags)
endif()

# =========================================================
# WOOTING ANALOG SDK INTEGRATION (Windows only)
# =========================================================
if(WIN32)
    set(WOOTING_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/wooting/include")
    set(WOOTING_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/wooting/lib")

    if(EXISTS "${WOOTING_INCLUDE_DIR}" AND EXISTS "${WOOTING_LIB_DIR}")
        target_include_directories(NeonSakuraStudio PRIVATE "${WOOTING_INCLUDE_DIR}")

        find_library(WOOTING_LIB
            NAMES wooting_analog_sdk wooting_analog_sdk.dll
            PATHS "${WOOTING_LIB_DIR}"
            NO_DEFAULT_PATH
        )

        if(WOOTING_LIB)
            target_link_libraries(NeonSakuraStudio PRIVATE "${WOOTING_LIB}")
            message(STATUS "Wooting Analog SDK found: ${WOOTING_LIB}")
        else()
            message(WARNING "Wooting Analog SDK library not found in ${WOOTING_LIB_DIR}")
        endif()
    else()
        message(STATUS "Wooting Analog SDK not found - keyboard features disabled")
    endif()
elseif(APPLE)
    message(STATUS "Wooting Analog SDK not available on macOS - keyboard features disabled")
elseif(UNIX)
    message(STATUS "Wooting Analog SDK not available on Linux - keyboard features disabled")
endif()
