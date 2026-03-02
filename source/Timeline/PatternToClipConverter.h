#pragma once

#include "TimelineClip.h"
#include "../TrackModel.h"
#include "../TrackType.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

class TrackAudioProcessor;
class TimelineData;

/**
 * PatternToClipConverter - Converts Step Sequencer patterns to TimelineClips
 *
 * Supports two conversion modes:
 * - Sampler tracks: Offline renders the triggered samples to an AudioBuffer
 * - Synth tracks: Converts active steps to MIDI notes
 */
class PatternToClipConverter
{
public:
    PatternToClipConverter();
    ~PatternToClipConverter() = default;

    /**
     * Convert a TrackModel pattern to a TimelineClip
     *
     * @param model The TrackModel containing step data
     * @param processor The TrackAudioProcessor for audio rendering
     * @param startBeat Position on timeline (in beats)
     * @param bpm Current BPM for timing calculations
     * @param sampleRate Sample rate for audio rendering
     * @return A unique_ptr to a new TimelineClip, or nullptr on failure
     */
    std::unique_ptr<TimelineClip> convertToClip(
        const TrackModel& model,
        TrackAudioProcessor& processor,
        double startBeat,
        double bpm,
        double sampleRate = 44100.0
    );

    /**
     * Convert steps to MIDI notes (for Synth tracks)
     *
     * @param model The TrackModel containing step data
     * @param clip The TimelineClip to populate with MIDI data
     * @param baseNote The MIDI note number to use (default: 60 = C4)
     * @param velocity Note velocity (default: 0.8)
     */
    void convertStepsToMidi(
        const TrackModel& model,
        TimelineClip& clip,
        int baseNote = 60,
        float velocity = 0.8f
    );

    /**
     * Render steps to audio (for Sampler tracks)
     *
     * @param model The TrackModel containing step data
     * @param processor The TrackAudioProcessor for sample rendering
     * @param clip The TimelineClip to populate with audio data
     * @param bpm Current BPM
     * @param sampleRate Sample rate for rendering
     */
    void renderStepsToAudio(
        const TrackModel& model,
        TrackAudioProcessor& processor,
        TimelineClip& clip,
        double bpm,
        double sampleRate
    );

    /**
     * Calculate the length of a clip based on loop length
     *
     * @param loopLength Number of steps in the loop (e.g., 16)
     * @param bpm Current BPM
     * @return Length in beats
     */
    static double calculateClipLengthBeats(int loopLength, double bpm);

private:
    // Helper to calculate beat position of a step
    double stepToBeat(int step, int stepsPerBeat = 4) const;
};
