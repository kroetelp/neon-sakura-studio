#include "PatternGenerator.h"
#include "TrackManager.h"
#include "ISampleProvider.h"
#include "TrackComponent.h"
#include <chrono>

PatternGenerator::PatternGenerator(
    TrackManager& tracks,
    ISampleProvider& samples,
    std::function<void(double)> bpmCallback
)
    : trackManager(tracks)
    , sampleProvider(samples)
    , onBpmChanged(std::move(bpmCallback))
{
}

void PatternGenerator::setBPM(double newBpm)
{
    if (onBpmChanged)
        onBpmChanged(newBpm);
}

// Helper function to check if category is available and load it
bool PatternGenerator::setTrackCategoryIfAvailable(int trackIdx, const juce::String& category)
{
    if (trackIdx >= numTracks) return false;

    juce::StringArray categories = sampleProvider.getSampleCategories();
    for (const auto& cat : categories)
    {
        if (cat == category)
        {
            trackManager.getTrack(trackIdx).setSelectedCategory(category);
            sampleProvider.loadSampleForCategory(trackIdx, category);
            return true;
        }
    }
    return false;
}

// Helper function to set multiple steps on a track
void PatternGenerator::setStepsOnTrack(int trackIdx, const std::vector<int>& steps)
{
    if (trackIdx >= numTracks) return;

    for (int step : steps)
    {
        if (step >= 0 && step < totalSteps)
            trackManager.getTrack(trackIdx).setStepActive(step, true);
    }
}

// Set step modifier helper
void PatternGenerator::setStepModifier(int trackIdx, int step, char type, int value)
{
    if (trackIdx >= numTracks) return;
    trackManager.getTrack(trackIdx).setStepModifier(step, type, value);
}

// Euclidean rhythm generator - creates evenly distributed patterns
std::vector<int> PatternGenerator::getEuclidean(int pulses, int steps, int offset)
{
    std::vector<int> result;
    if (pulses <= 0 || steps <= 0 || pulses > steps) return result;

    for (int i = 0; i < steps; ++i)
    {
        if (((i * pulses) % steps) < pulses)
            result.push_back((i + offset) % steps);
    }
    return result;
}

// Helper to fully clear a track (steps and P-Locks)
void PatternGenerator::clearTrackFully(int trackIdx)
{
    if (trackIdx >= numTracks) return;

    for (int step = 0; step < totalSteps; ++step)
    {
        StepModifierState state = trackManager.getTrack(trackIdx).getStepState(step);
        state.active = false;
        state.modifierType = ' ';
        state.modifierValue = 1;
        state.hasPitchLock = false;
        state.hasVolLock = false;
        trackManager.getTrack(trackIdx).setStepState(step, state);
    }
}

void PatternGenerator::clearAllTracks()
{
    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        clearTrackFully(trackIdx);
        trackManager.getTrack(trackIdx).setMuted(false);
        trackManager.getTrack(trackIdx).setSolo(false);
    }
}

// Euclidean Algorithmic Song Generation with P-Locks, Polyrhythms & Modifiers
// Each generation creates UNIQUE patterns!

void PatternGenerator::generateSong(Genre genre)
{
    // Clear all tracks fully (steps + P-Locks)
    clearAllTracks();

    // Random generator with TIME-based seed for unique patterns every time!
    unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
    std::mt19937 rng(seed);

    switch (genre)
    {
        case Genre::TECHNO:
            generateTechno(rng);
            break;
        case Genre::HOUSE:
            generateHouse(rng);
            break;
        case Genre::TRAP:
            generateTrapDrill(rng);
            break;
        case Genre::DNB:
            generateDnB(rng);
            break;
        case Genre::AMBIENT:
            generateAmbient(rng);
            break;
        case Genre::GARAGE:
            generateIdmGlitch(rng);
            break;
    }
}

