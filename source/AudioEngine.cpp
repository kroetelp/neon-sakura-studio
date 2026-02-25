#include "AudioEngine.h"
#include "TrackComponent.h"
#include "TrackType.h"
#include "Modulation/ModulationSource.h"
#include "WavetableSynth/WavetableSynth.h"

AudioEngine::AudioEngine(std::array<std::unique_ptr<TrackComponent>, numTracks>& tracksRef)
    : tracks(tracksRef)
{
}

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    currentSampleRate = newSampleRate;

    // Initialize master reverb
    masterReverb.setSampleRate(newSampleRate);
    reverbParams.roomSize = 0.8f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = reverbWetLevel.load();
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    masterReverb.setParameters(reverbParams);

    // Initialize Wavetable Synth
    wavetableEngine.prepareToPlay(samplesPerBlockExpected, newSampleRate);
    wavetableBuffer = std::make_unique<juce::AudioBuffer<float>>(2, samplesPerBlockExpected);

    // Pre-allocate track buffers and modulation filters to avoid allocation in audio thread
    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers[i] = std::make_unique<juce::AudioBuffer<float>>(2, samplesPerBlockExpected);
        modulationFilters[i] = std::make_unique<FilterProcessor>();
        modulationFilters[i]->reset();
        tracks[i]->getSynthesiser().setCurrentPlaybackSampleRate(newSampleRate);
        tracks[i]->prepareAudio(newSampleRate, samplesPerBlockExpected);
    }
    calculateSamplesPerStep();
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    const int localSamplesPerStep = samplesPerStep.load();
    const bool localIsPlaying = playing.load();
    const float swingVal = swingAmount.load();

    std::array<juce::MidiBuffer, numTracks> trackMidiBuffers;

    // Random number generator for probability modifier (using member for reproducibility)
    std::uniform_int_distribution<int> probDist(1, 100);

    if (localIsPlaying && localSamplesPerStep > 0)
    {
        const int swingOffsetSamples = (int)(localSamplesPerStep * swingVal);

        // OPTIMIZED: Process step changes instead of every sample
        const uint64_t startSamplePos = samplePosition.load();
        const uint64_t endSamplePos = startSamplePos + bufferToFill.numSamples;

        // Process each track with its OWN loop length for polyrhythms
        for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
        {
            // PER-TRACK loop length for polyrhythms
            const int trackLoopLen = tracks[trackIdx]->getTrackLoopLength();

            // Iterate through steps in this buffer
            for (uint64_t samplePos = startSamplePos; samplePos < endSamplePos; )
            {
                // Calculate step based on THIS track's loop length
                const int currentStep = (int)(samplePos / localSamplesPerStep) % trackLoopLen;
                const int nextStepBoundary = ((int)(samplePos / localSamplesPerStep) + 1) * localSamplesPerStep;
                const int samplesUntilNextStep = (int)juce::jmin((uint64_t)nextStepBoundary - samplePos, endSamplePos - samplePos);

                if (tracks[trackIdx]->isStepActive(currentStep))
                {
                    const StepModifierState stepState = tracks[trackIdx]->getStepState(currentStep);

                    // P-Lock: Use locked values if available, otherwise use track defaults
                    const float trackVolume = tracks[trackIdx]->getVolume();
                    const int trackPitch = tracks[trackIdx]->getPitch();
                    const float finalVolume = stepState.hasVolLock ? stepState.volLock : trackVolume;
                    const int finalPitch = stepState.hasPitchLock ? stepState.pitchLock : trackPitch;

                    const juce::uint8 velocity = static_cast<juce::uint8>(finalVolume * 127);
                    const int midiNote = 60 + finalPitch;

                    const bool stepChanged = (currentStep != trackLastStep[trackIdx]);

                    // Calculate sample index within buffer
                    const int sampleIndexInBuffer = (int)(samplePos - startSamplePos);

                    // Apply swing offset for odd steps
                    int eventIndex = sampleIndexInBuffer;
                    if (currentStep % 2 != 0 && swingVal > 0.0f)
                    {
                        eventIndex = juce::jmin(sampleIndexInBuffer + swingOffsetSamples, bufferToFill.numSamples - 1);
                    }

                    if (stepChanged)
                    {
                        // Handle different modifiers
                        bool shouldTrigger = false;

                        if (stepState.modifierType == '/')
                        {
                            // Slow modifier - trigger every Nth loop
                            if (globalLoopCounter % stepState.modifierValue == 0)
                                shouldTrigger = true;
                        }
                        else if (stepState.modifierType == '?')
                        {
                            // Probability modifier - roll dice once per step trigger
                            int roll = probDist(probabilityRng);
                            if (roll <= stepState.modifierValue)
                                shouldTrigger = true;
                        }
                        else
                        {
                            shouldTrigger = true;
                        }

                        if (shouldTrigger)
                        {
                            juce::MidiMessage midiMessage = juce::MidiMessage::noteOn(1, midiNote, velocity);
                            trackMidiBuffers[trackIdx].addEvent(midiMessage, eventIndex);

                            // Calculate note-off time based on gate ratio (default 80% of step)
                            const float gateRatio = 0.8f;  // Could be made adjustable per track
                            const int noteOffDelay = juce::jmax(1, static_cast<int>(localSamplesPerStep * gateRatio));
                            const int noteOffIndex = juce::jmin(eventIndex + noteOffDelay, bufferToFill.numSamples - 1);

                            juce::MidiMessage noteOffMessage = juce::MidiMessage::noteOff(1, midiNote);
                            trackMidiBuffers[trackIdx].addEvent(noteOffMessage, noteOffIndex);
                        }

                        trackLastStep[trackIdx] = currentStep;
                    }

                    // Handle ratchet (speed) modifier
                    if (stepState.modifierType == '*' && localSamplesPerStep > 0)
                    {
                        const int samplesPerRatchet = localSamplesPerStep / stepState.modifierValue;
                        const int currentRatchet = (int)((samplePos % localSamplesPerStep) / samplesPerRatchet);

                        if (currentRatchet != trackLastRatchet[trackIdx] && currentRatchet > 0)
                        {
                            const int ratchetSampleIndex = (int)(samplePos - startSamplePos);
                            int ratchetEventIndex = ratchetSampleIndex;
                            if (currentStep % 2 != 0 && swingVal > 0.0f)
                            {
                                ratchetEventIndex = juce::jmin(ratchetSampleIndex + swingOffsetSamples, bufferToFill.numSamples - 1);
                            }

                            juce::MidiMessage midiMessage = juce::MidiMessage::noteOn(1, midiNote, velocity);
                            trackMidiBuffers[trackIdx].addEvent(midiMessage, ratchetEventIndex);

                            // Add note-off for ratchet notes
                            const int ratchetGateTime = juce::jmax(1, samplesPerRatchet / 2);
                            const int ratchetNoteOffIndex = juce::jmin(ratchetEventIndex + ratchetGateTime, bufferToFill.numSamples - 1);
                            juce::MidiMessage ratchetNoteOffMessage = juce::MidiMessage::noteOff(1, midiNote);
                            trackMidiBuffers[trackIdx].addEvent(ratchetNoteOffMessage, ratchetNoteOffIndex);
                        }
                        trackLastRatchet[trackIdx] = currentRatchet;
                    }
                }

                // Move to next step boundary or end of buffer
                samplePos += samplesUntilNextStep;
            }
        }

        // Update global sample position
        samplePosition.store(endSamplePos);

        // Update global loop counter based on global loop length
        const int globalLoopLen = loopLength.load();
        const uint64_t globalStep = endSamplePos / localSamplesPerStep;
        if (globalStep / globalLoopLen > lastGlobalLoopCheck)
        {
            globalLoopCounter++;
            lastGlobalLoopCheck = globalStep / globalLoopLen;
        }
    }

    // Render all tracks and mix into buffer
    renderAndMixTracks(bufferToFill, trackMidiBuffers);

    // Apply master reverb
    applyMasterReverb(*bufferToFill.buffer);
}

