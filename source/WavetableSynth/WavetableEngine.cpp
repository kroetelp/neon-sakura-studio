#include "WavetableEngine.h"

WavetableEngine::WavetableEngine()
{
    // Initialize track modulation enabled flags
    for (auto& enabled : trackModulationEnabled)
        enabled = false;

    cachedModulationValues.fill(0.0f);
}

void WavetableEngine::prepareToPlay(int samplesPerBlock, double newSampleRate)
{
    sampleRate = newSampleRate;
    synth.setCurrentPlaybackSampleRate(newSampleRate);
}

void WavetableEngine::releaseResources()
{
    // Synth cleanup happens automatically
}

void WavetableEngine::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (currentMode.load() != Mode::Standalone)
        return;

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

    // Cache modulation values for modulator mode
    for (int t = 0; t < static_cast<int>(ModulationTarget::Count); ++t)
    {
        cachedModulationValues[t] = synth.getModulationMatrix().getModulationValue(static_cast<ModulationTarget>(t));
    }
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
        noteOn(message.getNoteNumber(), message.getFloatVelocity());
    }
    else if (message.isNoteOff())
    {
        noteOff(message.getNoteNumber());
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

void WavetableEngine::noteOn(int midiNote, float velocity)
{
    int channel = 1;  // Channel 1 for now
    synth.noteOn(channel, midiNote, velocity);
}

void WavetableEngine::noteOff(int midiNote)
{
    int channel = 1;
    synth.noteOff(channel, midiNote, 1.0f, true);
}
