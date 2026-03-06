#pragma once

/**
 * MasterOutputProcessor - Final output stage for the DAW
 *
 * This processor sits at the end of the audio chain and handles:
 *   - Master volume control
 *   - Master reverb
 *   - Soft clipping / limiting for output protection
 *   - Peak metering
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

class MasterOutputProcessor : public juce::AudioProcessor
{
public:
    MasterOutputProcessor();
    ~MasterOutputProcessor() override;

    // ============================================================
    // AudioProcessor Interface
    // ============================================================

    const juce::String getName() const override { return "Master Output"; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    double getTailLengthSeconds() const override { return 0.5; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    // ============================================================
    // Master Controls
    // ============================================================

    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume.load(); }

    void setMasterPan(float pan);
    float getMasterPan() const { return masterPan.load(); }

    void setMasterMute(bool muted);
    bool isMasterMuted() const { return masterMuted.load(); }

    void setReverbWetLevel(float wetLevel);
    float getReverbWetLevel() const { return reverbWetLevel.load(); }

    void setReverbRoomSize(float roomSize);
    void setReverbDamping(float damping);

    // ============================================================
    // Metering
    // ============================================================

    float getPeakLevel() const { return peakLevel.load(); }
    float getPeakLevelLeft() const { return peakLeft.load(); }
    float getPeakLevelRight() const { return peakRight.load(); }

private:
    std::atomic<float> masterVolume{0.8f};
    std::atomic<float> masterPan{0.0f};
    std::atomic<bool> masterMuted{false};
    std::atomic<float> reverbWetLevel{0.0f};
    std::atomic<float> reverbRoomSize{0.5f};
    std::atomic<float> reverbDamping{0.5f};

    std::atomic<float> peakLevel{0.0f};
    std::atomic<float> peakLeft{0.0f};
    std::atomic<float> peakRight{0.0f};

    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                    juce::dsp::IIR::Coefficients<float>> dcFilter;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    void updateReverbParams();
    static float softClip(float sample);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterOutputProcessor)
};