void AudioEngine::releaseResources()
{
    // Clean up buffers and filters
    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers[i].reset();
        if (modulationFilters[i])
            modulationFilters[i]->reset();
    }

    // Clean up wavetable buffer
    wavetableBuffer.reset();
    wavetableMidiBuffer.clear();
    wavetableEngine.releaseResources();
}

void AudioEngine::startPlayback()
{
    playing.store(true);
}

void AudioEngine::stopPlayback()
{
    playing.store(false);

    globalLoopCounter = 0;
    lastGlobalLoopCheck = 0;

    for (auto& track : tracks)
    {
        track->getSynthesiser().allNotesOff(0, false);
    }

    samplePosition.store(0);

    // Reset per-track state
    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }
}

void AudioEngine::resetPlaybackPosition()
{
    globalLoopCounter = 0;
    lastGlobalLoopCheck = 0;

    for (auto& track : tracks)
    {
        track->getSynthesiser().allNotesOff(0, false);
    }

    samplePosition.store(0);

    // Reset per-track state
    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }
}

void AudioEngine::setBPM(double newBpm)
{
    bpm.store(newBpm);
    calculateSamplesPerStep();
    wavetableEngine.setBPM(static_cast<float>(newBpm));
}

void AudioEngine::setSwingAmount(float swing)
{
    swingAmount.store(swing);
}

