#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <array>
#include <atomic>
#include "WavetableOscillator.h"
#include "SubOscillator.h"
#include "WavetableFilter.h"
#include "WavetableParams.h"
#include "../Modulation/ModulationSource.h"
#include "../Modulation/ModulationMatrix.h"

class WavetableSynth;

/**
 * WavetableVoice - Single voice for the wavetable synthesiser
 *
 * Contains:
 * - 3 Wavetable oscillators with morphing, unison, detune
 * - Sub oscillator
 * - Multi-mode filter
 * - Amp envelope
 */
class WavetableVoice : public juce::SynthesiserVoice
{
public:
    WavetableVoice();
    ~WavetableVoice() override = default;

    // SynthesiserVoice interface
    bool canPlaySound(juce::SynthesiserSound*) override { return true; }
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controllerNumber, int newValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    // Setup
    void setWavetable(std::shared_ptr<WavetableData> wavetable);
    void setModulationMatrix(ModulationMatrix* matrix) { modulationMatrix = matrix; }

    // Shared params (new system)
    void setSharedParams(std::shared_ptr<WavetableParams> params) { sharedParams = params; }
    std::shared_ptr<WavetableParams> getSharedParams() const { return sharedParams; }

    // Voice parameters (thread-safe)
    struct VoiceParams
    {
        // Oscillator levels and morph positions
        std::array<std::atomic<float>, 3> oscLevels = { {1.0f, 0.0f, 0.0f} };
        std::array<std::atomic<float>, 3> oscMorphs = { {0.0f, 0.0f, 0.0f} };
        std::array<std::atomic<float>, 3> oscDetunes = { {0.0f, 0.0f, 0.0f} };
        std::array<std::atomic<int>, 3> oscUnisonCounts = { {1, 1, 1} };
        std::array<std::atomic<float>, 3> oscPanSpreads = { {0.0f, 0.0f, 0.0f} };
        std::array<std::atomic<float>, 3> oscPans = { {0.0f, 0.0f, 0.0f} };
        std::array<std::atomic<float>, 3> oscPitchOffsets = { {0.0f, 0.0f, 0.0f} };  // Semitones

        // Sub oscillator
        std::atomic<float> subLevel{0.0f};
        std::atomic<int> subOctave{1};
        std::atomic<int> subWaveform{0};

        // Filter
        std::atomic<float> filterCutoff{1000.0f};
        std::atomic<float> filterResonance{0.0f};
        std::atomic<float> filterDrive{0.0f};
        std::atomic<int> filterMode{0};

        // Envelope
        std::atomic<float> envAttack{0.01f};
        std::atomic<float> envDecay{0.1f};
        std::atomic<float> envSustain{0.7f};
        std::atomic<float> envRelease{0.3f};

        // Master
        std::atomic<float> masterLevel{1.0f};
    };

    VoiceParams* getParams() { return params; }
    void setParams(VoiceParams* p) { params = p; }

private:
    // Oscillators
    std::array<WavetableOscillator, 3> oscillators;
    SubOscillator subOscillator;

    // Filter
    WavetableFilter filter;

    // Envelope (simple ADSR)
    struct ADSR
    {
        enum class Stage { Idle, Attack, Decay, Sustain, Release };
        Stage stage = Stage::Idle;
        float value = 0.0f;
        float attack = 0.01f;
        float decay = 0.1f;
        float sustain = 0.7f;
        float release = 0.3f;
        float releaseStartValue = 0.0f;
        double sampleRate = 44100.0;
        int samplesInStage = 0;
        int stageLengthSamples = 0;

        void setSampleRate(double sr) { sampleRate = sr; }
        void noteOn(float a, float d, float s, float r);
        void noteOff();
        float process();
        bool isActive() const { return stage != Stage::Idle; }
        bool isInRelease() const { return stage == Stage::Release; }
    } ampEnvelope;

    // Voice state
    int currentMidiNote = 60;
    float currentVelocity = 1.0f;
    float pitchBend = 0.0f;  // -1 to 1
    bool isActive = false;

    // Modulation
    ModulationMatrix* modulationMatrix = nullptr;
    VoiceParams* params = nullptr;

    // Shared params (new system - takes priority)
    std::shared_ptr<WavetableParams> sharedParams;

    // Helper
    float calculateFrequency(int midiNote, float pitchOffset = 0.0f) const;
    void applyModulation(float& left, float& right);
};
