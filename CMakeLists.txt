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

# Define the application
juce_add_gui_app(NeonSakuraStudio
    INSTALL_EXPORTS NeonSakuraStudioTargets
    COMPANY_NAME "NeonSakura"
    PRODUCT_NAME "NeonSakuraStudio"
    VERSION ${PROJECT_VERSION}
)

target_sources(NeonSakuraStudio
    PRIVATE
    source/Main.cpp
    source/MainComponent.h
    source/MainComponent.cpp

    # Interfaces
    source/ITrackDataProvider.h
    source/ISampleProvider.h

    # Managers and Controllers
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

    # Core Components
    source/TrackModel.h
    source/TrackModel.cpp
    source/TrackType.h
    source/TrackAudioProcessor.h
    source/TrackAudioProcessor.cpp
    source/TrackComponent.h
    source/TrackComponent.cpp
    source/PatternGenerator.h
    source/PatternGenerator.cpp
    source/AudioEngine.h
    source/AudioEngine.cpp
    source/RhythmExplorer.h
    source/RhythmExplorer.cpp
    source/MusicTheory.h
    source/MusicTheory.cpp
    source/MelodyGenerator.h
    source/MelodyGenerator.cpp
    source/MelodyPanel.h
    source/MelodyPanel.cpp

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

    # Wavetable UI
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

    # Timeline
    source/Timeline/TimelineClip.h
    source/Timeline/TimelineClip.cpp
    source/Timeline/TimelineTrack.h
    source/Timeline/TimelineData.h
    source/Timeline/TimelineData.cpp
    source/Timeline/RecordingManager.h
    source/Timeline/RecordingManager.cpp
    source/Timeline/PatternToClipConverter.h
    source/Timeline/PatternToClipConverter.cpp
    source/Timeline/TimelineComponent.h
    source/Timeline/TimelineComponent.cpp
    source/Timeline/TimelineTransport.h
    source/Timeline/TimelineTransport.cpp
    source/Timeline/TimelineRenderer.h
    source/Timeline/TimelineRenderer.cpp
    source/Timeline/TimelinePanel.h
    source/Timeline/TimelinePanel.cpp

    # Step Sequencer
    source/StepSequencer/NeonStepButton.h
    source/StepSequencer/NeonStepButton.cpp
    source/StepSequencer/StepSequencerPanel.h
    source/StepSequencer/StepSequencerPanel.cpp
)

# Link JUCE modules and melatonin_blur
target_link_libraries(NeonSakuraStudio
    PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    melatonin_blur
)

if(JUCE_WINDOWS)
    target_link_libraries(NeonSakuraStudio PRIVATE juce::juce_recommended_config_flags)
    target_link_libraries(NeonSakuraStudio PRIVATE juce::juce_recommended_lto_flags)
endif()

# =========================================================
# WOOTING ANALOG SDK INTEGRATION (Windows only)
# =========================================================
if(WIN32)
    # Check if Wooting SDK exists before linking
    set(WOOTING_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/wooting/include")
    set(WOOTING_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/wooting/lib")

    if(EXISTS "${WOOTING_INCLUDE_DIR}" AND EXISTS "${WOOTING_LIB_DIR}")
        target_include_directories(NeonSakuraStudio PRIVATE "${WOOTING_INCLUDE_DIR}")

        # Find the correct library file (could be .lib or .dll.lib)
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