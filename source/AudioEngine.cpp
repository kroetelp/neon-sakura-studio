#include "AudioEngine.h"
#include "PlaybackController.h"
#include "TrackType.h"
#include "TrackModel.h"
#include "Modulation/ModulationSource.h"
#include "WavetableSynth/WavetableSynth.h"
#include "Timeline/TimelineData.h"
#include "Timeline/TimelineTransport.h"
#include "Timeline/TimelineRenderer.h"
#include "Timeline/RecordingManager.h"

AudioEngine::AudioEngine(ITrackDataProvider* provider)
    : trackProvider(provider)
    , playbackController(nullptr)
{
    // Initialize Timeline components
    timelineData = std::make_unique<TimelineData>();
    timelineTransport = std::make_unique<TimelineTransport>(*timelineData);
    timelineRenderer = std::make_unique<TimelineRenderer>(*timelineData, *timelineTransport);
    recordingManager = std::make_unique<RecordingManager>(*timelineData);
}

AudioEngine::AudioEngine(ITrackDataProvider* provider, PlaybackController* controller)
    : trackProvider(provider)
    , playbackController(controller)
{
    // Initialize Timeline components
    timelineData = std::make_unique<TimelineData>();
    timelineTransport = std::make_unique<TimelineTransport>(*timelineData);
    timelineRenderer = std::make_unique<TimelineRenderer>(*timelineData, *timelineTransport);
    recordingManager = std::make_unique<RecordingManager>(*timelineData);
}

AudioEngine::~AudioEngine() = default;

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

    // Use maximum buffer size to prevent dynamic allocation in audio thread
    const int safeBufferSize = juce::jmax(samplesPerBlockExpected, maxBufferSize);

    // Initialize Wavetable Synth
    wavetableEngine.prepareToPlay(safeBufferSize, newSampleRate);
    wavetableBuffer = std::make_unique<juce::AudioBuffer<float>>(2, safeBufferSize);

    // Filter Spezifikationen vorbereiten
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(safeBufferSize);
    spec.numChannels = 2;

    // Pre-allocate track buffers with maximum size (NO allocation during playback!)
    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers[i] = std::make_unique<juce::AudioBuffer<float>>(2, safeBufferSize);

        // Initialisiere real-time sicheren Filter
        modulationFilters[i] = std::make_unique<juce::dsp::StateVariableTPTFilter<float>>();
        modulationFilters[i]->setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        modulationFilters[i]->prepare(spec);
        modulationFilters[i]->reset();

        trackProvider->getSynthesiser(i).setCurrentPlaybackSampleRate(newSampleRate);
    }

    // Sync with PlaybackController if available
    if (playbackController)
    {
        playbackController->setSampleRate(newSampleRate);
        samplesPerStep.store(playbackController->getSamplesPerStep());
    }
    else
    {
        calculateSamplesPerStep();
    }

    // Prepare Timeline renderer
    timelineRenderer->prepareToPlay(newSampleRate, safeBufferSize);
    timelineRenderer->setWavetableEngine(&wavetableEngine);
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    // Check engine mode and route accordingly
    if (engineMode.load() == EngineMode::Timeline)
    {
        // Timeline/DAW mode
        timelineRenderer->processBlock(*bufferToFill.buffer, wavetableMidiBuffer);

        // Handle recording if active
        if (recordingManager->isRecording())
        {
            // Note: For recording, we'd need input buffer from AudioDeviceManager
            // This is a simplified version - full implementation would capture input
        }

        applyMasterReverb(*bufferToFill.buffer);
        return;
    }

    // Step Sequencer mode (original logic)

    const int localSamplesPerStep = samplesPerStep.load();
    const bool localIsPlaying = playing.load();
    const float swingVal = swingAmount.load();

    std::array<juce::MidiBuffer, numTracks> trackMidiBuffers;

    if (localIsPlaying && localSamplesPerStep > 0)
    {
        const int swingOffsetSamples = (int)(localSamplesPerStep * swingVal);

        const uint64_t startSamplePos = samplePosition.load();
        const uint64_t endSamplePos = startSamplePos + bufferToFill.numSamples;

        // Process each track with its OWN loop length for polyrhythms
        for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
        {
            // PER-TRACK loop length for polyrhythms
            const int trackLoopLen = trackProvider->getTrackLoopLength(trackIdx);

            // Iterate through steps in this buffer
            for (uint64_t samplePos = startSamplePos; samplePos < endSamplePos; )
            {
                // Calculate step based on THIS track's loop length
                const int currentStep = (int)(samplePos / localSamplesPerStep) % trackLoopLen;
                const int nextStepBoundary = ((int)(samplePos / localSamplesPerStep) + 1) * localSamplesPerStep;
                const int samplesUntilNextStep = (int)juce::jmin((uint64_t)nextStepBoundary - samplePos, endSamplePos - samplePos);

                if (trackProvider->isStepActive(trackIdx, currentStep))
                {
                    const StepModifierState stepState = trackProvider->getStepState(trackIdx, currentStep);

                    // P-Lock: Use locked values if available, otherwise use track defaults
                    const float trackVolume = trackProvider->getVolume(trackIdx);
                    const int trackPitch = trackProvider->getPitch(trackIdx);
                    const float finalVolume = stepState.hasVolLock ? stepState.volLock : trackVolume;
                    const int finalPitch = stepState.hasPitchLock ? stepState.pitchLock : trackPitch;

                    const juce::uint8 velocity = static_cast<juce::uint8>(finalVolume * 127);
                    const int midiNote = 60 + finalPitch;

                    const bool stepChanged = (currentStep != trackLastStep[trackIdx]);

                    const int sampleIndexInBuffer = (int)(samplePos - startSamplePos);

                    // Apply swing offset for odd steps
                    int eventIndex = sampleIndexInBuffer;
                    if (currentStep % 2 != 0 && swingVal > 0.0f)
                    {
                        eventIndex = juce::jmin(sampleIndexInBuffer + swingOffsetSamples, bufferToFill.numSamples - 1);
                    }

                    if (stepChanged)
                    {
                        bool shouldTrigger = false;

                        if (stepState.modifierType == '/')
                        {
                            // Slow modifier - trigger every Nth loop
                            if (globalLoopCounter % stepState.modifierValue == 0)
                                shouldTrigger = true;
                        }
                        else if (stepState.modifierType == '?')
                        {
                            // Probability modifier - use fast LCG-based RNG
                            int roll = probabilityRng.nextInt(100);
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

                            const float gateRatio = 0.8f;
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

                            const int ratchetGateTime = juce::jmax(1, samplesPerRatchet / 2);
                            const int ratchetNoteOffIndex = juce::jmin(ratchetEventIndex + ratchetGateTime, bufferToFill.numSamples - 1);
                            juce::MidiMessage ratchetNoteOffMessage = juce::MidiMessage::noteOff(1, midiNote);
                            trackMidiBuffers[trackIdx].addEvent(ratchetNoteOffMessage, ratchetNoteOffIndex);
                        }
                        trackLastRatchet[trackIdx] = currentRatchet;
                    }
                }

                samplePos += samplesUntilNextStep;
            }
        }

        // Update global sample position
        samplePosition.store(endSamplePos);

        // Update PlaybackController if available
        if (playbackController)
        {
            playbackController->setSamplePosition(endSamplePos);
        }

        // Update global loop counter
        const int globalLoopLen = loopLength.load();
        const uint64_t globalStep = endSamplePos / localSamplesPerStep;
        if (globalStep / globalLoopLen > lastGlobalLoopCheck)
        {
            globalLoopCounter++;
            lastGlobalLoopCheck = globalStep / globalLoopLen;
        }
    }

    renderAndMixTracks(bufferToFill, trackMidiBuffers);
    applyMasterReverb(*bufferToFill.buffer);
}

