#include "TimelineRenderer.h"

TimelineRenderer::TimelineRenderer(TimelineData& data, TimelineTransport& transport)
    : timelineData(data)
    , transport(transport)
    , mixBuffer(2, 512)
{
}

void TimelineRenderer::prepareToPlay(double sr, int spb)
{
    sampleRate = sr;
    samplesPerBlock = spb;
    mixBuffer.setSize(2, spb);
    reset();
}

void TimelineRenderer::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (!transport.isPlaying())
        return;

    int numSamples = buffer.getNumSamples();
    double currentBeat = transport.getPlayheadBeat();
    double beatsPerSample = transport.getBPM() / 60.0 / sampleRate;

    // Clear output buffer
    buffer.clear();

    // Update active clips at current position
    updateActiveClips(currentBeat);

    // Process each track
    for (int trackIdx = 0; trackIdx < TimelineData::numTracks; ++trackIdx)
    {
        auto& track = timelineData.getTrack(trackIdx);

        // Skip muted tracks (unless soloed)
        if (track.muted.load())
            continue;

        if (timelineData.hasAnySoloedTrack() && !track.soloed.load())
            continue;

        // Clear mix buffer for this track
        mixBuffer.clear();

        // Process active clips on this track
        for (auto& [clip, state] : activeClips[trackIdx])
        {
            if (clip->muted.load())
                continue;

            if (clip->getType() == TimelineClip::Type::Audio)
            {
                renderAudioClip(mixBuffer, *clip, state);
            }
            else if (clip->getType() == TimelineClip::Type::Midi)
            {
                renderMidiClip(midiMessages, *clip, currentBeat, numSamples);
            }
            else if (clip->getType() == TimelineClip::Type::StepSequencer)
            {
                if (auto* stepClip = dynamic_cast<StepSequencerClip*>(clip))
                {
                    double endBeat = currentBeat + (numSamples * beatsPerSample);
                    renderStepSequencerClip(midiMessages, *stepClip, currentBeat, endBeat, transport.getBPM());
                }
            }
        }

        // Apply track volume/pan and mix to output
        applyTrackMixing(buffer, trackIdx);
    }

    // Advance playhead
    double beatsToAdvance = beatsPerSample * numSamples;
    transport.advancePlayhead(beatsToAdvance);
}

void TimelineRenderer::reset()
{
    for (auto& trackClips : activeClips)
    {
        trackClips.clear();
    }
}

void TimelineRenderer::updateActiveClips(double currentBeat)
{
    for (int trackIdx = 0; trackIdx < TimelineData::numTracks; ++trackIdx)
    {
        auto& track = timelineData.getTrack(trackIdx);
        auto& clips = activeClips[trackIdx];

        // Clear inactive clips
        clips.erase(
            std::remove_if(clips.begin(), clips.end(),
                [currentBeat](const auto& pair) {
                    return !pair.first->containsBeat(currentBeat);
                }),
            clips.end());

        // Find new active clips
        track.forEachClip([this, &clips, currentBeat, trackIdx](TimelineClip& clip)
        {
            if (!clip.containsBeat(currentBeat))
                return;

            // Check if already in active list
            bool found = false;
            for (auto& [activeClip, state] : clips)
            {
                if (activeClip->getId() == clip.getId())
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                ClipRenderState state;
                state.isActive = true;
                // Calculate position within clip
                double offsetBeats = currentBeat - clip.startBeat.load();
                state.positionInClip = beatsToSamples(offsetBeats);
                clips.push_back({ &clip, state });
            }
        });
    }
}

