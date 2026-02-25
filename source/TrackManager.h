#pragma once

/**
 * TrackManager - Owns and manages the track array
 *
 * Responsibilities:
 * - Track array ownership and creation
 * - Track access methods
 * - Audio thread-safe access to track data
 * - Implements ITrackDataProvider for AudioEngine
 */

#include "ITrackDataProvider.h"
#include "TrackComponent.h"
#include <array>
#include <memory>

class TrackManager : public ITrackDataProvider
{
public:
    static constexpr int numTracks = 8;

    explicit TrackManager(juce::AudioFormatManager& formatManager);
    ~TrackManager() override = default;

    // ITrackDataProvider Implementation
    juce::Synthesiser& getSynthesiser(int trackIndex) override;
    WavetableSynth* getWavetableSynth(int trackIndex) override;
    bool isStepActive(int trackIndex, int step) const override;
    StepModifierState getStepState(int trackIndex, int step) const override;
    float getVolume(int trackIndex) const override;
    int getPitch(int trackIndex) const override;
    float getCutoff(int trackIndex) const override;
    bool getMuted(int trackIndex) const override;
    bool getSolo(int trackIndex) const override;
    bool getWavetableModulationEnabled(int trackIndex) const override;
    int getTrackLoopLength(int trackIndex) const override;
    TrackType getTrackType(int trackIndex) const override;
    void processAudioBlock(int trackIndex, juce::AudioBuffer<float>& buffer) override;

    // Track array access
    TrackComponent& getTrack(int index);
    const TrackComponent& getTrack(int index) const;
    std::array<std::unique_ptr<TrackComponent>, numTracks>& getTracks() { return tracks; }
    const std::array<std::unique_ptr<TrackComponent>, numTracks>& getTracks() const { return tracks; }

    // Track operations
    void clearAllTracks();
    void prepareAudio(double sampleRate, int samplesPerBlock);

    // Helper to iterate all tracks
    template<typename Func>
    void forEachTrack(Func&& func)
    {
        for (int i = 0; i < numTracks; ++i)
        {
            func(i, *tracks[i]);
        }
    }

private:
    std::array<std::unique_ptr<TrackComponent>, numTracks> tracks;
    juce::AudioFormatManager& formatManager;

    // Helper to validate track index
    bool isValidTrackIndex(int index) const { return index >= 0 && index < numTracks; }
};