void PatternGenerator::generateTechno(std::mt19937& rng)
{
    std::uniform_int_distribution<int> boolDist(0, 1);
    std::uniform_int_distribution<int> smallDist(0, 2);
    std::uniform_int_distribution<int> mediumDist(0, 3);
    std::uniform_int_distribution<int> pitchDist(-12, 5);
    std::uniform_int_distribution<int> styleDist(0, 3);
    std::uniform_int_distribution<int> offsetDist(0, 15);

    int bpmVal = 128 + mediumDist(rng) * 4;  // 128-140 BPM
    setBPM(bpmVal);

    int kickStyle = styleDist(rng);

    // Track 0: Kick - Various 4-on-the-floor variations
    setTrackCategoryIfAvailable(0, "bd");
    if (kickStyle == 0)
        setStepsOnTrack(0, {0, 4, 8, 12});
    else if (kickStyle == 1)
        setStepsOnTrack(0, {0, 4, 8, 12, 14});
    else if (kickStyle == 2)
        setStepsOnTrack(0, {0, 4, 6, 8, 12});
    else
        setStepsOnTrack(0, {0, 3, 4, 8, 11, 12});

    // Track 1: Clap - Random Euclidean
    setTrackCategoryIfAvailable(1, "cp");
    int clapPulse = 1 + boolDist(rng);
    int clapOffset = offsetDist(rng);
    setStepsOnTrack(1, getEuclidean(clapPulse, 16, clapOffset));

    // Track 2: Hi-Hats - Random density
    setTrackCategoryIfAvailable(2, "hh");
    int hatPulse = 7 + mediumDist(rng);  // 7-10 pulses
    auto hatSteps = getEuclidean(hatPulse, 16, 0);
    setStepsOnTrack(2, hatSteps);
    for (int step : hatSteps)
    {
        if (boolDist(rng) == 0)  // 50% get probability
            trackManager.getTrack(2).setStepModifier(step, '?', 50 + smallDist(rng) * 25);
    }

    // Track 3: Bass - Euclidean with random pitch locks
    setTrackCategoryIfAvailable(3, "bass");
    int bassPulse = 3 + mediumDist(rng);
    auto bassSteps = getEuclidean(bassPulse, 16, 0);
    setStepsOnTrack(3, bassSteps);
    for (int step : bassSteps)
    {
        if (boolDist(rng) == 0)  // 50% get pitch lock
        {
            StepModifierState state = trackManager.getTrack(3).getStepState(step);
            state.hasPitchLock = true;
            state.pitchLock = pitchDist(rng);
            trackManager.getTrack(3).setStepState(step, state);
        }
    }

    // Track 4: Percussion - Random polyrhythm
    setTrackCategoryIfAvailable(4, "perc");
    int percPulse = 2 + smallDist(rng);
    int percSteps = 11 + smallDist(rng) * 2;  // 11, 13, or 15
    setStepsOnTrack(4, getEuclidean(percPulse, percSteps, 0));
}

