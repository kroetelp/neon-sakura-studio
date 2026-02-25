#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include "TrackType.h"

// Step state struct supporting TidalCycles modifiers and P-Locks
struct StepModifierState
{
    bool active = false;
    char modifierType = ' ';  // ' ' = Normal, '*' = Speed, '/' = Slow, '@' = Elongate, '!' = Replicate, '?' = Probability
    int modifierValue = 1;      // Modifier value (1-4 typically, or 25/50/75 for probability)

    // Parameter Locks (Elektron-style P-Locks)
    bool hasPitchLock = false;
    int pitchLock = 0;          // -12 to +12 semitones
    bool hasVolLock = false;
    float volLock = 1.0f;       // 0.0 to 1.0

    // Helper to check if step is effectively active
    bool isActive() const { return active; }

    // Helper to check if any P-Lock is active
    bool hasAnyLock() const { return hasPitchLock || hasVolLock; }

    // Helper to get display text
    juce::String getDisplayText() const
    {
        if (!active)
            return "";
        if (modifierType == ' ')
        {
            // Show P-Lock indicator if any lock is active
            if (hasAnyLock())
                return "[P]";
            return "";
        }
        if (modifierType == '?')
            return "?" + juce::String(modifierValue);
        return juce::String(modifierType) + juce::String(modifierValue);
    }
};

class TrackModel
{
public:
    static constexpr int totalSteps = 64;
    static constexpr int numBanks = 4;
    static constexpr int stepsPerBank = 16;

    TrackModel();

    // Step manipulation
    void setStepActive(int step, bool active);
    bool isStepActive(int step) const;
    StepModifierState getStepState(int step) const;
    void setStepState(int step, const StepModifierState& state);
    void setStepModifier(int step, char modifierType, int modifierValue);
    int getModifierValue(int step) const;
    char getModifierType(int step) const;
    void clearAllSteps();

    // Sample management
    void setCurrentSampleFiles(const juce::Array<juce::File>& files);
    void setCurrentSampleIndex(int index);
    juce::File getCurrentSampleFile() const;
    int getCurrentSampleIndex() const { return currentSampleIndex; }
    int getNumSamples() const { return currentSampleFiles.size(); }
    const juce::Array<juce::File>& getSampleFiles() const { return currentSampleFiles; }

    // Loop length
    void setTrackLoopLength(int length) { trackLoopLength.store(length); }
    int getTrackLoopLength() const { return trackLoopLength.load(); }

    // Track expansion state
    void setIsExpanded(bool expanded) { isExpanded = expanded; }
    bool getIsExpanded() const { return isExpanded; }

    // Bank management
    void setBank(int bank);
    int getBank() const { return currentBank; }

    int getTotalSteps() const { return totalSteps; }

    // Track type management
    void setTrackType(TrackType type) { trackType.store(type); }
    TrackType getTrackType() const { return trackType.load(); }

private:
    std::array<std::array<StepModifierState, stepsPerBank>, numBanks> bankSteps;
    int currentBank = 0;
    juce::Array<juce::File> currentSampleFiles;
    int currentSampleIndex = 0;
    std::atomic<int> trackLoopLength{16};
    std::atomic<TrackType> trackType{TrackType::Sampler};
    bool isExpanded = true;
};
