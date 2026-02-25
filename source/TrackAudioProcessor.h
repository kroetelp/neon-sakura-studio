#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>

class TrackModel;

// Custom SamplerVoice that properly wraps JUCE's SamplerSound
class CustomSamplerVoice : public juce::SamplerVoice
{
public:
    CustomSamplerVoice() = default;
};

class TrackAudioProcessor
{
public:
    TrackAudioProcessor(juce::AudioFormatManager& formatManager, TrackModel& model);
    ~TrackAudioProcessor();

    // Audio processing
    void prepareAudio(double sampleRate, int samplesPerBlock);
    void processAudioBlock(juce::AudioBuffer<float>& buffer);

    // Sample loading
    void loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory);
    void loadSampleAtIndex(int index);

    // Synthesizer access (for AudioEngine MIDI)
    juce::Synthesiser& getSynthesiser() { return synth; }

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

    // Synthesizer
    juce::Synthesiser synth;
    juce::AudioFormatManager& formatManager;
    juce::CriticalSection synthLock;

    // DSP Filter
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
