// ============================================================================
// UnifiedSequencerModel.cpp - Implementierung
// ============================================================================

#include "UnifiedSequencerModel.h"

UnifiedSequencerModel::UnifiedSequencerModel()
{
    initializePatterns();
}

void UnifiedSequencerModel::initializePatterns()
{
    // Initialisiere alle Patterns mit Standardwerten
    for (int track = 0; track < maxTracks; ++track)
    {
        patterns[track].trackIndex = track;
        patterns[track].numSteps = 16;
        patterns[track].activeSteps.resize(16, false);
        patterns[track].stepModifiers.resize(16);

        // Standard-Track-Parameter
        patterns[track].volume = 0.8f;
        patterns[track].pitch = 0;
        patterns[track].attack = 0.01f;
        patterns[track].decay = 0.1f;
        patterns[track].cutoff = 0.0f;
        patterns[track].resonance = 0.0f;
    }
}

// ============================================================================
// Sequencer Mode
// ============================================================================

void UnifiedSequencerModel::setSequencerMode(SequencerMode mode)
{
    if (currentMode != mode)
    {
        currentMode = mode;
        // Views können auf Mode-Wechsel reagieren
    }
}

// ============================================================================
// Pattern Data Access
// ============================================================================

const PatternData& UnifiedSequencerModel::getPattern(int trackIndex) const
{
    jassert(trackIndex >= 0 && trackIndex < maxTracks);
    return patterns[trackIndex];
}

PatternData& UnifiedSequencerModel::getPattern(int trackIndex)
{
    jassert(trackIndex >= 0 && trackIndex < maxTracks);
    return patterns[trackIndex];
}

void UnifiedSequencerModel::setPattern(int trackIndex, const PatternData& pattern)
{
    jassert(trackIndex >= 0 && trackIndex < maxTracks);
    patterns[trackIndex] = pattern;

    if (onTrackChanged)
        onTrackChanged(trackIndex);

    if (onPatternChanged)
        onPatternChanged();
}

// ============================================================================
// Step-Operationen
// ============================================================================

void UnifiedSequencerModel::setStepActive(int trackIndex, int step, bool active)
{
    if (trackIndex < 0 || trackIndex >= maxTracks)
        return;

    auto& pattern = patterns[trackIndex];

    // Resize falls nötig
    if (step >= static_cast<int>(pattern.activeSteps.size()))
    {
        pattern.activeSteps.resize(step + 1, false);
        pattern.stepModifiers.resize(step + 1);
    }

    pattern.activeSteps[step] = active;

    if (onStepChanged)
        onStepChanged(trackIndex, step, active);
}

void UnifiedSequencerModel::setStepModifier(int trackIndex, int step, const StepModifierState& modifier)
{
    if (trackIndex < 0 || trackIndex >= maxTracks)
        return;

    auto& pattern = patterns[trackIndex];

    // Resize falls nötig
    if (step >= static_cast<int>(pattern.stepModifiers.size()))
    {
        pattern.activeSteps.resize(step + 1, false);
        pattern.stepModifiers.resize(step + 1);
    }

    pattern.stepModifiers[step] = modifier;
    pattern.activeSteps[step] = modifier.active;

    if (onStepChanged)
        onStepChanged(trackIndex, step, modifier.active);
}

// ============================================================================
// Track-Parameter
// ============================================================================

void UnifiedSequencerModel::setTrackVolume(int trackIndex, float volume)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        patterns[trackIndex].volume = juce::jlimit(0.0f, 1.0f, volume);

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

void UnifiedSequencerModel::setTrackPitch(int trackIndex, int pitch)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        patterns[trackIndex].pitch = juce::jlimit(-24, 24, pitch);

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

void UnifiedSequencerModel::setTrackAttack(int trackIndex, float attack)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        patterns[trackIndex].attack = juce::jlimit(0.0f, 1.0f, attack);

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

void UnifiedSequencerModel::setTrackDecay(int trackIndex, float decay)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        patterns[trackIndex].decay = juce::jlimit(0.0f, 2.0f, decay);

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

void UnifiedSequencerModel::setTrackCutoff(int trackIndex, float cutoff)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        patterns[trackIndex].cutoff = juce::jlimit(0.0f, 1.0f, cutoff);

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

void UnifiedSequencerModel::setTrackResonance(int trackIndex, float resonance)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        patterns[trackIndex].resonance = juce::jlimit(0.0f, 1.0f, resonance);

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

// ============================================================================
// Bulk Operations
// ============================================================================

void UnifiedSequencerModel::clearTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < maxTracks)
    {
        auto& pattern = patterns[trackIndex];
        std::fill(pattern.activeSteps.begin(), pattern.activeSteps.end(), false);

        // Alle Modifiers zurücksetzen
        for (auto& mod : pattern.stepModifiers)
        {
            mod = StepModifierState();
        }

        if (onTrackChanged)
            onTrackChanged(trackIndex);
    }
}

void UnifiedSequencerModel::clearAll()
{
    for (int i = 0; i < maxTracks; ++i)
    {
        clearTrack(i);
    }

    if (onPatternChanged)
        onPatternChanged();
}
