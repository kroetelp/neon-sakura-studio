#include "AudioEngine.h"
#include "TrackComponent.h"
#include <random>
#include <chrono>

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

    // Pre-allocate track buffers to avoid allocation in audio thread
    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers[i] = std::make_unique<juce::AudioBuffer<float>>(2, samplesPerBlockExpected);
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

    // Random number generator for probability modifier
    static thread_local std::mt19937 probRng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()));
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
                            int roll = probDist(probRng);
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
        static uint64_t lastGlobalLoopCheck = 0;
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
    // Clean up buffers
    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers[i].reset();
    }
}

void AudioEngine::startPlayback()
{
    playing.store(true);
}

void AudioEngine::stopPlayback()
{
    playing.store(false);

    globalLoopCounter = 0;

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

    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        // Check mute/solo state
        const bool trackMuted = tracks[trackIdx]->getMuted();
        const bool trackSolo = tracks[trackIdx]->getSolo();

        // Skip track if muted, or if another track is soloed and this one isn't
        if (trackMuted || (anySolo && !trackSolo))
            continue;

        // Use pre-allocated buffer instead of creating new one each block
        auto& tempBuffer = *trackBuffers[trackIdx];

        // Ensure buffer is large enough (in case block size changed)
        if (tempBuffer.getNumSamples() < bufferToFill.numSamples)
        {
            tempBuffer.setSize(2, bufferToFill.numSamples);
        }

        tempBuffer.clear();

        tracks[trackIdx]->getSynthesiser().renderNextBlock(tempBuffer,
            trackMidiBuffers[trackIdx], 0, bufferToFill.numSamples);

        tracks[trackIdx]->processAudioBlock(tempBuffer);

        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            bufferToFill.buffer->addFrom(channel, 0, tempBuffer, channel, 0, bufferToFill.numSamples, vol);
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
