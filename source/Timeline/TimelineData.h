#pragma once

#include "TimelineClip.h"
#include "TimelineTrack.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <memory>
#include <vector>
#include <functional>
#include <mutex>

class TimelineData
{
public:
    static constexpr int numTracks = 8;

    TimelineData();
    ~TimelineData() = default;

    // Tracks
    TimelineTrack& getTrack(int index);
    const TimelineTrack& getTrack(int index) const;
    int getNumTracks() const { return numTracks; }

    // Global timing
    std::atomic<double> bpm{120.0};
    std::atomic<int> timeSigNumerator{4};
    std::atomic<int> timeSigDenominator{4};

    double getBPM() const { return bpm.load(); }
    void setBPM(double newBpm) { bpm.store(newBpm); }

    // Loop
    std::atomic<bool> loopEnabled{false};
    std::atomic<double> loopStartBeat{0.0};
    std::atomic<double> loopEndBeat{16.0};  // Default 4 bars

    // Playhead position (in beats)
    std::atomic<double> playheadBeat{0.0};

    // Get total length (end of last clip)
    double getTotalLengthBeats() const;

    // Beat/Time conversion
    double beatsToSeconds(double beats) const;
    double secondsToBeats(double seconds) const;
    int64_t beatsToSamples(double beats, double sampleRate) const;
    double samplesToBeats(int64_t samples, double sampleRate) const;

    // Find clip at position
    TimelineClip* findClipAt(int trackIndex, double beat) const;

    // Solo state helper
    bool hasAnySoloedTrack() const;

    // Callbacks for UI updates
    std::function<void()> onStructureChanged;
    std::function<void()> onPositionChanged;

private:
    std::array<std::unique_ptr<TimelineTrack>, numTracks> tracks;
    mutable std::mutex dataMutex;
};
