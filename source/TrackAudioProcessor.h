#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include "TrackType.h"

class TrackModel;
class WavetableSynth;
class WavetableParams;

// Custom SamplerVoice that properly wraps JUCE's SamplerSound
class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    CustomSamplerVoice() = default;
};

/**
 * TrackAudioProcessor - Dual-engine audio processor for tracks
 *
 * Supports:
 * - Sampler mode: Sample-based playback
 * - Wavetable mode: Wavetable synthesiser engine
 */
class TrackAudioProcessor
{
public:
    TrackAudioProcessor(juce::AudioFormatManager& formatManager, TrackModel& model);
    ~TrackAudioProcessor();

    // Audio processing
    void prepareAudio(double sampleRate, int samplesPerBlock);
    void processAudioBlock(juce::AudioBuffer<float>& buffer);

    // Track type management
    void setTrackType(TrackType type);
    TrackType getTrackType() const;

    // Sample loading (sampler mode)
    void loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory);
    void loadSampleAtIndex(int index);

    // Synthesizer access (for AudioEngine MIDI)
    juce::Synthesiser& getSynthesiser() { return synth; }

    // Wavetable synth access (for synth mode)
    WavetableSynth* getWavetableSynth() { return wavetableSynth.get(); }
    const WavetableSynth* getWavetableSynth() const { return wavetableSynth.get(); }

    // Shared wavetable params access (for UI integration)
    std::shared_ptr<WavetableParams> getWavetableParams() const;

    // Process synth audio (applies synth-specific processing)
    void processSynthAudio(juce::AudioBuffer<float>& buffer);

    // Control parameters (thread-safe)
    void setVolume(float vol) { volume.store(vol); }
    void setPitch(int pit) { pitch.store(pit); }
    void setAttack(float atk);
    void setDecay(float dec);
    void setCutoff(float cut);
    void setMuted(bool muted) { isMuted.store(muted); }
    void setSolo(bool solo) { isSolo.store(solo); }

    float getVolume() const { return volume.load(); }
    int getPitch() const { return pitch.load(); }
    float getDecay() const { return decay.load(); }
    float getAttack() const { return attack.load(); }
    float getCutoff() const { return cutoff.load(); }
    bool getMuted() const { return isMuted.load(); }
    bool getSolo() const { return isSolo.load(); }

private:
    TrackModel& model;

    // Track type
    TrackType currentTrackType{TrackType::Sampler};

    // Sampler Engine
    juce::Synthesiser synth;
    juce::AudioFormatManager& formatManager;
    juce::CriticalSection synthLock;

    // Wavetable Engine (per-track instance)
    std::unique_ptr<WavetableSynth> wavetableSynth;

    // DSP Filter (for sampler)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
    double currentSampleRate = 44100.0;

    // Control values (atomics for audio thread access)
    std::atomic<float> volume{0.8f};
    std::atomic<int> pitch{0};
    std::atomic<float> attack{0.0f};
    std::atomic<float> decay{0.5f};
    std::atomic<float> cutoff{20000.0f};
    float lastCutoff{20000.0f};
    std::atomic<bool> isMuted{false};
    std::atomic<bool> isSolo{false};

    // Current category for sample loading
    juce::String currentCategory;

    // Helper
    void updateDecayEnvelope();
};
