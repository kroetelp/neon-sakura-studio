#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <memory>

class TimelineClip
{
public:
    enum class Type { Audio, Midi };

    TimelineClip(Type type);
    ~TimelineClip() = default;

    // Identity
    juce::Uuid getId() const { return clipId; }
    Type getType() const { return clipType; }

    // Position (in beats)
    std::atomic<double> startBeat{0.0};
    std::atomic<double> lengthBeats{4.0};  // Default 1 bar at 4/4

    double getEndBeat() const { return startBeat.load() + lengthBeats.load(); }
    bool containsBeat(double beat) const {
        return beat >= startBeat.load() && beat < getEndBeat();
    }

    // Properties
    std::atomic<float> gain{1.0f};
    std::atomic<float> pan{0.0f};      // -1.0 to 1.0
    std::atomic<bool> muted{false};
    std::atomic<bool> loopEnabled{false};

    // Name
    juce::String name{"Clip"};

    // === Audio-specific data ===
    std::shared_ptr<juce::AudioBuffer<float>> audioBuffer;
    double audioSampleRate = 44100.0;

    // Audio setup
    void setAudioBuffer(std::shared_ptr<juce::AudioBuffer<float>> buffer, double sampleRate);
    int64_t getAudioLengthSamples() const;
    double getAudioLengthSeconds() const;

    // === MIDI-specific data ===
    struct MidiNote
    {
        int noteNumber = 60;
        float velocity = 0.8f;
        double startBeat = 0.0;      // Relative to clip start
        double lengthBeats = 1.0;
        int channel = 1;
    };
    std::vector<MidiNote> midiNotes;

    void addMidiNote(const MidiNote& note);
    void clearMidiNotes();
    const std::vector<MidiNote>& getMidiNotes() const { return midiNotes; }

    // Clone
    std::unique_ptr<TimelineClip> clone() const;

private:
    juce::Uuid clipId;
    Type clipType;
};
