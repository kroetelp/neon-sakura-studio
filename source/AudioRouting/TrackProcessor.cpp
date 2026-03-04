#include "TrackProcessor.h"
#include "SidechainManager.h"
#include "../WavetableSynth/WavetableEngine.h"
#include "../WavetableSynth/WavetableSynth.h"
#include "../WavetableSynth/WavetableParams.h"
#include "../ITrackDataProvider.h"
#include "../TrackType.h"
#include "../Timeline/TimelineTrack.h"
#include "../Timeline/AutomationLane.h"

//==============================================================================
TrackProcessor::TrackProcessor(int index, ITrackDataProvider* provider)
    : trackIndex(index)
    , trackProvider(provider)
{
}

//==============================================================================
TrackProcessor::~TrackProcessor()
{
}

//==============================================================================
void TrackProcessor::setWavetableEngine(WavetableEngine* engine)
{
    wavetableEngine = engine;
}

//==============================================================================
void TrackProcessor::setTimelineTrack(TimelineTrack* track)
{
    timelineTrack = track;
}

//==============================================================================
void TrackProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::ignoreUnused(samplesPerBlock);
}

//==============================================================================
void TrackProcessor::releaseResources()
{
}

//==============================================================================
void TrackProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    if (!trackProvider)
        return;

    // Check mute/solo state
    bool anySolo = false;
    for (int i = 0; i < 16; ++i)
    {
        if (trackProvider->getSolo(i))
        {
            anySolo = true;
            break;
        }
    }

    if (trackProvider->getMuted(trackIndex) || (anySolo && !trackProvider->getSolo(trackIndex)))
        return;

    // === Process Automation ===
    // Get current playhead position and apply automation to wavetable parameters
    if (timelineTrack)
    {
        // Get playhead position from track provider (if available)
        double currentBeat = trackProvider->getPlayheadBeat();

        // Process automation and get parameter values
        auto automationValues = timelineTrack->processAutomation(currentBeat);

        // Apply automation to wavetable engine if this is a wavetable track
        if (!automationValues.empty() && trackProvider->getTrackType(trackIndex) == TrackType::Wavetable)
        {
            applyWavetableAutomation(automationValues);
        }

        lastPlayheadBeat = currentBeat;
    }

    // Get track type
    TrackType trackType = trackProvider->getTrackType(trackIndex);

    switch (trackType)
    {
        case TrackType::Sampler:
            // Render using the track's sampler/synthesiser
            {
                auto& synth = trackProvider->getSynthesiser(trackIndex);
                synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
            }
            break;

        case TrackType::Wavetable:
            // Render using the wavetable synth
            if (wavetableEngine)
            {
                // Process modulations first
                wavetableEngine->processBlock(buffer, midiMessages);
            }
            else
            {
                auto* wtSynth = trackProvider->getWavetableSynth(trackIndex);
                if (wtSynth)
                {
                    wtSynth->processModulations();
                    wtSynth->renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
                }
            }
            break;

        case TrackType::Audio:
            // Audio input tracks would process input buffer here
            break;

        case TrackType::Plugin:
            // Plugin tracks are handled separately via insertPluginAfterTrack
            break;
    }

    // Let track provider do additional processing
    trackProvider->processAudioBlock(trackIndex, buffer);

    // Update peak level for metering
    float currentPeak = buffer.getMagnitude(0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 1)
        currentPeak = juce::jmax(currentPeak, buffer.getMagnitude(1, buffer.getNumSamples()));
    peakLevel.store(currentPeak);
}

//==============================================================================
void TrackProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Convert to float, process, convert back
    juce::AudioBuffer<float> floatBuffer(buffer.getNumChannels(), buffer.getNumSamples());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* src = buffer.getReadPointer(channel);
        auto* dst = floatBuffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            dst[sample] = static_cast<float>(src[sample]);
    }

    processBlock(floatBuffer, midiMessages);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* src = floatBuffer.getReadPointer(channel);
        auto* dst = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            dst[sample] = static_cast<double>(src[sample]);
    }
}

//==============================================================================
void TrackProcessor::addMidiBuffer(const juce::MidiBuffer& buffer)
{
    juce::ignoreUnused(buffer);
    // Could accumulate MIDI for later processing
}

//==============================================================================
void TrackProcessor::applyWavetableAutomation(const std::unordered_map<juce::String, float>& values)
{
    if (values.empty())
        return;

    // Get WavetableParams from the synth (if available)
    std::shared_ptr<WavetableParams> params;

    if (wavetableEngine)
    {
        params = wavetableEngine->getSynthesiser().getSharedParams();
    }
    else if (trackProvider)
    {
        auto* wtSynth = trackProvider->getWavetableSynth(trackIndex);
        if (wtSynth)
            params = wtSynth->getSharedParams();
    }

    if (!params)
        return;

    for (const auto& [paramId, value] : values)
    {
        // Map parameter IDs to WavetableParams
        // These IDs should match what's used in AutomationLane

        if (paramId == "wt_osc1_morph" || paramId == "osc1Morph")
        {
            params->setOscMorph(0, value);
        }
        else if (paramId == "wt_osc2_morph" || paramId == "osc2Morph")
        {
            params->setOscMorph(1, value);
        }
        else if (paramId == "wt_osc3_morph" || paramId == "osc3Morph")
        {
            params->setOscMorph(2, value);
        }
        else if (paramId == "wt_osc1_level" || paramId == "osc1Level")
        {
            params->setOscLevel(0, value);
        }
        else if (paramId == "wt_osc2_level" || paramId == "osc2Level")
        {
            params->setOscLevel(1, value);
        }
        else if (paramId == "wt_osc3_level" || paramId == "osc3Level")
        {
            params->setOscLevel(2, value);
        }
        else if (paramId == "wt_cutoff" || paramId == "filterCutoff")
        {
            // Scale from normalized (0-1) to frequency (20Hz-20kHz, logarithmic)
            float freq = 20.0f * std::pow(1000.0f, value);
            params->setFilterCutoff(freq);
        }
        else if (paramId == "wt_resonance" || paramId == "filterResonance")
        {
            params->setFilterResonance(value);
        }
        else if (paramId == "wt_masterLevel" || paramId == "masterVolume")
        {
            params->setMasterLevel(value);
        }
        else if (paramId == "wt_attack")
        {
            params->setEnvAttack(value);
        }
        else if (paramId == "wt_decay")
        {
            params->setEnvDecay(value);
        }
        else if (paramId == "wt_sustain")
        {
            params->setEnvSustain(value);
        }
        else if (paramId == "wt_release")
        {
            params->setEnvRelease(value);
        }
        // FX parameters (via WavetableEngine)
        else if (paramId == "wt_chorusMix" && wavetableEngine)
        {
            wavetableEngine->getFXParams().chorusMix.store(value);
        }
        else if (paramId == "wt_delayMix" && wavetableEngine)
        {
            wavetableEngine->getFXParams().delayMix.store(value);
        }
        else if (paramId == "wt_reverbMix" && wavetableEngine)
        {
            wavetableEngine->getFXParams().reverbMix.store(value);
        }
        else if (paramId == "wt_delayTime" && wavetableEngine)
        {
            // Scale to reasonable delay time range (0.05 to 2.0 seconds)
            float delayTime = 0.05f + value * 1.95f;
            wavetableEngine->getFXParams().delayTime.store(delayTime);
        }
        else if (paramId == "wt_reverbSize" && wavetableEngine)
        {
            wavetableEngine->getFXParams().reverbSize.store(value);
        }
        // Add more parameter mappings as needed
    }
}
