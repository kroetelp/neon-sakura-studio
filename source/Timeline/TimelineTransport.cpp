#include "TimelineTransport.h"

TimelineTransport::TimelineTransport(TimelineData& data)
    : timelineData(data)
{
}

void TimelineTransport::play()
{
    if (state.load() == State::Stopped)
    {
        state.store(State::Playing);
    }
}

void TimelineTransport::stop()
{
    state.store(State::Stopped);
}

void TimelineTransport::record()
{
    if (state.load() == State::Stopped)
    {
        state.store(State::Recording);
    }
}

void TimelineTransport::setPlayheadBeat(double beat)
{
    timelineData.playheadBeat.store(std::max(0.0, beat));

    if (timelineData.onPositionChanged)
        timelineData.onPositionChanged();
}

void TimelineTransport::advancePlayhead(double beats)
{
    double newBeat = timelineData.playheadBeat.load() + beats;

    // Handle loop
    if (timelineData.loopEnabled.load())
    {
        double loopStart = timelineData.loopStartBeat.load();
        double loopEnd = timelineData.loopEndBeat.load();

        if (newBeat >= loopEnd)
        {
            newBeat = loopStart + std::fmod(newBeat - loopStart, loopEnd - loopStart);
        }
    }

    timelineData.playheadBeat.store(newBeat);
}

void TimelineTransport::setLoopRange(double startBeat, double endBeat)
{
    if (startBeat < endBeat)
    {
        timelineData.loopStartBeat.store(startBeat);
        timelineData.loopEndBeat.store(endBeat);
    }
}
