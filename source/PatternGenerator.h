#pragma once

/**
 * PatternGenerator - Algorithmic pattern generation for various genres
 *
 * This class is decoupled from MainComponent through interfaces:
 * - TrackManager: Access to tracks for pattern manipulation
 * - ISampleProvider: Access to sample categories and loading
 *
 * Uses callbacks for BPM changes instead of direct UI access.
 */

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <array>
#include <memory>
#include <random>
#include <atomic>
#include <functional>

// Forward declarations
class TrackManager;
class ISampleProvider;
struct StepModifierState;

class PatternGenerator
{
public:
    enum class Genre { TECHNO = 1, HOUSE = 2, TRAP = 3, DNB = 4, AMBIENT = 5, GARAGE = 6 };

    /**
     * Constructor with decoupled dependencies
     * @param trackManager Access to tracks for pattern manipulation
     * @param sampleProvider Access to sample categories and loading
     * @param onBpmChangedCallback Callback when BPM changes (to notify UI)
     */
    PatternGenerator(
        TrackManager& trackManager,
        ISampleProvider& sampleProvider,
        std::function<void(double)> onBpmChangedCallback = nullptr
    );

    // Generate a song pattern for the specified genre
    void generateSong(Genre genre);

    // Generate pattern for a specific track only (or all tracks if targetTrack == -1)
    void generateSongForTrack(Genre genre, int targetTrack);

    // Clear a single track (steps and P-Locks)
    void clearTrackFully(int trackIdx);

    // Clear all tracks
    void clearAllTracks();

private:
    // Dependencies
    TrackManager& trackManager;
    ISampleProvider& sampleProvider;

    // Callback for BPM changes
    std::function<void(double)> onBpmChanged;

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

    // Set BPM and notify callback
    void setBPM(double newBpm);

    static constexpr int numTracks = 8;
    static constexpr int totalSteps = 64;
};
