#include "TimelineData.h"
#include "TimelineTrack.h"

TimelineData::TimelineData()
{
    for (int i = 0; i < numTracks; ++i)
    {
        tracks[i] = std::make_unique<TimelineTrack>(i);
        tracks[i]->name = "Track " + juce::String(i + 1);
    }
}

TimelineTrack& TimelineData::getTrack(int index)
{
    jassert(index >= 0 && index < numTracks);
    return *tracks[index];
}

const TimelineTrack& TimelineData::getTrack(int index) const
{
    jassert(index >= 0 && index < numTracks);
    return *tracks[index];
}

double TimelineData::getTotalLengthBeats() const
{
    double maxLength = 0.0;
    for (int i = 0; i < numTracks; ++i)
    {
        double trackEnd = tracks[i]->getEndBeat();
        if (trackEnd > maxLength)
            maxLength = trackEnd;
    }
    return maxLength;
}

double TimelineData::beatsToSeconds(double beats) const
{
    double b = bpm.load();
    if (b <= 0.0) return 0.0;
    // At 120 BPM, 1 beat = 0.5 seconds
    // seconds = beats * 60 / BPM
    return beats * 60.0 / b;
}

double TimelineData::secondsToBeats(double seconds) const
{
    double b = bpm.load();
    if (b <= 0.0) return 0.0;
    return seconds * b / 60.0;
}

int64_t TimelineData::beatsToSamples(double beats, double sampleRate) const
{
    double seconds = beatsToSeconds(beats);
    return static_cast<int64_t>(seconds * sampleRate);
}

double TimelineData::samplesToBeats(int64_t samples, double sampleRate) const
{
    if (sampleRate <= 0.0) return 0.0;
    double seconds = static_cast<double>(samples) / sampleRate;
    return secondsToBeats(seconds);
}

TimelineClip* TimelineData::findClipAt(int trackIndex, double beat) const
{
    if (trackIndex < 0 || trackIndex >= numTracks)
        return nullptr;

    return tracks[trackIndex]->findClipAtBeat(beat);
}

bool TimelineData::hasAnySoloedTrack() const
{
    for (int i = 0; i < numTracks; ++i)
    {
        if (tracks[i]->soloed.load())
            return true;
    }
    return false;
}