void PatternGenerator::generateIdmGlitch(std::mt19937& rng)
{
    std::uniform_int_distribution<int> boolDist(0, 1);
    std::uniform_int_distribution<int> smallDist(0, 2);
    std::uniform_int_distribution<int> mediumDist(0, 3);
    std::uniform_int_distribution<int> volPercentDist(0, 5);
    std::uniform_int_distribution<int> offsetDist(0, 15);

    int bpmVal = 135 + mediumDist(rng) * 5;  // 135-150 BPM
    setBPM(bpmVal);

    // Track 0: Kick - Sparse random
    setTrackCategoryIfAvailable(0, "bd");
    int kickPulse = 2 + smallDist(rng);
    auto kickSteps = getEuclidean(kickPulse, 16, offsetDist(rng));
    setStepsOnTrack(0, kickSteps);
    for (int step : kickSteps)
        trackManager.getTrack(0).setStepModifier(step, '?', 60 + smallDist(rng) * 15);

    // Track 1: Snare - Random with replicates
    setTrackCategoryIfAvailable(1, "sn");
    int snarePulse = 1 + boolDist(rng);
    auto snareSteps = getEuclidean(snarePulse, 16, 4);
    setStepsOnTrack(1, snareSteps);
    for (int step : snareSteps)
        if (boolDist(rng) == 0)
            trackManager.getTrack(1).setStepModifier(step, '!', 2 + smallDist(rng));

    // Track 2: Hi-Hats - Chaos with random ratchets
    setTrackCategoryIfAvailable(2, "hh");
    int hatFill = 8 + mediumDist(rng) * 4;  // 8-20 steps
    for (int step = 0; step < hatFill && step < 16; ++step)
    {
        trackManager.getTrack(2).setStepActive(step, true);
        if (boolDist(rng) == 0)
            trackManager.getTrack(2).setStepModifier(step, '*', 2 + smallDist(rng));
    }

    // Track 3: Bass - Random glitchy
    setTrackCategoryIfAvailable(3, "bass");
    setStepsOnTrack(3, getEuclidean(4 + mediumDist(rng), 16, offsetDist(rng)));

    // Track 5: Glitch/Noise with volume locks
    setTrackCategoryIfAvailable(5, "noise");
    int glitchPulse = 2 + mediumDist(rng);
    auto glitchSteps = getEuclidean(glitchPulse, 11, 0);
    setStepsOnTrack(5, glitchSteps);
    for (int step : glitchSteps)
    {
        StepModifierState state = trackManager.getTrack(5).getStepState(step);
        state.hasVolLock = true;
        state.volLock = 0.3f + volPercentDist(rng) * 0.14f;
        trackManager.getTrack(5).setStepState(step, state);
    }
}

void PatternGenerator::generateTrapDrill(std::mt19937& rng)
{
    std::uniform_int_distribution<int> boolDist(0, 1);
    std::uniform_int_distribution<int> smallDist(0, 2);
    std::uniform_int_distribution<int> mediumDist(0, 3);

    int bpmVal = 140 + smallDist(rng) * 10;  // 140-160 BPM
    setBPM(bpmVal);

    // Track 0: Kick - Random trap pattern
    setTrackCategoryIfAvailable(0, "bd");
    std::vector<int> kickPattern = {0};
    if (boolDist(rng)) kickPattern.push_back(8);
    if (boolDist(rng)) kickPattern.push_back(10 + smallDist(rng));
    if (boolDist(rng)) kickPattern.push_back(14);
    setStepsOnTrack(0, kickPattern);

    // Track 1: Snare/Clap
    setTrackCategoryIfAvailable(1, "sn");
    setStepsOnTrack(1, {4, 12});
    if (boolDist(rng))
        trackManager.getTrack(1).setStepModifier(12, '!', 2);

    // Track 2: Hi-Hats - Dense with random drill rolls
    setTrackCategoryIfAvailable(2, "hh");
    setStepsOnTrack(2, getEuclidean(12 + boolDist(rng) * 2, 16, 0));
    // Random drill rolls
    int rollStep = 6 + mediumDist(rng) * 2;
    trackManager.getTrack(2).setStepModifier(rollStep, '*', 3 + boolDist(rng));
    if (boolDist(rng))
        trackManager.getTrack(2).setStepModifier(14, '*', 4);

    // Track 3: 808 Bass - Match kick with sliding pitch
    setTrackCategoryIfAvailable(3, "bass");
    setStepsOnTrack(3, kickPattern);
    int lastPitch = -12;
    for (int step : kickPattern)
    {
        StepModifierState state = trackManager.getTrack(3).getStepState(step);
        state.hasPitchLock = true;
        state.pitchLock = lastPitch + (smallDist(rng) - 1) * 3;  // Slide up/down
        state.pitchLock = std::max(-12, std::min(5, state.pitchLock));
        lastPitch = state.pitchLock;
        trackManager.getTrack(3).setStepState(step, state);
    }

    // Track 4: Percussion
    setTrackCategoryIfAvailable(4, "perc");
    setStepsOnTrack(4, getEuclidean(2 + smallDist(rng), 16, 2));
}

