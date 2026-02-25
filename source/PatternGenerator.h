#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <array>
#include <memory>
#include <random>
#include <atomic>

// Forward declarations
class TrackComponent;
struct StepModifierState;

class PatternGenerator
{
public:
    enum class Genre { TECHNO = 1, HOUSE = 2, TRAP = 3, DNB = 4, AMBIENT = 5, GARAGE = 6 };

    PatternGenerator(
        std::array<std::unique_ptr<TrackComponent>, 8>& tracksRef,
        const juce::StringArray& categoriesRef,
        const juce::File& directoryRef,
        std::atomic<double>& bpmRef,
        juce::Slider& bpmSliderRef,
        std::function<void()> onBpmChangedCallback
    );

    void generateSong(Genre genre);
    void clearTrackFully(int trackIdx);
    void clearAllTracks();

private:
    // Dependencies (references to MainComponent members)
    std::array<std::unique_ptr<TrackComponent>, 8>& tracks;
    const juce::StringArray& sampleCategories;
    const juce::File& sampleDirectory;
    std::atomic<double>& bpm;
    juce::Slider& bpmSlider;
    std::function<void()> onBpmChanged;

    // Helper functions
    bool setTrackCategoryIfAvailable(int trackIdx, const juce::String& category);
    void setStepsOnTrack(int trackIdx, const std::vector<int>& steps);
    void setStepModifier(int trackIdx, int step, char type, int value);
    std::vector<int> getEuclidean(int pulses, int steps, int offset = 0);

    // Genre-specific generators
    void generateTechno(std::mt19937& rng);
    void generateIdmGlitch(std::mt19937& rng);
    void generateTrapDrill(std::mt19937& rng);
    void generateAmbient(std::mt19937& rng);
    void generateDnB(std::mt19937& rng);
    void generateHouse(std::mt19937& rng);

    static constexpr int numTracks = 8;
    static constexpr int totalSteps = 64;
};