void AudioEngine::setLoopLength(int steps)
{
    loopLength.store(steps);
    // Reset per-track state when loop length changes
    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }
}

void AudioEngine::setMasterVolume(float volume)
{
    masterVolume.store(volume);
}

void AudioEngine::setReverbWetLevel(float wetLevel)
{
    reverbWetLevel.store(wetLevel);
}

void AudioEngine::resetTrackStates()
{
    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }
}

void AudioEngine::calculateSamplesPerStep()
{
    const double bpmValue = bpm.load();
    const double secondsPerBeat = 60.0 / bpmValue;
    const double secondsPerStep = secondsPerBeat / 4.0;
    samplesPerStep.store(static_cast<int>(currentSampleRate * secondsPerStep));
}

void AudioEngine::renderAndMixTracks(const juce::AudioSourceChannelInfo& bufferToFill,
                                     const std::array<juce::MidiBuffer, numTracks>& trackMidiBuffers)
{
    const float vol = masterVolume.load();

    // Check if any track has solo enabled
    bool anySolo = false;
    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        if (tracks[trackIdx]->getSolo())
        {
            anySolo = true;
            break;
        }
    }

    // Check if Wavetable is in Modulator mode
    const bool isModulatorMode = wavetableEngine.getMode() == WavetableEngine::Mode::Modulator;

    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        // Check mute/solo state
        const bool trackMuted = tracks[trackIdx]->getMuted();
        const bool trackSolo = tracks[trackIdx]->getSolo();

        // Skip track if muted, or if another track is soloed and this one isn't
        if (trackMuted || (anySolo && !trackSolo))
            continue;

        // Get track type
        const TrackType trackType = tracks[trackIdx]->getTrackType();

        // Apply wavetable modulation to track parameters if enabled (sampler tracks only)
        if (trackType == TrackType::Sampler && isModulatorMode && tracks[trackIdx]->getWavetableModulationEnabled())
        {
            // Get modulation values from wavetable engine
            float cutoffMod = wavetableEngine.getModulationValue(trackIdx, ModulationTarget::Filter_Cutoff);
            float pitchMod = wavetableEngine.getModulationValue(trackIdx, ModulationTarget::Osc1_Pitch);

            // Apply filter cutoff modulation (exponential scaling for musical response)
            if (cutoffMod != 0.0f)
            {
                float currentCutoff = tracks[trackIdx]->getCutoff();
                // Scale modulation logarithmically for cutoff
                float modFactor = 1.0f + (cutoffMod * 2.0f);  // -1 to +1 becomes 0x to 3x
                float newCutoff = currentCutoff * modFactor;
                newCutoff = juce::jlimit(20.0f, 20000.0f, newCutoff);
            }
        }

        // Use pre-allocated buffer instead of creating new one each block
        auto& tempBuffer = *trackBuffers[trackIdx];

        // Ensure buffer is large enough (in case block size changed)
        if (tempBuffer.getNumSamples() < bufferToFill.numSamples)
        {
            tempBuffer.setSize(2, bufferToFill.numSamples);
        }

        tempBuffer.clear();

        // Render based on track type
        if (trackType == TrackType::Wavetable)
        {
            // Render wavetable synth directly (per-track synth instance)
            auto* wtSynth = tracks[trackIdx]->getWavetableSynth();
            if (wtSynth)
            {
                // Process modulations before rendering
                wtSynth->processModulations();

                // Render the wavetable synth
                wtSynth->renderNextBlock(tempBuffer,
                    trackMidiBuffers[trackIdx], 0, bufferToFill.numSamples);
            }

            // Apply synth-specific audio processing (filter, etc.)
            tracks[trackIdx]->processAudioBlock(tempBuffer);
        }
        else
        {
            // Sampler mode - render sampler
            tracks[trackIdx]->getSynthesiser().renderNextBlock(tempBuffer,
                trackMidiBuffers[trackIdx], 0, bufferToFill.numSamples);

            // Apply modulation-affected filter processing (sampler only)
            if (isModulatorMode && tracks[trackIdx]->getWavetableModulationEnabled())
            {
                float cutoffMod = wavetableEngine.getModulationValue(trackIdx, ModulationTarget::Filter_Cutoff);
                if (cutoffMod != 0.0f)
                {
                    // Apply modulated filter to the rendered audio
                    float currentCutoff = tracks[trackIdx]->getCutoff();
                    float modFactor = 1.0f + (cutoffMod * 2.0f);
                    float newCutoff = juce::jlimit(20.0f, 20000.0f, currentCutoff * modFactor);

                    // Use pre-allocated filter (no allocation in audio thread)
                    *modulationFilters[trackIdx]->state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                        currentSampleRate, newCutoff, 0.5f);

                    juce::dsp::AudioBlock<float> block(tempBuffer);
                    modulationFilters[trackIdx]->process(juce::dsp::ProcessContextReplacing<float>(block));
                }
            }

            tracks[trackIdx]->processAudioBlock(tempBuffer);
        }

        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            bufferToFill.buffer->addFrom(channel, 0, tempBuffer, channel, 0, bufferToFill.numSamples, vol);
        }
    }

    // Process Wavetable Synth in Standalone mode
    if (wavetableEngine.getMode() == WavetableEngine::Mode::Standalone)
    {
        // Ensure buffer is large enough
        if (wavetableBuffer->getNumSamples() < bufferToFill.numSamples)
        {
            wavetableBuffer->setSize(2, bufferToFill.numSamples);
        }

        wavetableBuffer->clear();
        wavetableMidiBuffer.clear();

        // Process the wavetable synth
        wavetableEngine.processBlock(*wavetableBuffer, wavetableMidiBuffer);

        // Mix into main buffer
        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            bufferToFill.buffer->addFrom(channel, 0, *wavetableBuffer, channel, 0, bufferToFill.numSamples, vol);
        }
    }
}

void AudioEngine::applyMasterReverb(juce::AudioBuffer<float>& buffer)
{
    reverbParams.wetLevel = reverbWetLevel.load();
    masterReverb.setParameters(reverbParams);

    if (buffer.getNumChannels() >= 2)
    {
        float* left = buffer.getWritePointer(0, 0);
        float* right = buffer.getWritePointer(1, 0);
        masterReverb.processStereo(left, right, buffer.getNumSamples());
    }
}
