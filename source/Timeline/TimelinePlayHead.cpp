#include "TimelinePlayHead.h"
#include "TimelineData.h"
#include "TimelineTransport.h"

TimelinePlayHead::TimelinePlayHead(TimelineData& data, TimelineTransport& transport)
    : timelineData(data)
    , timelineTransport(transport)
{
}

juce::Optional<juce::AudioPlayHead::PositionInfo> TimelinePlayHead::getPosition() const
{
    // Get atomic values from TimelineData (thread-safe reads)
    const double bpm = timelineData.getBPM();
    const double playheadBeat = timelineData.playheadBeat.load();
    const int timeSigNumerator = timelineData.timeSigNumerator.load();
    const int timeSigDenominator = timelineData.timeSigDenominator.load();
    const bool loopEnabled = timelineData.loopEnabled.load();
    const double loopStartBeat = timelineData.loopStartBeat.load();
    const double loopEndBeat = timelineData.loopEndBeat.load();

    // Get transport state
    const bool isPlaying = timelineTransport.isPlaying();
    const bool isRecording = timelineTransport.isRecording();

    // Calculate time in seconds from beats
    // Formula: seconds = beats * (60 / BPM)
    const double secondsPerBeat = 60.0 / bpm;
    const double timeInSeconds = playheadBeat * secondsPerBeat;

    // Build PositionInfo
    auto positionInfo = PositionInfo();

    // Set time in seconds (wrapped in Optional)
    positionInfo.setTimeInSeconds(juce::makeOptional(timeInSeconds));

    // Set BPM (wrapped in Optional)
    positionInfo.setBpm(juce::makeOptional(bpm));

    // Set time signature (wrapped in Optional)
    TimeSignature timeSig;
    timeSig.numerator = timeSigNumerator;
    timeSig.denominator = timeSigDenominator;
    positionInfo.setTimeSignature(juce::makeOptional(timeSig));

    // Set play/record state (these are bool, not Optional)
    positionInfo.setIsPlaying(isPlaying);
    positionInfo.setIsRecording(isRecording);

    // Set PPQ position (beats in quarter note units)
    // In our system, 1 beat = 1 quarter note, so PPQ = beats
    positionInfo.setPpqPosition(juce::makeOptional(playheadBeat));

    // Calculate bar and beat within bar
    // For time signatures: 1 bar = timeSigNumerator beats
    const double beatsPerBar = static_cast<double>(timeSigNumerator);
    const int64_t barCount = static_cast<int64_t>(playheadBeat / beatsPerBar);
    const double ppqPositionOfLastBarStart = static_cast<double>(barCount) * beatsPerBar;

    positionInfo.setBarCount(juce::makeOptional(barCount));
    positionInfo.setPpqPositionOfLastBarStart(juce::makeOptional(ppqPositionOfLastBarStart));

    // Set loop points if looping is enabled
    positionInfo.setIsLooping(loopEnabled);
    if (loopEnabled)
    {
        LoopPoints loopPoints;
        loopPoints.ppqStart = loopStartBeat;
        loopPoints.ppqEnd = loopEndBeat;
        positionInfo.setLoopPoints(juce::makeOptional(loopPoints));
    }

    // Set frame rate (30 fps for video sync)
    FrameRate frameRate = FrameRate(FrameRateType::fps30);
    positionInfo.setFrameRate(juce::makeOptional(frameRate));

    // Set edit origin time (time when the project was created, default to 0)
    positionInfo.setEditOriginTime(juce::makeOptional(0.0));

    // Calculate time in samples
    const int64_t timeInSamples = static_cast<int64_t>(timeInSeconds * currentSampleRate);
    positionInfo.setTimeInSamples(juce::makeOptional(timeInSamples));

    return juce::makeOptional(positionInfo);
}
