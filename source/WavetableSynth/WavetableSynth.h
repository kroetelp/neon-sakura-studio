#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <array>
#include "WavetableData.h"
#include "WavetableVoice.h"
#include "WavetableParams.h"
#include "../Modulation/ModulationMatrix.h"
#include "../Modulation/LFOModulator.h"
#include "../Modulation/EnvelopeModulator.h"

/**
 * WavetableSound - Basic sound that applies to all notes
 */
class WavetableSound : public juce::SynthesiserSound
{
public:
    WavetableSound() = default;

    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

/**
 * WavetableSynth - JUCE Synthesiser wrapper for the wavetable engine
 *
 * Manages:
 * - Voice allocation
 * - Shared wavetable data
 * - Shared parameters (WavetableParams) for UI integration
 * - LFOs and Envelopes
 */
class WavetableSynth : public juce::Synthesiser
{
public:
    static constexpr int maxVoices = 16;
    static constexpr int numLFOs = 4;
    static constexpr int numEnvelopes = 3;

    WavetableSynth();
    ~WavetableSynth() override = default;

    // Setup
    void setSampleRate(double sampleRate);
    void setWavetable(std::shared_ptr<WavetableData> wavetable);
    std::shared_ptr<WavetableData> getWavetable() const { return wavetableData; }

    // Shared parameters (for UI integration)
    void setSharedParams(std::shared_ptr<WavetableParams> params);
    std::shared_ptr<WavetableParams> getSharedParams() const { return sharedParams; }

    // Voice management
    void setNumVoices(int numVoices);
    int getNumVoicesActive() const { return getNumVoices(); }

    // Legacy parameter access (deprecated - use sharedParams instead)
    // These now read from sharedParams
    WavetableVoice::VoiceParams& getParams() { return legacyParams; }

    // LFO access
    LFOModulator& getLFO(int index) { return lfos[index]; }
    const LFOModulator& getLFO(int index) const { return lfos[index]; }

    // Envelope access (for modulation, not voice envelopes)
    EnvelopeModulator& getEnvelope(int index) { return envelopes[index]; }
    const EnvelopeModulator& getEnvelope(int index) const { return envelopes[index]; }

    // Modulation matrix
    ModulationMatrix& getModulationMatrix() { return modulationMatrix; }
    const ModulationMatrix& getModulationMatrix() const { return modulationMatrix; }

    // Process modulations (call before rendering audio)
    void processModulations();

    // Get current modulation value from any source
    float getModulatorValue(ModulationSource source) const;

    // Macro controls (MIDI CC or UI assignable)
    void setMacroValue(int macroIndex, float value);
    float getMacroValue(int macroIndex) const;

    // MIDI learn / performance
    void setModWheelValue(float value) { modWheelValue = value; }
    void setPitchBendValue(float value) { pitchBendValue = value; }
    void setAftertouchValue(float value) { aftertouchValue = value; }

private:
    // Shared wavetable
    std::shared_ptr<WavetableData> wavetableData;

    // Shared parameters (thread-safe, shared with UI)
    std::shared_ptr<WavetableParams> sharedParams;

    // Legacy params for backward compatibility
    WavetableVoice::VoiceParams legacyParams;

    // LFOs (global, not per-voice)
    std::array<LFOModulator, numLFOs> lfos;

    // Envelopes (global for modulation, voices have their own amp/filter envelopes)
    std::array<EnvelopeModulator, numEnvelopes> envelopes;

    // Modulation matrix
    ModulationMatrix modulationMatrix;

    // Macro controls
    std::array<float, 4> macroValues = { {0.0f, 0.0f, 0.0f, 0.0f} };

    // Performance controllers
    float modWheelValue = 0.0f;
    float pitchBendValue = 0.0f;
    float aftertouchValue = 0.0f;

    double currentSampleRate = 44100.0;

    void setupModulationMatrix();
};