void AudioEngine::releaseResources()
{
    for (int i = 0; i < numTracks; ++i)
    {
        trackBuffers[i].reset();
        if (modulationFilters[i])
            modulationFilters[i]->reset();
    }

    wavetableBuffer.reset();
    wavetableMidiBuffer.clear();
    wavetableEngine.releaseResources();
}

void AudioEngine::startPlayback()
{
    playing.store(true);

    if (playbackController)
        playbackController->startPlayback();
}

void AudioEngine::stopPlayback()
{
    playing.store(false);

    globalLoopCounter = 0;
    lastGlobalLoopCheck = 0;

    for (int i = 0; i < numTracks; ++i)
    {
        trackProvider->getSynthesiser(i).allNotesOff(0, false);
    }

    samplePosition.store(0);

    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }

    if (playbackController)
        playbackController->stopPlayback();
}

void AudioEngine::resetPlaybackPosition()
{
    globalLoopCounter = 0;
    lastGlobalLoopCheck = 0;

    for (int i = 0; i < numTracks; ++i)
    {
        trackProvider->getSynthesiser(i).allNotesOff(0, false);
    }

    samplePosition.store(0);

    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }

    if (playbackController)
        playbackController->resetPlaybackPosition();
}

bool AudioEngine::isPlaying() const
{
    return playing.load();
}

void AudioEngine::setBPM(double newBpm)
{
    bpm.store(newBpm);
    calculateSamplesPerStep();
    wavetableEngine.setBPM(static_cast<float>(newBpm));

    if (playbackController)
        playbackController->setBPM(newBpm);
}

void AudioEngine::setSwingAmount(float swing)
{
    swingAmount.store(swing);

    if (playbackController)
        playbackController->setSwingAmount(swing);
}

