#include "AudioEngine.h"
#include "PlaybackController.h"
#include "TrackType.h"
#include "TrackModel.h"
#include "Modulation/ModulationSource.h"
#include "WavetableSynth/WavetableSynth.h"
#include "Timeline/TimelineData.h"
#include "Timeline/TimelineTransport.h"
#include "Timeline/TimelineRenderer.h"
#include "Timeline/TimelinePlayHead.h"
#include "Timeline/RecordingManager.h"
#include "AudioRouting/AudioRoutingGraph.h"
#include "VSTHost/VSTPluginManager.h"
#include "VSTHost/PluginLoadingCoordinator.h"
#include "AudioRouting/CPUProfiler.h"

AudioEngine::AudioEngine(ITrackDataProvider* provider)
    : trackProvider(provider)
    , playbackController(nullptr)
{
    // Initialize Timeline components
    timelineData = std::make_unique<TimelineData>();
    timelineTransport = std::make_unique<TimelineTransport>(*timelineData);
    timelineRenderer = std::make_unique<TimelineRenderer>(*timelineData, *timelineTransport);
    recordingManager = std::make_unique<RecordingManager>(*timelineData);
    timelinePlayHead = std::make_unique<TimelinePlayHead>(*timelineData, *timelineTransport);
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
    timelinePlayHead = std::make_unique<TimelinePlayHead>(*timelineData, *timelineTransport);


    // Initialize Audio Routing Graph for VST hosting
    audioRoutingGraph = std::make_unique<AudioRoutingGraph>();
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

        // Pre-allocate MIDI buffers to prevent heap allocation in audio thread
        // 2048 events should be more than enough for any single buffer
        trackMidiBuffers[i].ensureSize(2048);
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

    // Update PlayHead sample rate for accurate time-in-samples calculation
    if (timelinePlayHead)
    {
        timelinePlayHead->setSampleRate(newSampleRate);
    }

    // Prepare Audio Routing Graph for VST hosting
    if (audioRoutingGraph)
    {
        audioRoutingGraph->initialize(trackProvider, &wavetableEngine);
        audioRoutingGraph->prepareToPlay(newSampleRate, safeBufferSize);

        // Connect PlayHead to AudioRoutingGraph so plugins can sync with tempo/position
        if (timelinePlayHead)
        {
            audioRoutingGraph->setPlayHead(timelinePlayHead.get());
        }
    }

    // Phase 6.1: Prepare Plugin Loading Coordinator
    if (pluginLoadingCoordinator)
    {
        pluginLoadingCoordinator->prepareToPlay(newSampleRate, safeBufferSize);
    }

    // Phase 6.3: Prepare CPU Profiler
    if (cpuProfiler)
    {
        cpuProfiler->initialize();
        cpuProfiler->setSampleRate(newSampleRate);
        cpuProfiler->setBlockSize(safeBufferSize);
    }
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    // === Phase 6.3: CPU Profiling - Start Global Measure ===
    if (cpuProfiler)
    {
        cpuProfiler->startMeasure(ProfilingComponent::GlobalEngine);
    }

    // === Unified Timeline Architecture ===
    // Timeline is always active and handles:
    // - Audio/MIDI clips on tracks
    // - VST plugins via AudioRoutingGraph
    // - Step sequencer patterns as track data

    const bool localIsPlaying = playing.load();
    const float vol = masterVolume.load();

    // Clear pre-allocated MIDI buffers
    for (int i = 0; i < numTracks; ++i)
        trackMidiBuffers[i].clear();
    wavetableMidiBuffer.clear();

    // === Step 1: Generate MIDI from Step Sequencer Patterns ===
    // This will be moved to StepSequencerClip in Phase 2.2
    if (localIsPlaying)
    {
        generateStepSequencerMidi(bufferToFill.numSamples);
    }

    // === Step 2: Render Timeline Clips ===
    // Process timeline clips (audio/MIDI) if transport is playing
    if (localIsPlaying && timelineTransport->isPlaying())
    {
        timelineRenderer->processBlock(*bufferToFill.buffer, wavetableMidiBuffer);
    }

    // === Step 3: Render and Mix Tracks ===
    // Mix step sequencer tracks with timeline content
    renderAndMixTracks(bufferToFill, trackMidiBuffers);

    // === Step 4: Process VST Plugin Graph ===
    if (audioRoutingGraph && audioRoutingGraph->isInitialized())
    {
        juce::MidiBuffer emptyMidi;
        audioRoutingGraph->processBlock(*bufferToFill.buffer, emptyMidi);
    }

    // === Phase 6.1: Process Pending Plugin Loads (Lock-Free) ===
    // This safely transfers plugins from background thread to audio thread
    if (pluginLoadingCoordinator)
    {
        pluginLoadingCoordinator->processPendingLoads();
    }

    // === Step 5: Apply Master Effects ===
    applyMasterReverb(*bufferToFill.buffer);

    // === Step 6: Update Playhead ===
    if (localIsPlaying && timelineData != nullptr)
    {
        const double currentBpm = timelineData->getBPM();
        const double beatsPerSecond = currentBpm / 60.0;
        const double samplesPerBeatVal = currentSampleRate / beatsPerSecond;
        const double currentBeat = static_cast<double>(samplePosition.load()) / samplesPerBeatVal;
        timelineData->playheadBeat.store(currentBeat);

        // Also update TrackManager for automation access
        if (trackProvider)
        {
            trackProvider->setPlayheadBeat(currentBeat);
        }
    }

    // === Phase 6.3: CPU Profiling - End Global Measure ===
    if (cpuProfiler)
    {
        cpuProfiler->endMeasure(ProfilingComponent::GlobalEngine);
    }
}

