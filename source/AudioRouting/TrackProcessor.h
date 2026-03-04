#pragma once

/**
 * TrackProcessor - AudioProcessor wrapper for a single DAW track
 *
 * Each track in the DAW is represented by a TrackProcessor node.
 * This processor handles:
 *   - Audio rendering from the track's synthesizer or sampler
 *   - MIDI input from the sequencer/timeline
 *   - Volume, pan, mute, solo
 *   - Parameter automation via AutomationManager
 *   - Sidechain input for plugin sidechain routing
 *
 * Implements IAudioSource for unified audio source interface.
 * This enables:
 *   - Latency compensation across tracks
 *   - Tail time handling for reverb/delay
 *   - Consistent interface for all audio sources
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>
#include "IAudioSource.h"

class ITrackDataProvider;
class WavetableEngine;
class TimelineTrack;
class AutomationManager;
class SidechainBuffer;

class TrackProcessor : public juce::AudioProcessor, public IAudioSource
{
public:
    /** Constructor */
    TrackProcessor(int index, ITrackDataProvider* provider);

    /** Destructor */
    ~TrackProcessor() override;

    // ============================================================
    // Wavetable Engine (for synthesis tracks)
    // ============================================================

    /** Assign a WavetableEngine for synthesis. */
    void setWavetableEngine(WavetableEngine* engine);

    // ============================================================
    // Timeline Track Connection (for automation)
    // ============================================================

    /** Connect to TimelineTrack for automation data. */
    void setTimelineTrack(TimelineTrack* track);

    /** Get connected TimelineTrack. */
    TimelineTrack* getTimelineTrack() const { return timelineTrack; }

    // ============================================================
    // AudioProcessor Interface
    // ============================================================

    const juce::String getName() const override { return "Track " + juce::String(trackIndex + 1); }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<double>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    double getTailLengthSeconds() const override { return 0.0; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }

    // ============================================================
    // IAudioSource Interface Implementation
    // ============================================================

    /** IAudioSource::getLatencySamples() - delegates to AudioProcessor */
    int getLatencySamples() const override { return AudioProcessor::getLatencySamples(); }

    /** IAudioSource::getTailSeconds() - delegates to AudioProcessor */
    double getTailSeconds() const override { return getTailLengthSeconds(); }

    /** IAudioSource::supportsDoublePrecision() - we have double processing */
    bool supportsDoublePrecision() const override { return true; }

    /** IAudioSource::processBlockDouble() - implemented via AudioProcessor */
    void processBlockDouble(juce::AudioBuffer<double>& buffer,
                            juce::MidiBuffer& midi) override
    {
        processBlock(buffer, midi);
    }

    /** IAudioSource::isBypassed() */
    bool isBypassed() const override { return bypassed.load(); }

    /** IAudioSource::setBypassed() */
    void setBypassed(bool bypass) override { bypassed.store(bypass); }

    // ============================================================
    // Metering
    // ============================================================

    float getPeakLevel() const { return peakLevel.load(); }

    // ============================================================
    // MIDI Accumulation
    // ============================================================

    void addMidiBuffer(const juce::MidiBuffer& buffer);

    // ============================================================
    // Plugin Chain Support (for latency propagation)
    // ============================================================

    /** Set the cumulative latency from plugins in this track's chain.
        This is called by AudioRoutingGraph when plugins are added/removed. */
    void setPluginChainLatency(int latencySamples) { pluginChainLatency = latencySamples; }

    /** Get the total latency including plugins. */
    int getTotalLatencySamples() const { return getLatencySamples() + pluginChainLatency; }

    // ============================================================
    // Sidechain Input Support
    // ============================================================

    /** Set the sidechain buffer for this track.
        The sidechain buffer contains audio from other tracks routed as sidechain input. */
    void setSidechainBuffer(SidechainBuffer* buffer) { sidechainBuffer = buffer; }

    /** Get the sidechain buffer for this track. */
    SidechainBuffer* getSidechainBuffer() const { return sidechainBuffer; }

    /** Check if this track has sidechain input available. */
    bool hasSidechainInput() const { return sidechainBuffer != nullptr; }

private:
    int trackIndex;
    ITrackDataProvider* trackProvider;
    WavetableEngine* wavetableEngine = nullptr;
    TimelineTrack* timelineTrack = nullptr;
    std::atomic<float> peakLevel{0.0f};
    std::atomic<bool> bypassed{false};
    double currentSampleRate = 44100.0;
    int pluginChainLatency = 0;  // Cumulative latency from plugins in chain
    double lastPlayheadBeat = 0.0;  // For automation processing
    SidechainBuffer* sidechainBuffer = nullptr;  // Sidechain input buffer (non-owning)

    /** Apply automation values to WavetableEngine parameters */
    void applyWavetableAutomation(const std::unordered_map<juce::String, float>& values);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackProcessor)
};
