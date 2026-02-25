#pragma once

#include "MusicTheory.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <random>
#include <functional>

// Forward declaration
struct StepModifierState;

// Melodic pattern types
enum class MelodyType
{
    Scale,          // Ascending/descending scale
    Arpeggio,       // Chord tones only
    Random,         // Random scale notes
    Contour,        // Melodic contour (up-down patterns)
    CallResponse,   // Call and response pattern
    Ostinato,       // Repeating motif
    Sequence        // Transposed sequence
};

// Melody generation parameters
struct MelodyParams
{
    int rootNote = 0;           // Root note (0=C, 1=C#, etc.)
    int octave = 4;             // Base octave (MIDI 48-59 = C4-B4)
    int octaveRange = 1;        // How many octaves to span

    // Scale selection
    int scaleIndex = 0;         // Index into Scales::getAllScales()

    // Pattern settings
    MelodyType type = MelodyType::Random;
    int density = 50;           // Note density 0-100
    int variation = 30;         // Variation amount 0-100

    // Rhythm settings
    bool syncopate = false;     // Add syncopation
    int noteLengthBias = 50;    // 0=short notes, 100=long notes

    // Motion settings
    int maxLeap = 5;            // Maximum interval leap (semitones)
    float stepProbability = 0.7f;  // Probability of stepwise motion

    // Chord settings (for arpeggios)
    int chordIndex = 0;         // Index into Chords::getAllChords()
    bool arpeggiateUp = true;
    bool arpeggiateDown = false;  // If both false, random chord tones
};

// Generated melody note
struct MelodyNote
{
    int midiNote;               // MIDI note number
    int step;                   // Step position (0-63)
    int duration;               // Duration in steps
    float velocity;             // Velocity 0.0-1.0
    bool isGhost;               // Ghost note (lower velocity)
};

class MelodyGenerator
{
public:
    MelodyGenerator();

    // Generate a melody for the given number of steps
    std::vector<MelodyNote> generateMelody(const MelodyParams& params, int numSteps = 16);

    // Generate arpeggio pattern
    std::vector<MelodyNote> generateArpeggio(const MelodyParams& params, int numSteps = 16);

    // Generate chord progression melody
    std::vector<MelodyNote> generateChordProgression(
        const MusicTheory::ChordProgression& progression,
        const MelodyParams& params,
        int beatsPerChord = 4
    );

    // Generate bass line
    std::vector<MelodyNote> generateBassLine(const MelodyParams& params, int numSteps = 16);

    // Generate lead line (more expressive, varied)
    std::vector<MelodyNote> generateLeadLine(const MelodyParams& params, int numSteps = 16);

    // Set random seed for reproducibility
    void setSeed(unsigned int seed);

    // Get available scales/chords for UI
    static juce::StringArray getScaleNames();
    static juce::StringArray getChordNames();
    static juce::StringArray getProgressionNames();
    static juce::StringArray getMelodyTypeNames();

private:
    std::mt19937 rng;

    // Helper methods
    int getScaleNote(const MelodyParams& params, int degree, int octaveOffset = 0);
    int quantizeToScale(int midiNote, const MelodyParams& params);

    // Melodic contour shapes
    enum class ContourShape { Ascending, Descending, Arch, Valley, Wave, Random };
    ContourShape generateContourShape();

    // Generate specific pattern types
    std::vector<MelodyNote> generateScalePattern(const MelodyParams& params, int numSteps);
    std::vector<MelodyNote> generateContourPattern(const MelodyParams& params, int numSteps);
    std::vector<MelodyNote> generateCallResponse(const MelodyParams& params, int numSteps);
    std::vector<MelodyNote> generateOstinato(const MelodyParams& params, int numSteps);
    std::vector<MelodyNote> generateSequence(const MelodyParams& params, int numSteps);

    // Rhythm generation
    std::vector<int> generateRhythm(const MelodyParams& params, int numSteps);

    // Velocity shaping
    float shapeVelocity(int step, int totalSteps, const MelodyParams& params);
};

// Melody to StepModifierState converter
class MelodyConverter
{
public:
    // Convert melody notes to step states for a track
    // Returns pairs of (step, pitchOffset) where pitchOffset is relative to base pitch
    static std::vector<std::pair<int, int>> melodyToSteps(
        const std::vector<MelodyNote>& melody,
        int basePitch = 60  // Middle C
    );

    // Create pattern from chord tones
    static std::vector<int> chordToSteps(
        const MusicTheory::Chord& chord,
        int rootNote,
        int numSteps,
        int pattern = 0  // Different arpeggio patterns
    );
};