void TimelineRenderer::renderAudioClip(juce::AudioBuffer<float>& buffer, TimelineClip& clip, ClipRenderState& state)
{
    if (!clip.audioBuffer || clip.audioBuffer->getNumSamples() == 0)
        return;

    auto* sourceBuffer = clip.audioBuffer.get();
    int numSamples = buffer.getNumSamples();
    int numChannels = std::min(buffer.getNumChannels(), sourceBuffer->getNumChannels());
    int64_t sourceLength = sourceBuffer->getNumSamples();
    float clipGain = clip.gain.load();

    int64_t writePos = 0;

    while (writePos < numSamples)
    {
        // Handle looping
        int64_t effectivePos = state.positionInClip;
        if (clip.loopEnabled.load())
        {
            effectivePos = effectivePos % sourceLength;
        }

        // Check if we've reached the end of non-looped clip
        if (effectivePos >= sourceLength)
            break;

        // Calculate how many samples we can copy
        int samplesToCopy = static_cast<int>(std::min(
            static_cast<int64_t>(numSamples - writePos),
            sourceLength - effectivePos));

        // Copy samples
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addFrom(ch, static_cast<int>(writePos),
                          *sourceBuffer, ch, static_cast<int>(effectivePos),
                          samplesToCopy, clipGain);
        }

        writePos += samplesToCopy;
        state.positionInClip += samplesToCopy;
    }
}

void TimelineRenderer::renderMidiClip(juce::MidiBuffer& midiBuffer, TimelineClip& clip,
                                       double startBeat, int numSamples)
{
    const auto& notes = clip.getMidiNotes();
    if (notes.empty())
        return;

    double clipStart = clip.startBeat.load();
    double beatsPerSample = transport.getBPM() / 60.0 / sampleRate;
    double endBeat = startBeat + (numSamples * beatsPerSample);

    for (const auto& note : notes)
    {
        // Convert note position from clip-relative to timeline-relative
        double noteStartBeat = clipStart + note.startBeat;
        double noteEndBeat = noteStartBeat + note.lengthBeats;

        // Check if note starts within this block
        if (noteStartBeat >= startBeat && noteStartBeat < endBeat)
        {
            int sampleOffset = static_cast<int>((noteStartBeat - startBeat) / beatsPerSample);

            // Add note-on
            midiBuffer.addEvent(juce::MidiMessage::noteOn(note.channel, note.noteNumber,
                                                          note.velocity),
                               sampleOffset);
        }

        // Check if note ends within this block
        if (noteEndBeat >= startBeat && noteEndBeat < endBeat)
        {
            int sampleOffset = static_cast<int>((noteEndBeat - startBeat) / beatsPerSample);

            // Add note-off
            midiBuffer.addEvent(juce::MidiMessage::noteOff(note.channel, note.noteNumber),
                               sampleOffset);
        }
    }
}

void TimelineRenderer::renderStepSequencerClip(juce::MidiBuffer& midiBuffer, StepSequencerClip& clip,
                                                 double currentBeat, double endBeat, double bpm)
{
    double clipStart = clip.startBeat.load();
    double clipEnd = clipStart + clip.lengthBeats.load();

    // Check if we're within the clip's time range
    if (currentBeat >= clipEnd || endBeat <= clipStart)
        return;

    // Render the step sequencer clip to MIDI
    clip.renderToMidiBuffer(midiBuffer, clipStart, currentBeat, endBeat, bpm, 0);
}

void TimelineRenderer::applyTrackMixing(juce::AudioBuffer<float>& buffer, int trackIndex)
{
    auto& track = timelineData.getTrack(trackIndex);
    float volume = track.volume.load();
    float pan = track.pan.load();

    int numSamples = mixBuffer.getNumSamples();
    int numChannels = std::min(buffer.getNumChannels(), mixBuffer.getNumChannels());

    // Apply pan and volume
    float leftGain = volume * std::cos((pan + 1.0f) * 0.5f * juce::MathConstants<float>::pi / 2.0f);
    float rightGain = volume * std::sin((pan + 1.0f) * 0.5f * juce::MathConstants<float>::pi / 2.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float channelGain = (ch == 0) ? leftGain : rightGain;
        buffer.addFrom(ch, 0, mixBuffer, ch, 0, numSamples, channelGain);
    }
}

double TimelineRenderer::samplesToBeats(int64_t samples) const
{
    double seconds = static_cast<double>(samples) / sampleRate;
    return seconds * transport.getBPM() / 60.0;
}

int64_t TimelineRenderer::beatsToSamples(double beats) const
{
    double seconds = beats * 60.0 / transport.getBPM();
    return static_cast<int64_t>(seconds * sampleRate);
}
