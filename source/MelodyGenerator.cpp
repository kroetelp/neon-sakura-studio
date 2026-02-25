#include "MelodyGenerator.h"
#include <algorithm>
#include <cmath>

MelodyGenerator::MelodyGenerator()
{
    std::random_device rd;
    rng.seed(rd());
}

void MelodyGenerator::setSeed(unsigned int seed)
{
    rng.seed(seed);
}

std::vector<MelodyNote> MelodyGenerator::generateMelody(const MelodyParams& params, int numSteps)
{
    switch (params.type)
    {
        case MelodyType::Scale:
            return generateScalePattern(params, numSteps);
        case MelodyType::Arpeggio:
            return generateArpeggio(params, numSteps);
        case MelodyType::Contour:
            return generateContourPattern(params, numSteps);
        case MelodyType::CallResponse:
            return generateCallResponse(params, numSteps);
        case MelodyType::Ostinato:
            return generateOstinato(params, numSteps);
        case MelodyType::Sequence:
            return generateSequence(params, numSteps);
        case MelodyType::Random:
        default:
            break;
    }

    // Random melody generation
    std::vector<MelodyNote> melody;

    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];

    // Generate rhythm first
    auto rhythm = generateRhythm(params, numSteps);

    int lastNote = params.rootNote + params.octave * 12;
    std::uniform_real_distribution<float> probDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> octaveDist(-params.octaveRange, params.octaveRange);

    for (int step : rhythm)
    {
        MelodyNote note;
        note.step = step;
        note.duration = 1;
        note.isGhost = probDist(rng) < 0.15f;
        note.velocity = shapeVelocity(step, numSteps, params);

        if (note.isGhost)
            note.velocity *= 0.5f;

        // Determine next note
        if (probDist(rng) < params.stepProbability)
        {
            // Stepwise motion
            int direction = (probDist(rng) < 0.5f) ? 1 : -1;
            int currentDegree = 0;

            // Find current degree in scale
            int relativeNote = (lastNote - params.rootNote - params.octave * 12) % 12;
            for (size_t i = 0; i < scale.intervals.size(); ++i)
            {
                if (scale.intervals[i] == relativeNote)
                {
                    currentDegree = static_cast<int>(i);
                    break;
                }
            }

            int newDegree = (currentDegree + direction + static_cast<int>(scale.intervals.size()))
                          % static_cast<int>(scale.intervals.size());
            note.midiNote = params.rootNote + params.octave * 12 + scale.intervals[newDegree];
        }
        else
        {
            // Leap to random scale note
            int octaveOffset = octaveDist(rng);
            int degreeIndex = std::uniform_int_distribution<int>(0, static_cast<int>(scale.intervals.size()) - 1)(rng);
            note.midiNote = params.rootNote + (params.octave + octaveOffset) * 12 + scale.intervals[degreeIndex];

            // Limit leap size
            int leap = std::abs(note.midiNote - lastNote);
            if (leap > params.maxLeap)
            {
                // Reduce leap
                note.midiNote = lastNote + (note.midiNote > lastNote ? params.maxLeap : -params.maxLeap);
                note.midiNote = quantizeToScale(note.midiNote, params);
            }
        }

        lastNote = note.midiNote;
        melody.push_back(note);
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateArpeggio(const MelodyParams& params, int numSteps)
{
    std::vector<MelodyNote> melody;

    auto chords = MusicTheory::Chords::getAllChords();
    const auto& chord = chords[params.chordIndex];

    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];

    // Get chord tones
    std::vector<int> chordTones;
    for (int interval : chord.intervals)
    {
        chordTones.push_back(params.rootNote + params.octave * 12 + interval);
    }

    int noteIndex = 0;

    for (int step = 0; step < numSteps; ++step)
    {
        // Density check
        std::uniform_int_distribution<int> densityCheck(0, 100);
        if (densityCheck(rng) > params.density)
            continue;

        MelodyNote note;
        note.step = step;
        note.duration = 1;
        note.velocity = shapeVelocity(step, numSteps, params);
        note.isGhost = false;

        if (params.arpeggiateUp && !params.arpeggiateDown)
        {
            // Ascending arpeggio
            note.midiNote = chordTones[noteIndex % static_cast<int>(chordTones.size())];
            noteIndex++;
        }
        else if (params.arpeggiateDown && !params.arpeggiateUp)
        {
            // Descending arpeggio
            note.midiNote = chordTones[static_cast<int>(chordTones.size()) - 1 - (noteIndex % static_cast<int>(chordTones.size()))];
            noteIndex++;
        }
        else
        {
            // Random chord tone
            std::uniform_int_distribution<int> toneDist(0, static_cast<int>(chordTones.size()) - 1);
            note.midiNote = chordTones[toneDist(rng)];
        }

        melody.push_back(note);
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateBassLine(const MelodyParams& params, int numSteps)
{
    MelodyParams bassParams = params;
    bassParams.octave = 2;  // Bass octave
    bassParams.octaveRange = 0;  // Stay in one octave
    bassParams.maxLeap = 7;  // Allow larger leaps
    bassParams.stepProbability = 0.4f;  // More leaps for bass
    bassParams.density = 60;  // Moderate density
    bassParams.type = MelodyType::Random;

    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[bassParams.scaleIndex];

    std::vector<MelodyNote> bassLine;

    // Emphasize root and 5th
    for (int step = 0; step < numSteps; ++step)
    {
        std::uniform_int_distribution<int> densityCheck(0, 100);
        if (densityCheck(rng) > bassParams.density)
            continue;

        MelodyNote note;
        note.step = step;
        note.duration = 1;
        note.velocity = shapeVelocity(step, numSteps, bassParams);
        note.isGhost = false;

        // Strong beats emphasize root
        if (step % 4 == 0)
        {
            note.midiNote = params.rootNote + bassParams.octave * 12;  // Root
        }
        else if (step % 4 == 2 && bassParams.density > 40)
        {
            // 5th on beat 3 sometimes
            int fifth = 0;
            for (int interval : scale.intervals)
            {
                if (interval == 7) fifth = interval;
            }
            note.midiNote = params.rootNote + bassParams.octave * 12 + fifth;
        }
        else
        {
            // Random scale note
            std::uniform_int_distribution<int> degreeDist(0, static_cast<int>(scale.intervals.size()) - 1);
            note.midiNote = params.rootNote + bassParams.octave * 12 + scale.intervals[degreeDist(rng)];
        }

        bassLine.push_back(note);
    }

    return bassLine;
}

std::vector<MelodyNote> MelodyGenerator::generateLeadLine(const MelodyParams& params, int numSteps)
{
    MelodyParams leadParams = params;
    leadParams.octaveRange = 2;  // Two octave range
    leadParams.variation = juce::jmax(params.variation, 50);
    leadParams.stepProbability = 0.6f;
    leadParams.type = MelodyType::Contour;

    return generateContourPattern(leadParams, numSteps);
}

std::vector<MelodyNote> MelodyGenerator::generateChordProgression(
    const MusicTheory::ChordProgression& progression,
    const MelodyParams& params,
    int beatsPerChord)
{
    std::vector<MelodyNote> melody;

    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];
    auto chords = MusicTheory::Chords::getAllChords();

    int step = 0;
    for (size_t i = 0; i < progression.degrees.size(); ++i)
    {
        int degree = progression.degrees[i] - 1;  // Convert to 0-indexed
        int chordRoot = (params.rootNote + scale.intervals[degree % static_cast<int>(scale.intervals.size())]) % 12;

        // Determine if major or minor chord based on scale degree
        const MusicTheory::Chord& chordType = (degree == 1 || degree == 2 || degree == 5) ?
            MusicTheory::Chords::Minor : MusicTheory::Chords::Major;

        // Generate arpeggio for this chord
        std::vector<int> chordTones;
        for (int interval : chordType.intervals)
        {
            chordTones.push_back(chordRoot + params.octave * 12 + interval);
        }

        // Add notes for the chord duration
        int chordSteps = beatsPerChord;
        if (i < progression.durations.size())
            chordSteps = progression.durations[i];

        for (int s = 0; s < chordSteps && step < 64; ++s)
        {
            MelodyNote note;
            note.step = step;
            note.duration = 1;
            note.velocity = shapeVelocity(step, progression.getNumChords() * beatsPerChord, params);
            note.isGhost = false;

            // Cycle through chord tones
            note.midiNote = chordTones[s % static_cast<int>(chordTones.size())];

            melody.push_back(note);
            step++;
        }
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateScalePattern(const MelodyParams& params, int numSteps)
{
    std::vector<MelodyNote> melody;

    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];

    int direction = 1;  // 1 = up, -1 = down
    int degree = 0;
    int maxDegree = static_cast<int>(scale.intervals.size()) * params.octaveRange;

    for (int step = 0; step < numSteps; ++step)
    {
        std::uniform_int_distribution<int> densityCheck(0, 100);
        if (densityCheck(rng) > params.density)
            continue;

        MelodyNote note;
        note.step = step;
        note.duration = 1;
        note.velocity = shapeVelocity(step, numSteps, params);
        note.isGhost = false;

        note.midiNote = getScaleNote(params, degree);

        melody.push_back(note);

        degree += direction;
        if (degree >= maxDegree)
        {
            direction = -1;
            degree = maxDegree - 1;
        }
        else if (degree < 0)
        {
            direction = 1;
            degree = 0;
        }
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateContourPattern(const MelodyParams& params, int numSteps)
{
    std::vector<MelodyNote> melody;

    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];

    ContourShape shape = generateContourShape();

    auto rhythm = generateRhythm(params, numSteps);
    int totalNotes = static_cast<int>(rhythm.size());

    int degree = 0;
    int maxDegree = static_cast<int>(scale.intervals.size()) * params.octaveRange;

    for (size_t i = 0; i < rhythm.size(); ++i)
    {
        int step = rhythm[i];
        float position = static_cast<float>(i) / totalNotes;

        MelodyNote note;
        note.step = step;
        note.duration = 1;
        note.velocity = shapeVelocity(step, numSteps, params);
        note.isGhost = false;

        // Calculate degree based on contour shape
        switch (shape)
        {
            case ContourShape::Ascending:
                degree = static_cast<int>(position * maxDegree);
                break;
            case ContourShape::Descending:
                degree = static_cast<int>((1.0f - position) * maxDegree);
                break;
            case ContourShape::Arch:
                degree = static_cast<int>((1.0f - std::abs(2.0f * position - 1.0f)) * maxDegree);
                break;
            case ContourShape::Valley:
                degree = static_cast<int>(std::abs(2.0f * position - 1.0f) * maxDegree);
                break;
            case ContourShape::Wave:
                degree = static_cast<int>(std::sin(position * 6.28f) * maxDegree / 2 + maxDegree / 2);
                break;
            case ContourShape::Random:
            default:
                std::uniform_int_distribution<int> degreeDist(0, maxDegree);
                degree = degreeDist(rng);
                break;
        }

        note.midiNote = getScaleNote(params, degree);
        melody.push_back(note);
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateCallResponse(const MelodyParams& params, int numSteps)
{
    std::vector<MelodyNote> melody;

    // Generate "call" phrase
    int halfSteps = numSteps / 2;
    auto callParams = params;
    callParams.density = juce::jmin(params.density + 10, 100);
    auto call = generateMelody(callParams, halfSteps);

    // Copy call notes with adjusted step positions
    for (const auto& note : call)
    {
        melody.push_back(note);
    }

    // Generate "response" - similar but ending differently
    auto response = generateMelody(params, halfSteps);

    // Transpose response slightly and adjust
    std::uniform_int_distribution<int> transposeDist(-2, 2);
    int transpose = 0;

    for (auto& note : response)
    {
        note.step += halfSteps;

        // Last note should resolve to root or 5th
        if (note.step >= numSteps - 2)
        {
            auto scales = MusicTheory::Scales::getAllScales();
            const auto& scale = scales[params.scaleIndex];

            // Resolve to root
            note.midiNote = params.rootNote + params.octave * 12;
        }
        else
        {
            note.midiNote = quantizeToScale(note.midiNote + transpose, params);
        }

        melody.push_back(note);
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateOstinato(const MelodyParams& params, int numSteps)
{
    std::vector<MelodyNote> melody;

    // Generate a short motif (4-8 steps)
    std::uniform_int_distribution<int> motifLengthDist(4, 8);
    int motifLength = motifLengthDist(rng);

    auto motif = generateMelody(params, motifLength);

    // Repeat motif across the pattern
    int step = 0;
    while (step < numSteps)
    {
        for (const auto& note : motif)
        {
            if (step + note.step < numSteps)
            {
                MelodyNote repeatNote = note;
                repeatNote.step = step + note.step;
                melody.push_back(repeatNote);
            }
        }
        step += motifLength;
    }

    return melody;
}

std::vector<MelodyNote> MelodyGenerator::generateSequence(const MelodyParams& params, int numSteps)
{
    std::vector<MelodyNote> melody;

    // Generate a short phrase
    int phraseLength = numSteps / 4;
    auto phrase = generateMelody(params, phraseLength);

    // Repeat with transposition (sequence)
    int transpositions[] = {0, 2, 4, 5};  // Up by scale degrees

    for (int seq = 0; seq < 4; ++seq)
    {
        int transposeSemitones = 0;

        if (seq > 0)
        {
            auto scales = MusicTheory::Scales::getAllScales();
            const auto& scale = scales[params.scaleIndex];
            int degree = transpositions[seq] % static_cast<int>(scale.intervals.size());
            transposeSemitones = scale.intervals[degree];
        }

        for (const auto& note : phrase)
        {
            MelodyNote seqNote = note;
            seqNote.step = seq * phraseLength + note.step;

            if (seqNote.step < numSteps)
            {
                seqNote.midiNote = quantizeToScale(note.midiNote + transposeSemitones, params);
                melody.push_back(seqNote);
            }
        }
    }

    return melody;
}

std::vector<int> MelodyGenerator::generateRhythm(const MelodyParams& params, int numSteps)
{
    std::vector<int> rhythm;

    // Start with strong beats
    for (int step = 0; step < numSteps; step += 4)
    {
        rhythm.push_back(step);
    }

    // Add offbeats based on density
    std::uniform_int_distribution<int> densityCheck(0, 100);

    for (int step = 0; step < numSteps; ++step)
    {
        if (step % 4 == 0) continue;  // Already added

        int threshold = params.density;
        if (params.syncopate && step % 4 == 2)
            threshold += 20;  // More likely on beat 3
        if (params.syncopate && step % 2 == 1)
            threshold += 10;  // Syncopation on offbeats

        if (densityCheck(rng) < threshold)
            rhythm.push_back(step);
    }

    std::sort(rhythm.begin(), rhythm.end());
    return rhythm;
}

float MelodyGenerator::shapeVelocity(int step, int totalSteps, const MelodyParams& params)
{
    // Base velocity
    float velocity = 0.7f;

    // Emphasize downbeats
    if (step % 4 == 0)
        velocity += 0.15f;

    // Slight crescendo towards middle/end
    float position = static_cast<float>(step) / totalSteps;
    velocity += (0.5f - std::abs(position - 0.5f)) * 0.1f;

    // Add some randomness
    std::uniform_real_distribution<float> velDist(-0.1f, 0.1f);
    velocity += velDist(rng);

    return juce::jlimit(0.3f, 1.0f, velocity);
}

int MelodyGenerator::getScaleNote(const MelodyParams& params, int degree, int octaveOffset)
{
    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];

    int numNotesInScale = static_cast<int>(scale.intervals.size());
    int octave = params.octave + octaveOffset + (degree / numNotesInScale);
    int degreeInOctave = degree % numNotesInScale;

    return params.rootNote + octave * 12 + scale.intervals[degreeInOctave];
}

int MelodyGenerator::quantizeToScale(int midiNote, const MelodyParams& params)
{
    auto scales = MusicTheory::Scales::getAllScales();
    const auto& scale = scales[params.scaleIndex];

    int octave = midiNote / 12;
    int noteInOctave = midiNote % 12;

    // Find closest scale note
    int closestInterval = scale.intervals[0];
    int minDistance = 12;

    for (int interval : scale.intervals)
    {
        int distance = std::abs(interval - noteInOctave);
        distance = std::min(distance, 12 - distance);

        if (distance < minDistance)
        {
            minDistance = distance;
            closestInterval = interval;
        }
    }

    return octave * 12 + closestInterval + params.rootNote;
}

MelodyGenerator::ContourShape MelodyGenerator::generateContourShape()
{
    std::uniform_int_distribution<int> shapeDist(0, 5);
    return static_cast<ContourShape>(shapeDist(rng));
}

//==============================================================================
// MelodyConverter Implementation
//==============================================================================

std::vector<std::pair<int, int>> MelodyConverter::melodyToSteps(
    const std::vector<MelodyNote>& melody,
    int basePitch)
{
    std::vector<std::pair<int, int>> steps;
    steps.reserve(melody.size());

    for (const auto& note : melody)
    {
        int pitchOffset = note.midiNote - basePitch;
        steps.emplace_back(note.step, pitchOffset);
    }

    return steps;
}

std::vector<int> MelodyConverter::chordToSteps(
    const MusicTheory::Chord& chord,
    int rootNote,
    int numSteps,
    int pattern)
{
    std::vector<int> steps;
    auto notes = chord.getNotes(rootNote);

    if (notes.empty()) return steps;

    switch (pattern)
    {
        case 0:  // Root only
            for (int i = 0; i < numSteps; i += 4)
                steps.push_back(notes[0]);
            break;

        case 1:  // Root-5th
            for (int i = 0; i < numSteps; i += 4)
            {
                steps.push_back(notes[0]);
                if (i + 2 < numSteps)
                    steps.push_back(notes.size() > 2 ? notes[2] : notes[0]);
            }
            break;

        case 2:  // Full arpeggio up
            for (int i = 0; i < numSteps; ++i)
                steps.push_back(notes[i % notes.size()]);
            break;

        case 3:  // Full arpeggio up-down
            {
                bool goingUp = true;
                int idx = 0;
                for (int i = 0; i < numSteps; ++i)
                {
                    steps.push_back(notes[idx]);
                    if (goingUp)
                    {
                        idx++;
                        if (idx >= static_cast<int>(notes.size()) - 1)
                            goingUp = false;
                    }
                    else
                    {
                        idx--;
                        if (idx <= 0)
                            goingUp = true;
                    }
                }
            }
            break;

        default:  // Random
            for (int i = 0; i < numSteps; ++i)
                steps.push_back(notes[std::rand() % notes.size()]);
            break;
    }

    return steps;
}

//==============================================================================
// Static Helper Functions
//==============================================================================

juce::StringArray MelodyGenerator::getScaleNames()
{
    juce::StringArray names;
    for (const auto& scale : MusicTheory::Scales::getAllScales())
        names.add(scale.name);
    return names;
}

juce::StringArray MelodyGenerator::getChordNames()
{
    juce::StringArray names;
    for (const auto& chord : MusicTheory::Chords::getAllChords())
        names.add(chord.name);
    return names;
}

juce::StringArray MelodyGenerator::getProgressionNames()
{
    juce::StringArray names;
    for (const auto& prog : MusicTheory::Progressions::getAllProgressions())
        names.add(prog.name + " (" + prog.style + ")");
    return names;
}

juce::StringArray MelodyGenerator::getMelodyTypeNames()
{
    return {
        "Random",
        "Scale",
        "Arpeggio",
        "Contour",
        "Call & Response",
        "Ostinato",
        "Sequence"
    };
}
