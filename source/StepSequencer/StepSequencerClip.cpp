// ============================================================================
// StepSequencerClip.cpp - Implementierung
// ============================================================================

#include "StepSequencerClip.h"
#include <algorithm>
#include <cmath>

StepSequencerClip::StepSequencerClip()
    : TimelineClip(Type::StepSequencer)
{
    // Initialize default values
    for (int t = 0; t < maxTracks; ++t)
    {
        trackBaseNote[t] = 60 + t;  // C4, D4, E4, F4, G4, A4, B4, C5
        trackMidiChannel[t] = 1;

        for (int s = 0; s < maxSteps; ++s)
        {
            stepActive[t][s] = false;
            stepVelocity[t][s] = 0.8f;
            stepNote[t][s] = -1;  // Use base note
        }
    }

    name = "Step Seq";
    lengthBeats = 4.0;  // Default: 1 bar at 16 steps
}

// === Pattern Configuration ===

void StepSequencerClip::setNumTracks(int num)
{
    numTracks = juce::jlimit(1, maxTracks, num);
}

void StepSequencerClip::setNumSteps(int num)
{
    numSteps = juce::jlimit(1, maxSteps, num);
    // Update length in beats based on steps
    lengthBeats = static_cast<double>(numSteps) / stepsPerBeat;
}

// === Step Data Access ===

void StepSequencerClip::setStepActive(int track, int step, bool active)
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        stepActive[track][step] = active;
    }
}

bool StepSequencerClip::isStepActive(int track, int step) const
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        return stepActive[track][step];
    }
    return false;
}

void StepSequencerClip::setStepVelocity(int track, int step, float velocity)
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        stepVelocity[track][step] = juce::jlimit(0.0f, 1.0f, velocity);
    }
}

float StepSequencerClip::getStepVelocity(int track, int step) const
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        return stepVelocity[track][step];
    }
    return 0.8f;
}

void StepSequencerClip::setStepNote(int track, int step, int noteNumber)
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        stepNote[track][step] = noteNumber;  // -1 = use base note
    }
}

int StepSequencerClip::getStepNote(int track, int step) const
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        return stepNote[track][step];
    }
    return -1;
}

void StepSequencerClip::setStepModifier(int track, int step, const StepModifier& modifier)
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        stepModifiers[track][step] = modifier;
    }
}

StepModifier StepSequencerClip::getStepModifier(int track, int step) const
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        return stepModifiers[track][step];
    }
    return StepModifier{};
}

// === Per-Track Settings ===

void StepSequencerClip::setTrackBaseNote(int track, int noteNumber)
{
    if (track >= 0 && track < maxTracks)
    {
        trackBaseNote[track] = juce::jlimit(0, 127, noteNumber);
    }
}

int StepSequencerClip::getTrackBaseNote(int track) const
{
    if (track >= 0 && track < maxTracks)
    {
        return trackBaseNote[track];
    }
    return 60;
}

void StepSequencerClip::setTrackMidiChannel(int track, int channel)
{
    if (track >= 0 && track < maxTracks)
    {
        trackMidiChannel[track] = juce::jlimit(1, 16, channel);
    }
}

int StepSequencerClip::getTrackMidiChannel(int track) const
{
    if (track >= 0 && track < maxTracks)
    {
        return trackMidiChannel[track];
    }
    return 1;
}

// === MIDI Generation ===

