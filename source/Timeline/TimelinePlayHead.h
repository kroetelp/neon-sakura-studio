#pragma once

/**
 * TimelinePlayHead - Implements juce::AudioPlayHead for VST plugin tempo/position sync
 *
 * This class provides timing information to plugins so they can sync to the
 * timeline's tempo, position, time signature, and loop points.
 *
 * Thread Safety:
 * - All data is read from atomic variables in TimelineData
 * - Safe to call from the audio thread
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>

class TimelineData;
class TimelineTransport;

class TimelinePlayHead : public juce::AudioPlayHead
{
public:
    /**
     * Construct a TimelinePlayHead with references to timeline data and transport.
     * Both references must remain valid for the lifetime of this object.
     */
    TimelinePlayHead(TimelineData& data, TimelineTransport& transport);
    ~TimelinePlayHead() override = default;

    /**
     * Get the current position information for plugins.
     * This is called by plugins to sync with the timeline.
     *
     * Returns Optional<PositionInfo> containing:
     * - Time in seconds
     * - BPM
     * - Time signature
     * - Play/Record state
     * - Loop points
     * - Bar/Beat position (PPQ)
     */
    juce::Optional<PositionInfo> getPosition() const override;

    /**
     * Check if the playhead can control the timeline (we don't allow plugin control).
     * Returns false - plugins cannot control our timeline transport.
     */
    bool canControlTransport() override { return false; }

    /**
     * Transport control - not implemented as plugins cannot control our transport.
     * These are no-ops as we don't allow plugins to start/stop playback.
     */
    void transportPlay(bool /*shouldPlay*/) override {}
    void transportRecord(bool /*shouldRecord*/) override {}
    void transportRewind() override {}

    /**
     * Set the sample rate for accurate time-in-samples calculation.
     * Called from AudioEngine::prepareToPlay().
     */
    void setSampleRate(double sampleRate) { currentSampleRate = sampleRate; }

private:
    TimelineData& timelineData;
    TimelineTransport& timelineTransport;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelinePlayHead)
};