void AudioEngine::setLoopLength(int steps)
{
    loopLength.store(steps);

    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }

    if (playbackController)
        playbackController->setLoopLength(steps);
}

void AudioEngine::setMasterVolume(float volume)
{
    masterVolume.store(volume);

    if (playbackController)
        playbackController->setMasterVolume(volume);
}

void AudioEngine::setReverbWetLevel(float wetLevel)
{
    reverbWetLevel.store(wetLevel);

    if (playbackController)
        playbackController->setReverbWetLevel(wetLevel);
}

void AudioEngine::resetTrackStates()
{
    for (int i = 0; i < numTracks; ++i)
    {
        trackLastStep[i] = -1;
        trackLastRatchet[i] = -1;
    }
}

uint64_t AudioEngine::getSamplePosition() const
{
    return samplePosition.load();
}

int AudioEngine::getSamplesPerStep() const
{
    return samplesPerStep.load();
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
        if (trackProvider->getSolo(trackIdx))
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
        const bool trackMuted = trackProvider->getMuted(trackIdx);
        const bool trackSolo = trackProvider->getSolo(trackIdx);

        // Skip track if muted, or if another track is soloed and this one isn't
        if (trackMuted || (anySolo && !trackSolo))
            continue;

        // Get track type
        const TrackType trackType = trackProvider->getTrackType(trackIdx);

        // Use pre-allocated buffer (assert to catch buffer overruns in debug)
        auto& tempBuffer = *trackBuffers[trackIdx];

        // Safety check: buffer should never exceed pre-allocated size
        jassert(tempBuffer.getNumSamples() >= bufferToFill.numSamples);

        tempBuffer.clear();

        // Render based on track type
        if (trackType == TrackType::Wavetable)
        {
            // Render wavetable synth directly (per-track synth instance)
            auto* wtSynth = trackProvider->getWavetableSynth(trackIdx);
            if (wtSynth)
            {
                wtSynth->processModulations();
                wtSynth->renderNextBlock(tempBuffer,
                    trackMidiBuffers[trackIdx], 0, bufferToFill.numSamples);
            }

            trackProvider->processAudioBlock(trackIdx, tempBuffer);
        }
        else
        {
            // Sampler mode
            trackProvider->getSynthesiser(trackIdx).renderNextBlock(tempBuffer,
                trackMidiBuffers[trackIdx], 0, bufferToFill.numSamples);

            // Apply modulation-affected filter processing (sampler only)
            if (isModulatorMode && trackProvider->getWavetableModulationEnabled(trackIdx))
            {
                float cutoffMod = wavetableEngine.getModulationValue(trackIdx, ModulationTarget::Filter_Cutoff);
                
                if (cutoffMod != 0.0f)
                {
                    float currentCutoff = trackProvider->getCutoff(trackIdx);
                    float modFactor = 1.0f + (cutoffMod * 2.0f);
                    float newCutoff = juce::jlimit(20.0f, 20000.0f, currentCutoff * modFactor);

                    // Aktualisiere Parameter sicher für Echtzeit ohne Allokation
                    modulationFilters[trackIdx]->setCutoffFrequency(newCutoff);
                    modulationFilters[trackIdx]->setResonance(0.5f); // Oder hier dynamische Resonanz reinholen

                    juce::dsp::AudioBlock<float> block(tempBuffer);
                    modulationFilters[trackIdx]->process(juce::dsp::ProcessContextReplacing<float>(block));
                }
            }

            trackProvider->processAudioBlock(trackIdx, tempBuffer);
        }

        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            bufferToFill.buffer->addFrom(channel, 0, tempBuffer, channel, 0, bufferToFill.numSamples, vol);
        }
    }

    // Process Wavetable Synth in Standalone mode
    if (wavetableEngine.getMode() == WavetableEngine::Mode::Standalone)
    {
        // Safety check: buffer should never exceed pre-allocated size
        jassert(wavetableBuffer->getNumSamples() >= bufferToFill.numSamples);

        wavetableBuffer->clear();
        wavetableMidiBuffer.clear();

        wavetableEngine.processBlock(*wavetableBuffer, wavetableMidiBuffer);

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

// === Timeline Mode ===

void AudioEngine::setEngineMode(EngineMode mode)
{
    engineMode.store(mode);

    if (mode == EngineMode::Timeline)
    {
        // Sync BPM to timeline
        timelineData->setBPM(bpm.load());
    }
}

TimelineData& AudioEngine::getTimelineData()
{
    return *timelineData;
}

const TimelineData& AudioEngine::getTimelineData() const
{
    return *timelineData;
}

TimelineTransport& AudioEngine::getTimelineTransport()
{
    return *timelineTransport;
}

TimelineRenderer& AudioEngine::getTimelineRenderer()
{
    return *timelineRenderer;
}

RecordingManager& AudioEngine::getRecordingManager()
{
    return *recordingManager;
}