void PatternGenerator::generateAmbient(std::mt19937& rng)
{
    std::uniform_int_distribution<int> boolDist(0, 1);
    std::uniform_int_distribution<int> smallDist(0, 2);
    std::uniform_int_distribution<int> mediumDist(0, 3);

    int bpmVal = 60 + smallDist(rng) * 15;  // 60-90 BPM
    setBPM(bpmVal);

    // Track 0: Very sparse kick
    setTrackCategoryIfAvailable(0, "bd");
    setStepsOnTrack(0, getEuclidean(1, 16 + mediumDist(rng) * 8, 0));

    // Track 1: Sparse snare with slow
    setTrackCategoryIfAvailable(1, "sn");
    auto snareSteps = getEuclidean(1, 16, 8);
    setStepsOnTrack(1, snareSteps);
    for (int step : snareSteps)
        trackManager.getTrack(1).setStepModifier(step, '/', 2 + boolDist(rng));

    // Track 2: Pads with elongate
    setTrackCategoryIfAvailable(2, "st");
    int padLoop = 13 + mediumDist(rng) * 4;  // Prime-ish
    auto padSteps = getEuclidean(2 + boolDist(rng), padLoop, 0);
    setStepsOnTrack(2, padSteps);
    for (int step : padSteps)
    {
        trackManager.getTrack(2).setStepModifier(step, '@', 2 + mediumDist(rng));
        if (boolDist(rng))
            trackManager.getTrack(2).setStepModifier(step, '/', 2);
    }

    // Track 3: Sub Bass
    setTrackCategoryIfAvailable(3, "bass");
    int subLoop = 17 + smallDist(rng) * 4;
    auto subSteps = getEuclidean(1 + boolDist(rng), subLoop, 0);
    setStepsOnTrack(3, subSteps);
    for (int step : subSteps)
    {
        trackManager.getTrack(3).setStepModifier(step, '@', 2 + smallDist(rng));
        StepModifierState state = trackManager.getTrack(3).getStepState(step);
        state.hasPitchLock = true;
        state.pitchLock = -12 + smallDist(rng) * 2;
        trackManager.getTrack(3).setStepState(step, state);
    }

    // Track 4: Sparse Perc
    setTrackCategoryIfAvailable(4, "perc");
    int percLoop = 19 + smallDist(rng) * 4;
    auto percSteps = getEuclidean(1 + boolDist(rng), percLoop, 0);
    setStepsOnTrack(4, percSteps);
    for (int step : percSteps)
    {
        trackManager.getTrack(4).setStepModifier(step, '?', 30 + mediumDist(rng) * 20);
        if (boolDist(rng))
            trackManager.getTrack(4).setStepModifier(step, '!', 2);
    }

    // Track 5: Sparse Noise
    setTrackCategoryIfAvailable(5, "noise");
    setStepsOnTrack(5, getEuclidean(1, 23 + mediumDist(rng) * 4, 0));
}

void PatternGenerator::generateDnB(std::mt19937& rng)
{
    std::uniform_int_distribution<int> boolDist(0, 1);
    std::uniform_int_distribution<int> smallDist(0, 2);
    std::uniform_int_distribution<int> mediumDist(0, 3);
    std::uniform_int_distribution<int> pitchDist(-12, 5);

    int bpmVal = 170 + smallDist(rng) * 10;  // 170-180 BPM
    setBPM(bpmVal);

    // Track 0: Kick - Random breakbeat
    setTrackCategoryIfAvailable(0, "bd");
    std::vector<int> kickPattern;
    kickPattern.push_back(0);
    if (boolDist(rng)) kickPattern.push_back(3 + smallDist(rng));
    kickPattern.push_back(5 + smallDist(rng));
    kickPattern.push_back(8);
    if (boolDist(rng)) kickPattern.push_back(10 + smallDist(rng));
    kickPattern.push_back(13);
    setStepsOnTrack(0, kickPattern);

    // Track 1: Snare
    setTrackCategoryIfAvailable(1, "sn");
    setStepsOnTrack(1, {4, 12});
    if (boolDist(rng))
        trackManager.getTrack(1).setStepModifier(12, '!', 2);

    // Track 2: Hi-Hats - Dense with random ratchets
    setTrackCategoryIfAvailable(2, "hh");
    for (int step = 0; step < 16; ++step)
    {
        if (boolDist(rng) || step % 2 == 0)
        {
            trackManager.getTrack(2).setStepActive(step, true);
            if (boolDist(rng) == 0)
                trackManager.getTrack(2).setStepModifier(step, '*', 2 + smallDist(rng));
        }
    }
    // Always add at least one ratchet
    trackManager.getTrack(2).setStepModifier(14, '*', 4);

    // Track 3: Reese Bass
    setTrackCategoryIfAvailable(3, "bass");
    int bassPulse = 2 + mediumDist(rng);
    auto bassSteps = getEuclidean(bassPulse, 16, 0);
    setStepsOnTrack(3, bassSteps);
    for (int step : bassSteps)
    {
        StepModifierState state = trackManager.getTrack(3).getStepState(step);
        state.hasPitchLock = true;
        state.pitchLock = -12 + pitchDist(rng);
        trackManager.getTrack(3).setStepState(step, state);
    }

    // Track 4: Percussion
    setTrackCategoryIfAvailable(4, "perc");
    setStepsOnTrack(4, getEuclidean(3 + mediumDist(rng), 16, 1));
}

