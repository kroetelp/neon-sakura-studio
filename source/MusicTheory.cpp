#include "MusicTheory.h"
#include <cmath>
#include <algorithm>

namespace MusicTheory
{

//==============================================================================
// Scale Implementation
//==============================================================================

int Scale::getNoteAtDegree(int degree) const
{
    // Handle negative and wrapping degrees
    int numNotes = getNumNotes();
    int octave = degree / numNotes;
    int degInOctave = degree % numNotes;

    if (degInOctave < 0)
    {
        degInOctave += numNotes;
        octave--;
    }

    return intervals[degInOctave] + (octave * 12);
}

std::vector<int> Scale::getAllNotes(int rootNote) const
{
    std::vector<int> notes;
    notes.reserve(intervals.size());

    for (int interval : intervals)
    {
        notes.push_back((rootNote + interval) % 12);
    }

    return notes;
}

//==============================================================================
// Chord Implementation
//==============================================================================

std::vector<int> Chord::getNotes(int rootNote) const
{
    std::vector<int> notes;
    notes.reserve(intervals.size());

    for (int interval : intervals)
    {
        notes.push_back((rootNote + interval) % 12);
    }

    return notes;
}

//==============================================================================
// Scales
//==============================================================================

std::vector<Scale> Scales::getAllScales()
{
    return {
        Major, NaturalMinor, HarmonicMinor, MelodicMinor,
        Dorian, Phrygian, Lydian, Mixolydian, Locrian,
        PentatonicMajor, PentatonicMinor, Blues,
        Chromatic, WholeTone, Diminished
    };
}

//==============================================================================
// Chords
//==============================================================================

std::vector<Chord> Chords::getAllChords()
{
    return {
        Major, Minor, Dim, Aug, Maj7, Min7, Dom7, Dim7,
        Sus2, Sus4, Add9, Min9
    };
}

//==============================================================================
// Progressions
//==============================================================================

std::vector<ChordProgression> Progressions::getAllProgressions()
{
    return {
        I_IV_V_I, I_V_vi_IV, ii_V_I, I_vi_IV_V,
        vi_IV_I_V, I_bVII_bVI, i_VII_VI_V, I_IV_vi_V
    };
}

//==============================================================================
// Utility Functions
//==============================================================================

juce::String noteToString(int note)
{
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    note = ((note % 12) + 12) % 12;  // Ensure 0-11
    return noteNames[note];
}

int stringToNote(const juce::String& str)
{
    static const char* noteNames[] = {
        "C", "C#", "DB", "D", "D#", "EB", "E", "F", "F#", "GB",
        "G", "G#", "AB", "A", "A#", "BB", "B"
    };
    static const int noteValues[] = {
        0, 1, 1, 2, 3, 3, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10, 11
    };

    juce::String upper = str.toUpperCase().trim();

    for (int i = 0; i < 17; ++i)
    {
        if (upper == noteNames[i])
            return noteValues[i];
    }

    return 0;  // Default to C
}

juce::String degreeToRomanNumeral(int degree, bool isMinor)
{
    static const char* majorNumerals[] = {"I", "ii", "iii", "IV", "V", "vi", "viio"};
    static const char* minorNumerals[] = {"i", "iio", "III", "iv", "V/v", "VI", "VII"};

    int idx = (degree - 1) % 7;
    if (idx < 0) idx += 7;

    return isMinor ? minorNumerals[idx] : majorNumerals[idx];
}

int transposeNote(int note, int semitones)
{
    return ((note + semitones) % 12 + 12) % 12;
}

int quantizeToScale(int note, int rootNote, const Scale& scale)
{
    note = ((note % 12) + 12) % 12;
    int relativeNote = ((note - rootNote) % 12 + 12) % 12;

    // Find closest scale degree
    int closestNote = scale.intervals[0];
    int minDistance = 12;

    for (int interval : scale.intervals)
    {
        int distance = std::abs(interval - relativeNote);
        int distanceWrap = std::min(distance, 12 - distance);

        if (distanceWrap < minDistance)
        {
            minDistance = distanceWrap;
            closestNote = interval;
        }
    }

    return (rootNote + closestNote) % 12;
}

int getInterval(int note1, int note2)
{
    int interval = (note2 - note1) % 12;
    if (interval < 0) interval += 12;
    return interval;
}

juce::String getIntervalName(int semitones)
{
    static const char* intervalNames[] = {
        "Unison", "m2", "M2", "m3", "M3", "P4",
        "Tri", "P5", "m6", "M6", "m7", "M7"
    };

    semitones = ((semitones % 12) + 12) % 12;
    return intervalNames[semitones];
}

int midiToOctave(int midiNote)
{
    return (midiNote / 12) - 1;
}

int midiToNoteInOctave(int midiNote)
{
    return midiNote % 12;
}

juce::String midiToNoteName(int midiNote)
{
    return noteToString(midiToNoteInOctave(midiNote)) + juce::String(midiToOctave(midiNote));
}

double midiToFrequency(int midiNote)
{
    // A4 = 440 Hz = MIDI note 69
    return 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
}

int frequencyToMidi(double frequency)
{
    return static_cast<int>(69 + 12.0 * std::log2(frequency / 440.0));
}

}  // namespace MusicTheory
