#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <array>
#include <atomic>
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
 */
class WavetableSynth : public juce::Synthesiser
{
public:
    static constexpr int maxVoices = 16;
    static constexpr int numLFOs = 4;
    static constexpr int numEnvelopes = 3;

    WavetableSynth();
    ~WavetableSynth() override = default;

    // Setup (Korrekt überschrieben für JUCE)
    void setCurrentPlaybackSampleRate(double sampleRate) override;
    
    void setWavetable(std::shared_ptr<WavetableData> wavetable);
    std::shared_ptr<WavetableData> getWavetable() const { return wavetableData; }

    // Shared parameters (for UI integration) - jetzt thread-safe
    void setSharedParams(std::shared_ptr<WavetableParams> params);
    std::shared_ptr<WavetableParams> getSharedParams() const { return sharedParams.load(); }

    // Voice management
    void setNumVoices(int numVoices);
    int getNumVoicesActive() const { return getNumVoices(); }

    // Überschrieben für Velocity-Tracking
    void noteOn(int midiChannel, int midiNoteNumber, float velocity) override;

    // Legacy parameter access
    WavetableVoice::VoiceParams& getParams() { return legacyParams; }

    // LFO access
    LFOModulator& getLFO(int index) { return lfos[index]; }
    const LFOModulator& getLFO(int index) const { return lfos[index]; }

    // Envelope access
    EnvelopeModulator& getEnvelope(int index) { return envelopes[index]; }
    const EnvelopeModulator& getEnvelope(int index) const { return envelopes[index]; }

    // Modulation matrix
    ModulationMatrix& getModulationMatrix() { return modulationMatrix; }
    const ModulationMatrix& getModulationMatrix() const { return modulationMatrix; }

    void processModulations();
    float getModulatorValue(ModulationSource source) const;

    // Macro controls
    void setMacroValue(int macroIndex, float value);
    float getMacroValue(int macroIndex) const;

    // Performance
    void setModWheelValue(float value) { modWheelValue = value; }
    void setPitchBendValue(float value) { pitchBendValue = value; }
    void setAftertouchValue(float value) { aftertouchValue = value; }

private:
    std::shared_ptr<WavetableData> wavetableData;

    // Thread-safe durch C++20 atomic shared_ptr
    std::atomic<std::shared_ptr<WavetableParams>> sharedParams;

    WavetableVoice::VoiceParams legacyParams;
    std::array<LFOModulator, numLFOs> lfos;
    std::array<EnvelopeModulator, numEnvelopes> envelopes;
    ModulationMatrix modulationMatrix;

    std::array<float, 4> macroValues = { {0.0f, 0.0f, 0.0f, 0.0f} };
    
    // Für die Modulationsmatrix
    std::atomic<float> lastVelocity { 0.8f }; 

    float modWheelValue = 0.0f;
    float pitchBendValue = 0.0f;
    float aftertouchValue = 0.0f;
    double currentSampleRate = 44100.0;

    void setupModulationMatrix();
};