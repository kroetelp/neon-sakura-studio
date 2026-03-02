#include "WavetableEngine.h"

WavetableEngine::WavetableEngine()
{
    // Initialize track modulation enabled flags
    for (auto& enabled : trackModulationEnabled)
        enabled = false;

    cachedModulationValues.fill(0.0f);

    // Initialize oscilloscope ring buffer
    for (int i = 0; i < scopeBufferSize; ++i)
    {
        scopeLeftBuffer[i].store(0.0f);
        scopeRightBuffer[i].store(0.0f);
    }
}

void WavetableEngine::prepareToPlay(int newSamplesPerBlock, double newSampleRate)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    synth.setCurrentPlaybackSampleRate(newSampleRate);

    // Prepare Chorus
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(newSamplesPerBlock);
    spec.numChannels = 2;

    chorus.prepare(spec);
    chorus.setMix(0.0f);
    chorus.setRate(1.0f);
    chorus.setDepth(0.25f);
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);

    // Prepare Delay
    juce::dsp::ProcessSpec delaySpec;
    delaySpec.sampleRate = newSampleRate;
    delaySpec.maximumBlockSize = static_cast<juce::uint32>(newSamplesPerBlock);
    delaySpec.numChannels = 2;
    delayLine.prepare(delaySpec);
    delayLine.reset();
    delayBuffer.setSize(2, static_cast<int>(newSampleRate * 2));  // 2 second buffer
    delayBuffer.clear();
    delayWritePos = 0;

    // Prepare Reverb - dsp::Reverb uses prepare() with ProcessSpec
    reverb.prepare(spec);
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 0.0f;
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
}

void WavetableEngine::releaseResources()
{
    // Synth cleanup happens automatically
    delayBuffer.clear();
}

void WavetableEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (currentMode.load() != Mode::Standalone)
        return;

    // Process MIDI events from lock-free queue first
    processMidiEventsFromQueue(midiMessages);

    // Process modulations
    synth.processModulations();

    // Render synth
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Apply master volume
    float vol = masterVolume.load();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.applyGain(channel, 0, buffer.getNumSamples(), vol);
    }

    // Process Master FX Chain
    processMasterFX(buffer);

    // === Write to oscilloscope ring buffer (lock-free) ===
    int numSamples = buffer.getNumSamples();
    int writePos = scopeWritePos.load();

    const float* leftData = buffer.getReadPointer(0);
    const float* rightData = buffer.getNumChannels() > 1
        ? buffer.getReadPointer(1)
        : leftData;

    for (int i = 0; i < numSamples; ++i)
    {
        scopeLeftBuffer[writePos].store(leftData[i]);
        scopeRightBuffer[writePos].store(rightData[i]);
        writePos = (writePos + 1) % scopeBufferSize;
    }
    scopeWritePos.store(writePos);

    // Cache modulation values for modulator mode
    for (int t = 0; t < static_cast<int>(ModulationTarget::Count); ++t)
    {
        cachedModulationValues[t] = synth.getModulationMatrix().getModulationValue(static_cast<ModulationTarget>(t));
    }
}

void WavetableEngine::processMasterFX(juce::AudioBuffer<float>& buffer)
{
    // Chain: Chorus -> Delay -> Reverb
    processChorus(buffer);
    processDelay(buffer);
    processReverb(buffer);
}

void WavetableEngine::processChorus(juce::AudioBuffer<float>& buffer)
{
    float mix = fxParams.chorusMix.load();
    if (mix < 0.001f)
        return;  // Bypass if mix is zero

    // Update chorus parameters
    chorus.setMix(mix);
    chorus.setRate(fxParams.chorusRate.load());
    chorus.setDepth(fxParams.chorusDepth.load());

    // Process through chorus
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chorus.process(context);
}

void WavetableEngine::processDelay(juce::AudioBuffer<float>& buffer)
{
    float mix = fxParams.delayMix.load();
    if (mix < 0.001f)
        return;  // Bypass if mix is zero

    float delayTimeSec = fxParams.delayTime.load();
    float feedback = fxParams.delayFeedback.load();

    int delaySamples = static_cast<int>(delayTimeSec * sampleRate);
    delaySamples = juce::jlimit(1, delayBuffer.getNumSamples() - 1, delaySamples);

    int numSamples = buffer.getNumSamples();
    int numChannels = juce::jmin(2, buffer.getNumChannels());

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Calculate read position
        int readPos = delayWritePos - delaySamples;
        if (readPos < 0)
            readPos += delayBuffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            // Read delayed sample
            float delayed = delayBuffer.getSample(ch, readPos);

            // Get input sample
            float input = buffer.getSample(ch, sample);

            // Mix dry/wet
            float output = input + delayed * mix;
            buffer.setSample(ch, sample, output);

            // Write to delay buffer with feedback
            float writeValue = input + delayed * feedback;
            delayBuffer.setSample(ch, delayWritePos, writeValue);
        }

        // Increment write position
        delayWritePos = (delayWritePos + 1) % delayBuffer.getNumSamples();
    }
}

