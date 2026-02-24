cmake_minimum_required(VERSION 3.22)

project(NeonSakuraStudio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE FetchContent configuration
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)

FetchContent_MakeAvailable(JUCE)

# Find JUCE module paths
set(JUCE_MODULE_PATH "${JUCE_SOURCE_DIR}/modules" CACHE PATH "")

# Define JUCE modules
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
    source/TrackComponent.h
    source/TrackComponent.cpp
)

# Add JUCE modules
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
)

# Build targets for supported platforms
if(JUCE_WINDOWS)
    target_link_libraries(NeonSakuraStudio PRIVATE juce::juce_recommended_config_flags)
    target_link_libraries(NeonSakuraStudio PRIVATE juce::juce_recommended_lto_flags)
endif()