bool StepSequencerClip::renderToMidiBuffer(
    juce::MidiBuffer& midiBuffer,
    double clipStartBeat,
    double currentBeat,
    double endBeat,
    double bpm,
    int startSample)
{
    if (muted.load())
        return false;

    bool generatedAny = false;
    double stepLength = getStepLengthBeats();
    double clipEndBeat = clipStartBeat + lengthBeats.load();

    // Check if we're within the clip bounds
    if (currentBeat < clipStartBeat || currentBeat >= clipEndBeat)
        return false;

    // Handle looping
    double effectiveLength = lengthBeats.load();
    if (loopEnabled.load() && effectiveLength > 0)
    {
        // Wrap around if we exceed the clip length
        while (currentBeat >= clipEndBeat)
        {
            currentBeat -= effectiveLength;
            clipStartBeat += effectiveLength;
            clipEndBeat += effectiveLength;
        }
    }

    // Calculate beats per sample for timing
    double samplesPerBeat = (60.0 / bpm) * 44100.0;  // Assuming 44100 Hz
    int samplesPerStep = static_cast<int>(samplesPerBeat / stepsPerBeat);

    // Process each track
    for (int track = 0; track < numTracks; ++track)
    {
        int channel = trackMidiChannel[track];
        int baseNote = trackBaseNote[track];

        // Calculate which steps fall within this time range
        double beatWithinClip = currentBeat - clipStartBeat;
        int startStep = static_cast<int>(beatWithinClip / stepLength);
        double endBeatWithinClip = endBeat - clipStartBeat;
        int endStep = static_cast<int>(std::ceil(endBeatWithinClip / stepLength));

        // Clamp to valid range
        startStep = juce::jmax(0, startStep);
        endStep = juce::jmin(numSteps - 1, endStep);

        // Process each step in range
        for (int step = startStep; step <= endStep; ++step)
        {
            bool isActive = stepActive[track][step];

            // Check for state change (note on/off)
            if (isActive != lastStepState[track])
            {
                double stepBeat = clipStartBeat + step * stepLength;
                double beatOffset = stepBeat - currentBeat;
                int sampleOffset = startSample + static_cast<int>(beatOffset * samplesPerBeat);

                if (sampleOffset >= 0 && sampleOffset < midiBuffer.getLastEventTime() + samplesPerStep)
                {
                    if (isActive)
                    {
                        // Note On
                        int noteNumber = (stepNote[track][step] >= 0)
                            ? stepNote[track][step]
                            : baseNote;

                        // Apply pitch lock from modifier
                        const auto& modifier = stepModifiers[track][step];
                        if (modifier.hasPitchLock)
                        {
                            noteNumber = juce::jlimit(0, 127, baseNote + modifier.pitchLock);
                        }

                        // Apply velocity
                        float velocity = stepVelocity[track][step];
                        if (modifier.hasVolLock)
                        {
                            velocity = modifier.volLock;
                        }

                        // Apply probability modifier
                        if (modifier.type == '?')
                        {
                            velocity *= (modifier.value / 100.0f);
                        }

                        velocity = juce::jlimit(0.0f, 1.0f, velocity);

                        midiBuffer.addEvent(
                            juce::MidiMessage::noteOn(channel, noteNumber, velocity),
                            sampleOffset
                        );
                        generatedAny = true;
                    }
                    else
                    {
                        // Note Off
                        int noteNumber = (stepNote[track][step] >= 0)
                            ? stepNote[track][step]
                            : baseNote;

                        midiBuffer.addEvent(
                            juce::MidiMessage::noteOff(channel, noteNumber),
                            sampleOffset
                        );
                    }
                }

                lastStepState[track] = isActive;
            }
        }
    }

    return generatedAny;
}

// === State Management ===

void StepSequencerClip::clearPattern()
{
    for (int t = 0; t < maxTracks; ++t)
    {
        for (int s = 0; s < maxSteps; ++s)
        {
            stepActive[t][s] = false;
            stepVelocity[t][s] = 0.8f;
            stepNote[t][s] = -1;
            stepModifiers[t][s] = StepModifier{};
        }
        lastStepState[t] = false;
    }
}

void StepSequencerClip::clearTrack(int track)
{
    if (track >= 0 && track < numTracks)
    {
        for (int s = 0; s < numSteps; ++s)
        {
            stepActive[track][s] = false;
            stepVelocity[track][s] = 0.8f;
            stepNote[track][s] = -1;
            stepModifiers[track][s] = StepModifier{};
        }
        lastStepState[track] = false;
    }
}

std::unique_ptr<StepSequencerClip> StepSequencerClip::cloneStepSequencer() const
{
    auto newClip = std::make_unique<StepSequencerClip>();

    // Copy base TimelineClip properties
    newClip->startBeat.store(startBeat.load());
    newClip->lengthBeats.store(lengthBeats.load());
    newClip->gain.store(gain.load());
    newClip->pan.store(pan.load());
    newClip->muted.store(muted.load());
    newClip->loopEnabled.store(loopEnabled.load());
    newClip->name = name + " (copy)";

    // Copy StepSequencer-specific properties
    newClip->numTracks = numTracks;
    newClip->numSteps = numSteps;
    newClip->stepsPerBeat = stepsPerBeat;

    newClip->stepActive = stepActive;
    newClip->stepVelocity = stepVelocity;
    newClip->stepNote = stepNote;
    newClip->stepModifiers = stepModifiers;
    newClip->trackBaseNote = trackBaseNote;
    newClip->trackMidiChannel = trackMidiChannel;

    return newClip;
}

// === Helpers ===

double StepSequencerClip::getStepLengthBeats() const
{
    return 1.0 / stepsPerBeat;
}

int StepSequencerClip::calculateCurrentStep(double beatWithinClip) const
{
    double stepLength = getStepLengthBeats();
    return static_cast<int>(beatWithinClip / stepLength) % numSteps;
}
