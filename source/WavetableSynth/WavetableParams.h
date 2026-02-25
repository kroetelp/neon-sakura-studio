#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <functional>
#include <memory>

/**
 * WavetableParams - Shared parameter state for Wavetable synthesizer
 *
 * Thread-safe parameter storage that can be shared between:
 * - WavetableSynth (audio rendering)
 * - WavetableSynthEditor (full UI)
 * - TrackComponent (track controls)
 *
 * Uses atomics for audio-thread safety and listeners for UI updates.
 */
class WavetableParams
{
public:
    WavetableParams();
    ~WavetableParams() = default;

    // Listener interface for UI updates
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void onParamsChanged() {}
        virtual void onOscParamChanged(int oscIndex, int paramId) { (void)oscIndex; (void)paramId; }
        virtual void onFilterParamChanged() {}
        virtual void onEnvelopeParamChanged() {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Notify listeners (call from UI thread only)
    void notifyChange();
    void notifyOscChange(int oscIndex, int paramId);
    void notifyFilterChange();
    void notifyEnvelopeChange();

    // ============================================================
    // Oscillator Parameters (3 oscillators)
    // ============================================================

    enum class OscParam
    {
        Level = 0,
        Morph,
        Detune,
        UnisonCount,
        PanSpread,
        Pan,
        PitchOffset
    };

    // Getters
    float getOscLevel(int oscIndex) const;
    float getOscMorph(int oscIndex) const;
    float getOscDetune(int oscIndex) const;
    int getOscUnisonCount(int oscIndex) const;
    float getOscPanSpread(int oscIndex) const;
    float getOscPan(int oscIndex) const;
    float getOscPitchOffset(int oscIndex) const;

    // Setters (thread-safe)
    void setOscLevel(int oscIndex, float value);
    void setOscMorph(int oscIndex, float value);
    void setOscDetune(int oscIndex, float value);
    void setOscUnisonCount(int oscIndex, int value);
    void setOscPanSpread(int oscIndex, float value);
    void setOscPan(int oscIndex, float value);
    void setOscPitchOffset(int oscIndex, float value);

    // ============================================================
    // Sub Oscillator Parameters
    // ============================================================

    float getSubLevel() const;
    int getSubOctave() const;
    int getSubWaveform() const;

    void setSubLevel(float value);
    void setSubOctave(int value);
    void setSubWaveform(int value);

    // ============================================================
    // Filter Parameters
    // ============================================================

    float getFilterCutoff() const;
    float getFilterResonance() const;
    float getFilterDrive() const;
    int getFilterMode() const;

    void setFilterCutoff(float value);
    void setFilterResonance(float value);
    void setFilterDrive(float value);
    void setFilterMode(int value);

    // ============================================================
    // Envelope Parameters
    // ============================================================

    float getEnvAttack() const;
    float getEnvDecay() const;
    float getEnvSustain() const;
    float getEnvRelease() const;

    void setEnvAttack(float value);
    void setEnvDecay(float value);
    void setEnvSustain(float value);
    void setEnvRelease(float value);

    // ============================================================
    // Master Parameters
    // ============================================================

    float getMasterLevel() const;
    void setMasterLevel(float value);

    // ============================================================
    // Wavetable Selection
    // ============================================================

    int getWavetableIndex() const;
    void setWavetableIndex(int index);

    // ============================================================
    // Preset Support
    // ============================================================

    // Copy all params from another WavetableParams (for preset loading)
    void copyFrom(const WavetableParams& other);

    // Reset to init values
    void resetToDefaults();

private:
    juce::ListenerList<Listener> listeners;

    // Oscillator params (3 oscillators)
    std::array<std::atomic<float>, 3> oscLevels;
    std::array<std::atomic<float>, 3> oscMorphs;
    std::array<std::atomic<float>, 3> oscDetunes;
    std::array<std::atomic<int>, 3> oscUnisonCounts;
    std::array<std::atomic<float>, 3> oscPanSpreads;
    std::array<std::atomic<float>, 3> oscPans;
    std::array<std::atomic<float>, 3> oscPitchOffsets;

    // Sub oscillator
    std::atomic<float> subLevel;
    std::atomic<int> subOctave;
    std::atomic<int> subWaveform;

    // Filter
    std::atomic<float> filterCutoff;
    std::atomic<float> filterResonance;
    std::atomic<float> filterDrive;
    std::atomic<int> filterMode;

    // Envelope
    std::atomic<float> envAttack;
    std::atomic<float> envDecay;
    std::atomic<float> envSustain;
    std::atomic<float> envRelease;

    // Master
    std::atomic<float> masterLevel;

    // Wavetable selection
    std::atomic<int> wavetableIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableParams)
};
