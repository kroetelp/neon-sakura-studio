// ============================================================================
// StepSequencerClip.h - Step Sequencer als Timeline Clip
// ============================================================================
//
// Speichert Step-Sequencer Pattern-Daten direkt und generiert MIDI zur Laufzeit.
// Ermöglicht mehrere StepSequencer-Clips pro Track mit individueller Positionierung.

#pragma once

#include "../Timeline/TimelineClip.h"
#include <array>
#include <map>

/**
 * StepModifier - Modifier fuer einzelne Steps (TidalCycles-style)
 */
struct StepModifier
{
    char type = ' ';       // ' ' = Normal, '*' = Speed, '/' = Slow, '@' = Elongate, '!' = Replicate, '?' = Probability
    int value = 1;         // Modifier value

    // Parameter Locks (Elektron-style P-Locks)
    bool hasPitchLock = false;
    int pitchLock = 0;     // -12 to +12 semitones
    bool hasVolLock = false;
    float volLock = 1.0f;  // 0.0 to 1.0
};

/**
 * StepSequencerClip - Timeline Clip der Step Sequencer Pattern-Daten speichert
 *
 * Features:
 * - Speichert Pattern-Daten direkt (keine Konvertierung zu Audio noetig)
 * - Generiert MIDI zur Laufzeit basierend auf Playhead-Position
 * - Unterstützt Looping innerhalb des Clips
 * - Mehrere StepSequencer-Clips pro Track möglich
 * - Volle Modifier und P-Lock Unterstützung
 */
class StepSequencerClip : public TimelineClip
{
public:
    static constexpr int maxTracks = 8;
    static constexpr int maxSteps = 64;

    StepSequencerClip();
    ~StepSequencerClip() override = default;

    // === Pattern Configuration ===
    void setNumTracks(int num);
    int getNumTracks() const { return numTracks; }

    void setNumSteps(int num);
    int getNumSteps() const { return numSteps; }

    void setStepsPerBeat(int spb) { stepsPerBeat = spb; }
    int getStepsPerBeat() const { return stepsPerBeat; }

    // === Step Data Access ===
    void setStepActive(int track, int step, bool active);
    bool isStepActive(int track, int step) const;

    void setStepVelocity(int track, int step, float velocity);
    float getStepVelocity(int track, int step) const;

    void setStepNote(int track, int step, int noteNumber);
    int getStepNote(int track, int step) const;

    void setStepModifier(int track, int step, const StepModifier& modifier);
    StepModifier getStepModifier(int track, int step) const;

    // === Per-Track Settings ===
    void setTrackBaseNote(int track, int noteNumber);
    int getTrackBaseNote(int track) const;

    void setTrackMidiChannel(int track, int channel);
    int getTrackMidiChannel(int track) const;

    // === MIDI Generation ===
    // Generiert MIDI-Events fuer den gegebenen Beat-Bereich
    // Gibt true zurueck, wenn Events generiert wurden
    bool renderToMidiBuffer(
        juce::MidiBuffer& midiBuffer,
        double clipStartBeat,      // Clip-Position auf Timeline
        double currentBeat,        // Aktuelle Playhead-Position
        double endBeat,            // Ende des zu rendernden Bereichs
        double bpm,                // Tempo
        int startSample            // Sample-Offset im Buffer
    );

    // === State Management ===
    void clearPattern();
    void clearTrack(int track);

    // Clone
    std::unique_ptr<StepSequencerClip> cloneStepSequencer() const;

    // === Playback State (mutable for rendering) ===
    // Aktueller Step innerhalb des Loops (wird beim Rendering verwendet)
    mutable int currentStep = 0;
    mutable double stepAccumulator = 0.0;
    mutable bool lastStepState[maxTracks] = {false};

private:
    int numTracks = 8;
    int numSteps = 16;
    int stepsPerBeat = 4;  // 4 = 16th notes, 8 = 32nd notes

    // Pattern data: [track][step]
    std::array<std::array<bool, maxSteps>, maxTracks> stepActive{};
    std::array<std::array<float, maxSteps>, maxTracks> stepVelocity{};
    std::array<std::array<int, maxSteps>, maxTracks> stepNote{};  // -1 = use base note
    std::array<std::array<StepModifier, maxSteps>, maxTracks> stepModifiers{};

    // Per-track settings
    std::array<int, maxTracks> trackBaseNote{};     // Default: 60 (C4)
    std::array<int, maxTracks> trackMidiChannel{};  // Default: 1

    // Helper
    double getStepLengthBeats() const;
    int calculateCurrentStep(double beatWithinClip) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StepSequencerClip)
};
