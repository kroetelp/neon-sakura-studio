#pragma once

/**
 * ITrackDataProvider - Interface for AudioEngine to access track data
 *
 * This interface abstracts track data access, allowing AudioEngine to operate
 * without direct coupling to the TrackComponent array owned by MainComponent.
 *
 * Uses Dependency Injection pattern to decouple AudioEngine from MainComponent.
 */

#include "TrackType.h"
#include <juce_audio_basics/juce_audio_basics.h>

struct StepModifierState;
class WavetableSynth;

class ITrackDataProvider
{
public:
    virtual ~ITrackDataProvider() = default;

    // Synthesiser access for sample playback
    virtual juce::Synthesiser& getSynthesiser(int trackIndex) = 0;

    // Wavetable synth access for wavetable tracks
    virtual WavetableSynth* getWavetableSynth(int trackIndex) = 0;

    // Step data access (read-only for audio thread)
    virtual bool isStepActive(int trackIndex, int step) const = 0;
    virtual StepModifierState getStepState(int trackIndex, int step) const = 0;

    // Track control values
    virtual float getVolume(int trackIndex) const = 0;
    virtual int getPitch(int trackIndex) const = 0;
    virtual float getCutoff(int trackIndex) const = 0;
    virtual bool getMuted(int trackIndex) const = 0;
    virtual bool getSolo(int trackIndex) const = 0;

    // Wavetable modulation
    virtual bool getWavetableModulationEnabled(int trackIndex) const = 0;

    // Track configuration
    virtual int getTrackLoopLength(int trackIndex) const = 0;
    virtual TrackType getTrackType(int trackIndex) const = 0;

    // Audio processing (for wavetable tracks)
    virtual void processAudioBlock(int trackIndex, juce::AudioBuffer<float>& buffer) = 0;

    // Playhead position (for automation)
    virtual double getPlayheadBeat() const = 0;
    virtual void setPlayheadBeat(double beat) = 0;
};
