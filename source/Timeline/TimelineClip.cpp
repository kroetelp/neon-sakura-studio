#include "TimelineClip.h"

TimelineClip::TimelineClip(Type type)
    : clipType(type)
    , clipId(juce::Uuid())
{
}

void TimelineClip::setAudioBuffer(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate)
{
    audioBuffer = buffer;
    audioSampleRate = sampleRate;
}

int64_t TimelineClip::getAudioLengthSamples() const
{
    if (!audioBuffer)
        return 0;
    return audioBuffer->getNumSamples();
}

double TimelineClip::getAudioLengthSeconds() const
{
    if (audioSampleRate <= 0)
        return 0.0;
    return static_cast<double>(getAudioLengthSamples()) / audioSampleRate;
}

void TimelineClip::addMidiNote(const MidiNote& note)
{
    midiNotes.push_back(note);
    // Sort by start time
    std::sort(midiNotes.begin(), midiNotes.end(),
        [](const MidiNote& a, const MidiNote& b) {
            return a.startBeat < b.startBeat;
        });
}

void TimelineClip::clearMidiNotes()
{
    midiNotes.clear();
}

std::unique_ptr<TimelineClip> TimelineClip::clone() const
{
    auto newClip = std::make_unique<TimelineClip>(clipType);

    newClip->startBeat.store(startBeat.load());
    newClip->lengthBeats.store(lengthBeats.load());
    newClip->gain.store(gain.load());
    newClip->pan.store(pan.load());
    newClip->muted.store(muted.load());
    newClip->loopEnabled.store(loopEnabled.load());
    newClip->name = name + " (copy)";

    if (clipType == Type::Audio && audioBuffer)
    {
        newClip->audioBuffer = std::make_shared<juce::AudioBuffer<float>>(*audioBuffer);
        newClip->audioSampleRate = audioSampleRate;
    }
    else if (clipType == Type::Midi)
    {
        newClip->midiNotes = midiNotes;
    }

    return newClip;
}
