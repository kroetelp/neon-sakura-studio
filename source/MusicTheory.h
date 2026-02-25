#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

// Music Theory Constants and Utilities
// Supports Western 12-tone equal temperament

namespace MusicTheory
{
    // Note names (C = 0, C# = 1, etc.)
    enum class Note : int
    {
        C = 0, Cs = 1, D = 2, Ds = 3, E = 4, F = 5,
        Fs = 6, G = 7, Gs = 8, A = 9, As = 10, B = 11
    };

    // Common scales as intervals from root (in semitones)
    struct Scale
    {
        juce::String name;
        std::vector<int> intervals;  // Intervals from root
        bool isMinor;

        // Get note at degree (0 = root, 1 = second, etc.)
        int getNoteAtDegree(int degree) const;

        // Number of notes in scale
        int getNumNotes() const { return static_cast<int>(intervals.size()); }

        // Get all notes in scale for a given root
        std::vector<int> getAllNotes(int rootNote) const;
    };

    // Chord types
    struct Chord
    {
        juce::String name;
        std::vector<int> intervals;  // Intervals from root

        // Get notes for this chord at root position
        std::vector<int> getNotes(int rootNote) const;
    };

    // Predefined scales
    namespace Scales
    {
        const Scale Major          {"Major",           {0, 2, 4, 5, 7, 9, 11}, false};
        const Scale NaturalMinor   {"Natural Minor",   {0, 2, 3, 5, 7, 8, 10}, true};
        const Scale HarmonicMinor  {"Harmonic Minor",  {0, 2, 3, 5, 7, 8, 11}, true};
        const Scale MelodicMinor   {"Melodic Minor",   {0, 2, 3, 5, 7, 9, 11}, true};
        const Scale Dorian         {"Dorian",          {0, 2, 3, 5, 7, 9, 10}, true};
        const Scale Phrygian       {"Phrygian",        {0, 1, 3, 5, 7, 8, 10}, true};
        const Scale Lydian         {"Lydian",          {0, 2, 4, 6, 7, 9, 11}, false};
        const Scale Mixolydian     {"Mixolydian",      {0, 2, 4, 5, 7, 9, 10}, false};
        const Scale Locrian        {"Locrian",         {0, 1, 3, 5, 6, 8, 10}, true};
        const Scale PentatonicMajor{"Pentatonic Major",{0, 2, 4, 7, 9}, false};
        const Scale PentatonicMinor{"Pentatonic Minor",{0, 3, 5, 7, 10}, true};
        const Scale Blues          {"Blues",           {0, 3, 5, 6, 7, 10}, true};
        const Scale Chromatic      {"Chromatic",       {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}, false};
        const Scale WholeTone      {"Whole Tone",      {0, 2, 4, 6, 8, 10}, false};
        const Scale Diminished     {"Diminished",      {0, 2, 3, 5, 6, 8, 9, 11}, false};

        // Get all scales as array
        std::vector<Scale> getAllScales();
    }

    // Predefined chords
    namespace Chords
    {
        const Chord Major         {"Major",       {0, 4, 7}};
        const Chord Minor         {"Minor",       {0, 3, 7}};
        const Chord Dim           {"Diminished",  {0, 3, 6}};
        const Chord Aug           {"Augmented",   {0, 4, 8}};
        const Chord Maj7          {"Maj7",        {0, 4, 7, 11}};
        const Chord Min7          {"Min7",        {0, 3, 7, 10}};
        const Chord Dom7          {"7",           {0, 4, 7, 10}};
        const Chord Dim7          {"Dim7",        {0, 3, 6, 9}};
        const Chord Sus2          {"Sus2",        {0, 2, 7}};
        const Chord Sus4          {"Sus4",        {0, 5, 7}};
        const Chord Add9          {"Add9",        {0, 4, 7, 14}};
        const Chord Min9          {"Min9",        {0, 3, 7, 10, 14}};

        std::vector<Chord> getAllChords();
    }

    // Chord progressions (degrees in Roman numeral notation)
    struct ChordProgression
    {
        juce::String name;
        juce::String style;
        std::vector<int> degrees;  // Scale degrees (1 = I, 4 = IV, etc.)
        std::vector<int> durations; // Duration of each chord in beats

        int getNumChords() const { return static_cast<int>(degrees.size()); }
    };

    namespace Progressions
    {
        const ChordProgression I_IV_V_I      {"Classic",        "Pop",    {1, 4, 5, 1}, {4, 4, 4, 4}};
        const ChordProgression I_V_vi_IV     {"Pop Progression","Pop",    {1, 5, 6, 4}, {4, 4, 4, 4}};
        const ChordProgression ii_V_I        {"Jazz Cadence",   "Jazz",   {2, 5, 1},    {4, 4, 4}};
        const ChordProgression I_vi_IV_V     {"50s Progression","Pop",    {1, 6, 4, 5}, {4, 4, 4, 4}};
        const ChordProgression vi_IV_I_V     {"Emotional",      "Pop",    {6, 4, 1, 5}, {4, 4, 4, 4}};
        const ChordProgression I_bVII_bVI    {"Andalusian",     "Rock",   {1, 7, 6, 5}, {4, 4, 4, 4}};
        const ChordProgression i_VII_VI_V    {"Spanish",        "Flamenco",{1, 7, 6, 5}, {4, 4, 4, 4}};
        const ChordProgression I_IV_vi_V     {"Canon",          "Classical",{1, 5, 6, 3, 4, 1, 4, 5}, {2, 2, 2, 2, 2, 2, 2, 2}};

        std::vector<ChordProgression> getAllProgressions();
    }

    // Utility functions
    juce::String noteToString(int note);
    int stringToNote(const juce::String& str);
    juce::String degreeToRomanNumeral(int degree, bool isMinor);

    // Transpose note by semitones (with wrapping)
    int transposeNote(int note, int semitones);

    // Quantize note to nearest scale degree
    int quantizeToScale(int note, int rootNote, const Scale& scale);

    // Get interval between two notes (in semitones)
    int getInterval(int note1, int note2);

    // Get interval name
    juce::String getIntervalName(int semitones);

    // MIDI note helpers
    int midiToOctave(int midiNote);
    int midiToNoteInOctave(int midiNote);
    juce::String midiToNoteName(int midiNote);

    // Frequency helpers
    double midiToFrequency(int midiNote);
    int frequencyToMidi(double frequency);
}
