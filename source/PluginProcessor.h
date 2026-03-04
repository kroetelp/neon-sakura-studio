#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <atomic>

// Forward declarations
class WavetableEngine;
class WavetableParams;

/**
 * NeonSakuraProcessor - VST3/AU Audio Plugin Processor
 *
 * This is the main plugin processor that wraps the WavetableEngine
 * and exposes parameters for DAW automation.
 *
 * Supported formats: VST3, AU, Standalone
 */
class NeonSakuraProcessor : public juce::AudioProcessor
{
public:
    NeonSakuraProcessor();
    ~NeonSakuraProcessor() override;

    // ============================================================
    // JUCE AudioProcessor API
    // ============================================================

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    // Editor creation
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ============================================================
    // Metadata
    // ============================================================

    const juce::String getName() const override { return "Neon Sakura Synth"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 2.0; }  // For reverb tail

    // ============================================================
    // Bus Configuration
    // ============================================================

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // ============================================================
    // Program/Preset Management
    // ============================================================

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    // State serialization (for DAW project save/load)
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ============================================================
    // Direct Engine Access (for Editor)
    // ============================================================

    WavetableEngine& getWavetableEngine() { return *wavetableEngine; }
    const WavetableEngine& getWavetableEngine() const { return *wavetableEngine; }

    std::shared_ptr<WavetableParams> getWavetableParams();

    // ============================================================
    // Parameter Access (for Editor)
    // ============================================================

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const { return apvts; }

private:
    // ============================================================
    // Audio Engine - Direct WavetableEngine for synth plugin
    // ============================================================

    std::unique_ptr<WavetableEngine> wavetableEngine;

    // ============================================================
    // Parameter Management
    // ============================================================

    juce::AudioProcessorValueTreeState apvts;

    // Parameter pointers for fast access
    std::atomic<float>* masterVolumeParam = nullptr;

    // OSC 1
    std::atomic<float>* osc1LevelParam = nullptr;
    std::atomic<float>* osc1MorphParam = nullptr;
    std::atomic<float>* osc1DetuneParam = nullptr;
    std::atomic<float>* osc1PanParam = nullptr;
    std::atomic<float>* osc1UnisonParam = nullptr;
    std::atomic<float>* osc1PanSpreadParam = nullptr;

    // OSC 2
    std::atomic<float>* osc2LevelParam = nullptr;
    std::atomic<float>* osc2MorphParam = nullptr;
    std::atomic<float>* osc2DetuneParam = nullptr;
    std::atomic<float>* osc2PanParam = nullptr;
    std::atomic<float>* osc2UnisonParam = nullptr;
    std::atomic<float>* osc2PanSpreadParam = nullptr;

    // OSC 3
    std::atomic<float>* osc3LevelParam = nullptr;
    std::atomic<float>* osc3MorphParam = nullptr;
    std::atomic<float>* osc3DetuneParam = nullptr;
    std::atomic<float>* osc3PanParam = nullptr;
    std::atomic<float>* osc3UnisonParam = nullptr;
    std::atomic<float>* osc3PanSpreadParam = nullptr;

    // Sub Oscillator
    std::atomic<float>* subLevelParam = nullptr;
    std::atomic<float>* subOctaveParam = nullptr;
    std::atomic<float>* subWaveformParam = nullptr;

    // Filter
    std::atomic<float>* filterCutoffParam = nullptr;
    std::atomic<float>* filterResonanceParam = nullptr;
    std::atomic<float>* filterDriveParam = nullptr;
    std::atomic<float>* filterModeParam = nullptr;

    // Envelope 1 (Main/Amp)
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* decayParam = nullptr;
    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;

    // Envelope 2 (Filter)
    std::atomic<float>* env2AttackParam = nullptr;
    std::atomic<float>* env2DecayParam = nullptr;
    std::atomic<float>* env2SustainParam = nullptr;
    std::atomic<float>* env2ReleaseParam = nullptr;

    // Waveshaper
    std::atomic<float>* shaperModeParam = nullptr;
    std::atomic<float>* shaperAmountParam = nullptr;
    std::atomic<float>* shaperMixParam = nullptr;

    // FM Modulation
    std::atomic<float>* fm12Param = nullptr;
    std::atomic<float>* fm13Param = nullptr;
    std::atomic<float>* fm23Param = nullptr;

    // AM Modulation
    std::atomic<float>* am12Param = nullptr;
    std::atomic<float>* am13Param = nullptr;
    std::atomic<float>* am23Param = nullptr;

    // Wavetable Selection
    std::atomic<float>* wavetableIndexParam = nullptr;

    // LFO 1
    std::atomic<float>* lfo1WaveformParam = nullptr;
    std::atomic<float>* lfo1RateParam = nullptr;
    std::atomic<float>* lfo1DepthParam = nullptr;
    std::atomic<float>* lfo1TempoSyncParam = nullptr;
    std::atomic<float>* lfo1TempoRateParam = nullptr;

    // LFO 2
    std::atomic<float>* lfo2WaveformParam = nullptr;
    std::atomic<float>* lfo2RateParam = nullptr;
    std::atomic<float>* lfo2DepthParam = nullptr;
    std::atomic<float>* lfo2TempoSyncParam = nullptr;
    std::atomic<float>* lfo2TempoRateParam = nullptr;

    // LFO 3
    std::atomic<float>* lfo3WaveformParam = nullptr;
    std::atomic<float>* lfo3RateParam = nullptr;
    std::atomic<float>* lfo3DepthParam = nullptr;
    std::atomic<float>* lfo3TempoSyncParam = nullptr;
    std::atomic<float>* lfo3TempoRateParam = nullptr;

    // LFO 4
    std::atomic<float>* lfo4WaveformParam = nullptr;
    std::atomic<float>* lfo4RateParam = nullptr;
    std::atomic<float>* lfo4DepthParam = nullptr;
    std::atomic<float>* lfo4TempoSyncParam = nullptr;
    std::atomic<float>* lfo4TempoRateParam = nullptr;

    // FX - Chorus
    std::atomic<float>* chorusMixParam = nullptr;
    std::atomic<float>* chorusRateParam = nullptr;
    std::atomic<float>* chorusDepthParam = nullptr;

    // FX - Delay
    std::atomic<float>* delayMixParam = nullptr;
    std::atomic<float>* delayTimeParam = nullptr;
    std::atomic<float>* delayFeedbackParam = nullptr;

    // FX - Reverb
    std::atomic<float>* reverbMixParam = nullptr;
    std::atomic<float>* reverbSizeParam = nullptr;
    std::atomic<float>* reverbDampingParam = nullptr;

    // Update engine parameters from APVTS values
    void updateEngineParameters();

    // Create parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ============================================================
    // Preset Management
    // ============================================================

    int currentProgramIndex = 0;
    juce::StringArray programNames;

    void initPresetNames();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonSakuraProcessor)
};
