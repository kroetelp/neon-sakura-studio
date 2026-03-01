#pragma once

#include "TimelineData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

class TimelineTransport
{
public:
    TimelineTransport(TimelineData& data);
    ~TimelineTransport() = default;

    // Playback state
    enum class State { Stopped, Playing, Recording };
    State getState() const { return state.load(); }
    bool isPlaying() const { return state.load() == State::Playing; }
    bool isRecording() const { return state.load() == State::Recording; }

    // Transport controls
    void play();
    void stop();
    void record();
    void setPlayheadBeat(double beat);
    double getPlayheadBeat() const { return timelineData.playheadBeat.load(); }

    // Called from AudioEngine to advance playhead
    void advancePlayhead(double beats);

    // Loop
    void setLoopEnabled(bool enabled) { timelineData.loopEnabled.store(enabled); }
    bool isLoopEnabled() const { return timelineData.loopEnabled.load(); }
    void setLoopRange(double startBeat, double endBeat);
    double getLoopStart() const { return timelineData.loopStartBeat.load(); }
    double getLoopEnd() const { return timelineData.loopEndBeat.load(); }

    // BPM
    void setBPM(double bpm) { timelineData.setBPM(bpm); }
    double getBPM() const { return timelineData.getBPM(); }

private:
    TimelineData& timelineData;
    std::atomic<State> state{State::Stopped};
};