void WavetableEngine::processReverb(juce::AudioBuffer<float>& buffer)
{
    float mix = fxParams.reverbMix.load();
    if (mix < 0.001f)
        return;  // Bypass if mix is zero

    // Update reverb parameters
    reverbParams.roomSize = fxParams.reverbSize.load();
    reverbParams.damping = fxParams.reverbDamping.load();
    reverbParams.wetLevel = mix;
    reverbParams.dryLevel = 1.0f - mix * 0.5f;  // Keep some dry signal
    reverb.setParameters(reverbParams);

    // Process reverb using dsp::ProcessContextReplacing
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}

float WavetableEngine::getModulationValue(int trackIndex, ModulationTarget target) const
{
    if (trackIndex < 0 || trackIndex >= 8 || !trackModulationEnabled[trackIndex].load())
        return 0.0f;

    int targetIndex = static_cast<int>(target);
    if (targetIndex >= 0 && targetIndex < static_cast<int>(ModulationTarget::Count))
        return cachedModulationValues[targetIndex];

    return 0.0f;
}

bool WavetableEngine::isModulationEnabled(int trackIndex) const
{
    if (trackIndex < 0 || trackIndex >= 8)
        return false;
    return trackModulationEnabled[trackIndex].load();
}

void WavetableEngine::setModulationEnabled(int trackIndex, bool enabled)
{
    if (trackIndex >= 0 && trackIndex < 8)
        trackModulationEnabled[trackIndex] = enabled;
}

void WavetableEngine::setBPM(float bpm)
{
    currentBPM = bpm;

    // Update LFO tempo sync
    for (int i = 0; i < WavetableSynth::numLFOs; ++i)
    {
        synth.getLFO(i).setBPM(bpm);
    }
}

void WavetableEngine::handleMidiEvent(const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        noteOn(message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        noteOff(message.getChannel(), message.getNoteNumber());
    }
    else if (message.isPitchWheel())
    {
        synth.setPitchBendValue((message.getPitchWheelValue() - 8192) / 8192.0f);
    }
    else if (message.isController())
    {
        if (message.getControllerNumber() == 1)  // Mod wheel
        {
            synth.setModWheelValue(message.getControllerValue() / 127.0f);
        }
    }
}

void WavetableEngine::noteOn(int channel, int midiNote, float velocity)
{
    synth.noteOn(channel, midiNote, velocity);
}

void WavetableEngine::noteOff(int channel, int midiNote)
{
    synth.noteOff(channel, midiNote, 1.0f, true);
}

void WavetableEngine::processMidiEventsFromQueue(juce::MidiBuffer& midiBuffer)
{
    if (!midiEventQueue)
        return;

    MidiEvent event;
    while (midiEventQueue->pop(event))
    {
        switch (event.type)
        {
            case MidiEvent::NoteOn:
                if (event.velocity > 0.0f)
                {
                    midiBuffer.addEvent(juce::MidiMessage::noteOn(
                        event.channel, event.noteNumber, event.velocity), 0);
                }
                else
                {
                    // Velocity 0 = note off
                    midiBuffer.addEvent(juce::MidiMessage::noteOff(
                        event.channel, event.noteNumber), 0);
                }
                break;

            case MidiEvent::NoteOff:
                midiBuffer.addEvent(juce::MidiMessage::noteOff(
                    event.channel, event.noteNumber), 0);
                break;

            case MidiEvent::Controller:
                midiBuffer.addEvent(juce::MidiMessage::controllerEvent(
                    event.channel, event.controllerNum, event.controllerVal), 0);
                break;

            case MidiEvent::PitchBend:
                midiBuffer.addEvent(juce::MidiMessage::pitchWheel(
                    event.channel, event.pitchBendVal + 8192), 0);
                break;

            case MidiEvent::Aftertouch:
                // Handle both channel pressure and polyphonic aftertouch
                {
                    int pressureValue = (event.controllerVal > 0)
                        ? event.controllerVal
                        : static_cast<int>(event.aftertouchVal * 127.0f);
                    float normalizedPressure = pressureValue / 127.0f;

                    // Check if this is polyphonic aftertouch (has note number)
                    if (event.noteNumber > 0 && event.noteNumber < 128)
                    {
                        // Polyphonic aftertouch - route directly to synth
                        synth.setPolyAftertouch(event.noteNumber, normalizedPressure);

                        // Create poly aftertouch MIDI message manually (raw bytes)
                        // Status byte: 0xA0 | channel, Data1: note, Data2: pressure
                        juce::MidiMessage polyMsg(0xA0 | (event.channel - 1),
                                                   event.noteNumber,
                                                   pressureValue);
                        midiBuffer.addEvent(polyMsg, 0);
                    }
                    else
                    {
                        // Channel pressure - set global aftertouch
                        synth.setAftertouchValue(normalizedPressure);
                        midiBuffer.addEvent(juce::MidiMessage::channelPressureChange(
                            event.channel, juce::jlimit(0, 127, pressureValue)), 0);
                    }
                }
                break;
        }
    }
}
