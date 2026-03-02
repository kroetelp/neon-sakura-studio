#include "PatternToClipConverter.h"
#include "../TrackAudioProcessor.h"
#include <cmath>

PatternToClipConverter::PatternToClipConverter()
{
}

std::unique_ptr<TimelineClip> PatternToClipConverter::convertToClip(
    const TrackModel& model,
    TrackAudioProcessor& processor,
    double startBeat,
    double bpm,
    double sampleRate)
{
    TrackType trackType = model.getTrackType();

    // Create appropriate clip type
    auto clip = std::make_unique<TimelineClip>(
        trackType == TrackType::Sampler
            ? TimelineClip::Type::Audio
            : TimelineClip::Type::Midi
    );

    // Set clip position
    clip->startBeat = startBeat;
    clip->lengthBeats = calculateClipLengthBeats(model.getTrackLoopLength(), bpm);

    // Set clip name based on type
    clip->name = trackType == TrackType::Sampler
        ? "Audio Pattern"
        : "MIDI Pattern";

    // Convert based on track type
    if (trackType == TrackType::Sampler)
    {
        renderStepsToAudio(model, processor, *clip, bpm, sampleRate);
    }
    else
    {
        convertStepsToMidi(model, *clip);
    }

    return clip;
}

void PatternToClipConverter::convertStepsToMidi(
    const TrackModel& model,
    TimelineClip& clip,
    int baseNote,
    float velocity)
{
    clip.clearMidiNotes();

    int loopLength = model.getTrackLoopLength();
    double stepLength = 1.0 / 4.0;  // Each step = 1/4 beat (16th note)

    // Iterate through all steps in the loop
    for (int step = 0; step < loopLength; ++step)
    {
        auto stepState = model.getStepState(step);

        if (stepState.isActive())
        {
            TimelineClip::MidiNote note;
            note.noteNumber = baseNote;

            // Apply pitch lock if present
            if (stepState.hasPitchLock)
            {
                note.noteNumber = juce::jlimit(0, 127, baseNote + stepState.pitchLock);
            }

            // Apply volume lock if present
            note.velocity = stepState.hasVolLock ? stepState.volLock : velocity;

            note.startBeat = step * stepLength;
            note.lengthBeats = stepLength;  // Default: one step length
            note.channel = 1;

            // Handle modifiers
            switch (stepState.modifierType)
            {
                case '@':  // Elongate - longer note
                    note.lengthBeats = stepLength * (1 + stepState.modifierValue);
                    break;
                case '/':  // Slow - same length but positioned differently (skip logic handled elsewhere)
                    break;
                case '*':  // Speed/Replicate - handled by creating additional notes
                    // For speed, we could create multiple shorter notes
                    break;
                case '?':  // Probability - could randomize velocity
                    // Apply probability effect on velocity
                    note.velocity *= (stepState.modifierValue / 100.0f);
                    break;
            }

            clip.addMidiNote(note);
        }
    }
}

void PatternToClipConverter::renderStepsToAudio(
    const TrackModel& model,
    TrackAudioProcessor& processor,
    TimelineClip& clip,
    double bpm,
    double sampleRate)
{
    int loopLength = model.getTrackLoopLength();
    double beatsPerSecond = bpm / 60.0;
    double secondsPerBeat = 1.0 / beatsPerSecond;
    double secondsPerStep = secondsPerBeat / 4.0;  // 16th notes

    // Calculate total length in samples
    double totalSeconds = loopLength * secondsPerStep;
    int totalSamples = static_cast<int>(std::ceil(totalSeconds * sampleRate));

    // Create audio buffer (stereo)
    auto buffer = std::make_shared<juce::AudioBuffer<float>>(2, totalSamples);
    buffer->clear();

    // Get the synthesizer for rendering
    auto& synth = processor.getSynthesiser();
    synth.setCurrentPlaybackSampleRate(sampleRate);

    // Process each step
    int samplesPerStep = static_cast<int>(secondsPerStep * sampleRate);

    // Create a temporary buffer for rendering each step
    juce::AudioBuffer<float> stepBuffer(2, samplesPerStep);

    for (int step = 0; step < loopLength; ++step)
    {
        auto stepState = model.getStepState(step);

        if (stepState.isActive())
        {
            // Clear the step buffer
            stepBuffer.clear();

            // Trigger the sample
            int midiNote = 60;  // C4

            // Apply pitch lock if present
            if (stepState.hasPitchLock)
            {
                midiNote = juce::jlimit(0, 127, 60 + stepState.pitchLock);
            }

            float vel = stepState.hasVolLock ? stepState.volLock : 0.8f;

            // Create MIDI buffer with note on/off
            juce::MidiBuffer midiBuffer;
            midiBuffer.addEvent(juce::MidiMessage::noteOn(1, midiNote, vel), 0);
            midiBuffer.addEvent(juce::MidiMessage::noteOff(1, midiNote), samplesPerStep - 100);

            // Render the step
            synth.renderNextBlock(stepBuffer, midiBuffer, 0, samplesPerStep);

            // Apply volume from modifier if present
            float volume = 1.0f;
            if (stepState.hasVolLock)
            {
                volume = stepState.volLock;
            }

            // Mix the step into the main buffer
            int destOffset = step * samplesPerStep;
            for (int channel = 0; channel < 2; ++channel)
            {
                buffer->addFrom(channel, destOffset, stepBuffer, channel, 0, samplesPerStep, volume);
            }

            // Turn off all notes to ensure clean state for next step
            synth.allNotesOff(0, false);
        }
    }

    // Set the audio buffer on the clip
    clip.setAudioBuffer(buffer, sampleRate);
}

double PatternToClipConverter::calculateClipLengthBeats(int loopLength, double bpm)
{
    // 16 steps = 4 beats (one bar at 4/4)
    // Each step = 1/4 beat
    return loopLength / 4.0;
}

double PatternToClipConverter::stepToBeat(int step, int stepsPerBeat) const
{
    return static_cast<double>(step) / stepsPerBeat;
}