void PatternGenerator::generateHouse(std::mt19937& rng)
{
    std::uniform_int_distribution<int> boolDist(0, 1);
    std::uniform_int_distribution<int> smallDist(0, 2);
    std::uniform_int_distribution<int> mediumDist(0, 3);
    std::uniform_int_distribution<int> styleDist(0, 3);
    std::uniform_int_distribution<int> offsetDist(0, 15);

    int bpmVal = 120 + smallDist(rng) * 10;  // 120-130 BPM
    setBPM(bpmVal);

    // Track 0: Kick - 4-on-the-floor (always)
    setTrackCategoryIfAvailable(0, "bd");
    setStepsOnTrack(0, {0, 4, 8, 12});

    // Track 1: Clap - With optional variation
    setTrackCategoryIfAvailable(1, "cp");
    if (boolDist(rng))
        setStepsOnTrack(1, {4, 12});
    else
        setStepsOnTrack(1, getEuclidean(2, 16, 4));

    // Track 2: Hi-Hats - Offbeats with variation
    setTrackCategoryIfAvailable(2, "hh");
    int hatStyle = styleDist(rng);
    if (hatStyle == 0)
        setStepsOnTrack(2, {2, 6, 10, 14});
    else if (hatStyle == 1)
        setStepsOnTrack(2, getEuclidean(7, 16, 0));
    else
        setStepsOnTrack(2, getEuclidean(9, 16, 0));

    for (int step = 0; step < 16; ++step)
    {
        if (trackManager.getTrack(2).isStepActive(step) && boolDist(rng))
            trackManager.getTrack(2).setStepModifier(step, '?', 40 + mediumDist(rng) * 20);
    }

    // Track 3: Bass - Classic with groove
    setTrackCategoryIfAvailable(3, "bass");
    int bassStyle = styleDist(rng);
    if (bassStyle == 0)
        setStepsOnTrack(3, {0, 3, 6, 9});
    else if (bassStyle == 1)
        setStepsOnTrack(3, {0, 2, 4, 6, 8, 10, 12, 14});  // Pumping
    else
        setStepsOnTrack(3, getEuclidean(5, 16, 0));

    // Add volume variation
    for (int step = 0; step < 16; ++step)
    {
        if (trackManager.getTrack(3).isStepActive(step) && boolDist(rng))
        {
            StepModifierState state = trackManager.getTrack(3).getStepState(step);
            state.hasVolLock = true;
            state.volLock = 0.5f + smallDist(rng) * 0.2f;
            trackManager.getTrack(3).setStepState(step, state);
        }
    }

    // Track 4: Percussion/Tom
    setTrackCategoryIfAvailable(4, "lt");
    setStepsOnTrack(4, getEuclidean(1 + boolDist(rng), 16, offsetDist(rng)));
}