void AudioEngine::generateStepSequencerMidi(int numSamples)
{
    // Step Sequencer Pattern Generation (will become StepSequencerClip in Phase 2.2)
    const int localSamplesPerStep = samplesPerStep.load();
    const float swingVal = swingAmount.load();

    if (localSamplesPerStep <= 0)
        return;

    const int swingOffsetSamples = static_cast<int>(localSamplesPerStep * swingVal);

    const uint64_t startSamplePos = samplePosition.load();
    const uint64_t endSamplePos = startSamplePos + numSamples;

    // Process each track with its OWN loop length for polyrhythms
    for (int trackIdx = 0; trackIdx < numTracks; ++trackIdx)
    {
        // PER-TRACK loop length for polyrhythms
        const int trackLoopLen = trackProvider->getTrackLoopLength(trackIdx);

        // Iterate through steps in this buffer
        for (uint64_t samplePos = startSamplePos; samplePos < endSamplePos; )
        {
            // Calculate step based on THIS track's loop length
            const int currentStep = static_cast<int>((samplePos / localSamplesPerStep) % trackLoopLen);
            const int nextStepBoundary = static_cast<int>((static_cast<int>(samplePos / localSamplesPerStep) + 1) * localSamplesPerStep);
            const int samplesUntilNextStep = static_cast<int>(juce::jmin(static_cast<uint64_t>(nextStepBoundary) - samplePos, endSamplePos - samplePos));

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

                const int sampleIndexInBuffer = static_cast<int>(samplePos - startSamplePos);

                // Apply swing offset for odd steps
                int eventIndex = sampleIndexInBuffer;
                if (currentStep % 2 != 0 && swingVal > 0.0f)
                {
                    eventIndex = juce::jmin(sampleIndexInBuffer + swingOffsetSamples, numSamples - 1);
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
                        const int noteOffIndex = juce::jmin(eventIndex + noteOffDelay, numSamples - 1);

                        juce::MidiMessage noteOffMessage = juce::MidiMessage::noteOff(1, midiNote);
                        trackMidiBuffers[trackIdx].addEvent(noteOffMessage, noteOffIndex);
                    }

                    trackLastStep[trackIdx] = currentStep;
                }

                // Handle ratchet (speed) modifier
                if (stepState.modifierType == '*' && localSamplesPerStep > 0)
                {
                    const int samplesPerRatchet = localSamplesPerStep / stepState.modifierValue;
                    const int currentRatchet = static_cast<int>((samplePos % localSamplesPerStep) / samplesPerRatchet);

                    if (currentRatchet != trackLastRatchet[trackIdx] && currentRatchet > 0)
                    {
                        const int ratchetSampleIndex = static_cast<int>(samplePos - startSamplePos);
                        int ratchetEventIndex = ratchetSampleIndex;
                        if (currentStep % 2 != 0 && swingVal > 0.0f)
                        {
                            ratchetEventIndex = juce::jmin(ratchetSampleIndex + swingOffsetSamples, numSamples - 1);
                        }

                        juce::MidiMessage midiMessage = juce::MidiMessage::noteOn(1, midiNote, velocity);
                        trackMidiBuffers[trackIdx].addEvent(midiMessage, ratchetEventIndex);

                        const int ratchetGateTime = juce::jmax(1, samplesPerRatchet / 2);
                        const int ratchetNoteOffIndex = juce::jmin(ratchetEventIndex + ratchetGateTime, numSamples - 1);
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

    // Phase 6.1: Release plugin loading coordinator resources
    if (pluginLoadingCoordinator)
    {
        pluginLoadingCoordinator->releaseResources();
    }
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

    // Reset timeline playhead
    if (timelineData != nullptr)
    {
        timelineData->playheadBeat.store(0.0);
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

    // Reset timeline playhead
    if (timelineData != nullptr)
    {
        timelineData->playheadBeat.store(0.0);
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

    // Sync with Timeline
    if (timelineData)
        timelineData->setBPM(newBpm);

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

        // Use pre-allocated buffer with safety check
        auto& tempBuffer = *trackBuffers[trackIdx];

        // Safety check: if buffer is too small, skip this track (prevents crash)
        if (tempBuffer.getNumSamples() < bufferToFill.numSamples)
        {
            jassertfalse;  // This should never happen - buffer was pre-allocated
            continue;
        }

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

        // Calculate peak level for audio meter (lock-free write to atomic)
        float currentPeak = tempBuffer.getMagnitude(0, bufferToFill.numSamples);
        if (tempBuffer.getNumChannels() > 1) {
            currentPeak = juce::jmax(currentPeak, tempBuffer.getMagnitude(1, bufferToFill.numSamples));
        }
        trackLevels[trackIdx].store(currentPeak, std::memory_order_relaxed);

        for (int channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
        {
            bufferToFill.buffer->addFrom(channel, 0, tempBuffer, channel, 0, bufferToFill.numSamples, vol);
        }
    }

    // Process Wavetable Synth in Standalone mode
    if (wavetableEngine.getMode() == WavetableEngine::Mode::Standalone)
    {
        // Safety check: if buffer is too small, skip processing (prevents crash)
        if (wavetableBuffer->getNumSamples() < bufferToFill.numSamples)
        {
            jassertfalse;  // This should never happen - buffer was pre-allocated
            return;
        }

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

// === Timeline (always active) ===

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

TimelinePlayHead& AudioEngine::getTimelinePlayHead()
{
    return *timelinePlayHead;
}

const TimelinePlayHead& AudioEngine::getTimelinePlayHead() const
{
    return *timelinePlayHead;
}

void AudioEngine::setVSTPluginManager(VSTPluginManager* manager)
{
    vstPluginManager = manager;
}

void AudioEngine::setPluginLoadingCoordinator(PluginLoadingCoordinator* coordinator)
{
    // Create plugin loading coordinator if provided
    if (coordinator && !pluginLoadingCoordinator)
    {
        pluginLoadingCoordinator = std::unique_ptr<PluginLoadingCoordinator>(coordinator);
        pluginLoadingCoordinator->initialize();
    }
}

void AudioEngine::setCPUProfiler(CPUProfiler* profiler)
{
    if (profiler && !cpuProfiler)
    {
        cpuProfiler = std::unique_ptr<CPUProfiler>(profiler);
    }
}