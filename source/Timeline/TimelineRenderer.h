#pragma once

#include "TimelineData.h"
#include "TimelineTransport.h"
#include "../WavetableSynth/WavetableEngine.h"
#include "../StepSequencer/StepSequencerClip.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <array>

class TimelineRenderer
{
public:
    TimelineRenderer(TimelineData& data, TimelineTransport& transport);
    ~TimelineRenderer() = default;

    // Set the wavetable engine for MIDI clip playback
    void setWavetableEngine(WavetableEngine* engine) { wavetableEngine = engine; }

    // Prepare for playback (call from AudioEngine::prepareToPlay)
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    // Process audio block (call from AudioEngine audio thread)
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // Reset renderer state
    void reset();

private:
    TimelineData& timelineData;
    TimelineTransport& transport;
    WavetableEngine* wavetableEngine = nullptr;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Per-track render state
    struct ClipRenderState
    {
        bool isActive = false;
        int64_t positionInClip = 0;  // In samples
    };

    // State for active clips (reused between calls)
    std::array<std::vector<std::pair<TimelineClip*, ClipRenderState>>, TimelineData::numTracks> activeClips;

    // Internal buffer for mixing
    juce::AudioBuffer<float> mixBuffer;

    // Helper methods
    void updateActiveClips(double currentBeat);
    void renderAudioClip(juce::AudioBuffer<float>& buffer, TimelineClip& clip, ClipRenderState& state);
    void renderMidiClip(juce::MidiBuffer& midiBuffer, TimelineClip& clip, double startBeat, int numSamples);
    void renderStepSequencerClip(juce::MidiBuffer& midiBuffer, StepSequencerClip& clip,
                                  double currentBeat, double endBeat, double bpm);
    void applyTrackMixing(juce::AudioBuffer<float>& buffer, int trackIndex);

    double samplesToBeats(int64_t samples) const;
    int64_t beatsToSamples(double beats) const;
};
