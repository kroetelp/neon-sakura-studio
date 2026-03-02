#pragma once

#include "WavetableSynth.h"
#include "../Modulation/ModulationSource.h"
#include "../MidiEventQueue.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <array>
#include <atomic>

/**
 * WavetableEngine - Integration layer between WavetableSynth and the main audio engine
 *
 * Supports two modes:
 * 1. Standalone - Generates audio directly (like a regular synth)
 * 2. Modulator - Modulates existing drum samples in tracks
 *
 * Master FX Chain: Synth -> Chorus -> Delay -> Reverb -> Output
 */
class WavetableEngine
{
public:
    enum class Mode
    {
        Standalone,     // Synth produces audio
        Modulator       // Modulates track samples
    };

    // FX Parameters structure (atomic for thread-safety)
    struct FXParams
    {
        // Chorus
        std::atomic<float> chorusMix{0.0f};
        std::atomic<float> chorusRate{1.0f};
        std::atomic<float> chorusDepth{0.25f};

        // Delay
        std::atomic<float> delayMix{0.0f};
        std::atomic<float> delayTime{0.33f};  // seconds
        std::atomic<float> delayFeedback{0.4f};

        // Reverb
        std::atomic<float> reverbMix{0.0f};
        std::atomic<float> reverbSize{0.5f};
        std::atomic<float> reverbDamping{0.5f};
    };

    WavetableEngine();
    ~WavetableEngine() = default;

    // Setup
    void prepareToPlay(int samplesPerBlock, double sampleRate);
    void releaseResources();

    // Mode control
    void setMode(Mode mode) { currentMode = mode; }
    Mode getMode() const { return currentMode.load(); }

    // Audio processing - Standalone mode
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // Modulator mode - Get modulation values for tracks
    float getModulationValue(int trackIndex, ModulationTarget target) const;
    bool isModulationEnabled(int trackIndex) const;
    void setModulationEnabled(int trackIndex, bool enabled);

    // Access to the synth for UI
    WavetableSynth& getSynthesiser() { return synth; }
    const WavetableSynth& getSynthesiser() const { return synth; }

    // FX Parameters access
    FXParams& getFXParams() { return fxParams; }
    const FXParams& getFXParams() const { return fxParams; }

    // BPM for LFO tempo sync
    void setBPM(float bpm);

    // MIDI input for standalone mode
    void handleMidiEvent(const juce::MidiMessage& message);

    // Note on/off for testing/keyboard (now with dynamic channel)
    void noteOn(int channel, int midiNote, float velocity);
    void noteOff(int channel, int midiNote);

    // Master volume
    void setMasterVolume(float volume) { masterVolume.store(volume); }
    float getMasterVolume() const { return masterVolume.load(); }

    /**
     * Set the MIDI event queue for lock-free thread communication
     * The queue is read by the audio thread during processBlock()
     */
    void setMidiEventQueue(MidiEventQueue* queue) { midiEventQueue = queue; }
    MidiEventQueue* getMidiEventQueue() const { return midiEventQueue; }

    // === Oscilloscope Ring Buffer (Lock-free audio visualization) ===
    static constexpr int scopeBufferSize = 2048;

    // Copy the latest scope samples into the provided buffers (for UI thread)
    void copyScopeSamples(float* leftDest, float* rightDest, int numSamples) const
    {
        int readPos = scopeWritePos.load();
        int numToCopy = juce::jmin(numSamples, scopeBufferSize);

        for (int i = 0; i < numToCopy; ++i)
        {
            int idx = (readPos - numToCopy + i + scopeBufferSize) % scopeBufferSize;
            leftDest[i] = scopeLeftBuffer[idx].load();
            rightDest[i] = scopeRightBuffer[idx].load();
        }
    }

private:
    WavetableSynth synth;

    std::atomic<Mode> currentMode{Mode::Standalone};
    std::atomic<float> masterVolume{0.8f};
    std::atomic<float> currentBPM{120.0f};

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // FX Parameters
    FXParams fxParams;

    // Oscilloscope ring buffer (lock-free: audio thread writes, UI thread reads)
    std::atomic<int> scopeWritePos{0};
    std::array<std::atomic<float>, scopeBufferSize> scopeLeftBuffer;
    std::array<std::atomic<float>, scopeBufferSize> scopeRightBuffer;

    // DSP Effects
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine{44100 * 2};  // 2 sec max
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    // Delay buffer for stereo
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    // Per-track modulation enable
    std::array<std::atomic<bool>, 8> trackModulationEnabled;

    // Per-track modulation values cache
    mutable std::array<float, static_cast<int>(ModulationTarget::Count)> cachedModulationValues{};

    // Lock-free MIDI event queue (owned by WootingManager, read by audio thread)
    MidiEventQueue* midiEventQueue = nullptr;

    // Process MIDI events from the lock-free queue into the MidiBuffer
    void processMidiEventsFromQueue(juce::MidiBuffer& midiBuffer);

    // Process master FX chain
    void processMasterFX(juce::AudioBuffer<float>& buffer);
    void processChorus(juce::AudioBuffer<float>& buffer);
    void processDelay(juce::AudioBuffer<float>& buffer);
    void processReverb(juce::AudioBuffer<float>& buffer);
};
