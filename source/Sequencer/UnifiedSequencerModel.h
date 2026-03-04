// ============================================================================
// UnifiedSequencerModel.h - Gemeinsames Datenmodell für alle Sequencer-Views
// ============================================================================
//
// Dieses Model speichert die Pattern-Daten für alle Views:
// - Track View (Track-basierte Darstellung)
// - Pattern View (Grid-basierte Darstellung)
// - Timeline View (Timeline-basierte Darstellung)
//
// Alle Views teilen sich die gleichen Daten, aber präsentieren sie anders.

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <memory>
#include "../TrackModel.h"

// ============================================================================
// Sequencer Mode - Welcher View ist aktiv?
// ============================================================================
enum class SequencerMode
{
    TrackView,    // Track-basierte Darstellung (wie ursprünglicher TrackComponent)
    PatternView,  // Pattern-Grid (wie StepSequencerPanel)
    TimelineView   // Timeline (wie TimelinePanel)
};

// ============================================================================
// Pattern Data - Daten für einen Track-Pattern
// ============================================================================
struct PatternData
{
    int trackIndex = 0;
    int numSteps = 16;
    std::vector<bool> activeSteps;  // Welche Steps sind aktiv?

    // Modifiers pro Step
    std::vector<StepModifierState> stepModifiers;

    // Track-spezifische Parameter
    float volume = 0.8f;
    int pitch = 0;
    float attack = 0.01f;
    float decay = 0.1f;
    float cutoff = 0.0f;
    float resonance = 0.0f;
};

// ============================================================================
// UnifiedSequencerModel - Das gemeinsame Datenmodell
// ============================================================================
class UnifiedSequencerModel
{
public:
    static constexpr int maxTracks = 8;
    static constexpr int maxSteps = 64;

    UnifiedSequencerModel();
    ~UnifiedSequencerModel() = default;

    // === Sequencer Mode ===
    void setSequencerMode(SequencerMode mode);
    SequencerMode getSequencerMode() const { return currentMode; }

    // === Pattern Data Access ===

    // Pattern für einen Track abrufen
    const PatternData& getPattern(int trackIndex) const;
    PatternData& getPattern(int trackIndex);

    // Pattern setzen
    void setPattern(int trackIndex, const PatternData& pattern);

    // Step-Zustand ändern
    void setStepActive(int trackIndex, int step, bool active);
    void setStepModifier(int trackIndex, int step, const StepModifierState& modifier);

    // Track-Parameter
    void setTrackVolume(int trackIndex, float volume);
    void setTrackPitch(int trackIndex, int pitch);
    void setTrackAttack(int trackIndex, float attack);
    void setTrackDecay(int trackIndex, float decay);
    void setTrackCutoff(int trackIndex, float cutoff);
    void setTrackResonance(int trackIndex, float resonance);

    // Bulk Operations
    void clearTrack(int trackIndex);
    void clearAll();

    // === Playback State ===
    void setCurrentStep(int step) { currentStep = step; }
    int getCurrentStep() const { return currentStep; }

    void setPlaying(bool playing) { isPlaying = playing; }
    bool getPlaying() const { return isPlaying; }

    void setBPM(double bpm) { currentBPM = bpm; }
    double getBPM() const { return currentBPM; }

    // === Callbacks ===
    std::function<void(int trackIndex, int step, bool active)> onStepChanged;
    std::function<void(int trackIndex)> onTrackChanged;
    std::function<void()> onPatternChanged;

private:
    SequencerMode currentMode = SequencerMode::TrackView;

    // Pattern Data pro Track
    std::array<PatternData, maxTracks> patterns;

    // Playback State
    int currentStep = 0;
    bool isPlaying = false;
    double currentBPM = 120.0;

    // Helper zum Initialisieren der Patterns
    void initializePatterns();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedSequencerModel)
};
