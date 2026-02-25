#include "WavetableSynth.h"

WavetableSynth::WavetableSynth()
{
    // Create default wavetable
    wavetableData = std::make_shared<WavetableData>();

    // Create shared params
    sharedParams = std::make_shared<WavetableParams>();

    // Add voices
    for (int i = 0; i < maxVoices; ++i)
    {
        auto voice = std::make_unique<WavetableVoice>();
        voice->setWavetable(wavetableData);
        voice->setParams(&legacyParams);
        voice->setSharedParams(sharedParams);
        voice->setModulationMatrix(&modulationMatrix);
        addVoice(voice.release());
    }

    // Add basic sound (always playable)
    addSound(new WavetableSound());

    // Setup modulation matrix value provider
    setupModulationMatrix();
}

void WavetableSynth::setSharedParams(std::shared_ptr<WavetableParams> params)
{
    sharedParams = params;

    // Update all voices with new shared params
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<WavetableVoice*>(getVoice(i)))
        {
            voice->setSharedParams(sharedParams);
        }
    }
}

void WavetableSynth::setSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;

    // Set sample rate on all voices
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<WavetableVoice*>(getVoice(i)))
        {
            // Voices get sample rate from Synthesiser base class
        }
    }

    // Set sample rate on LFOs and envelopes
    for (auto& lfo : lfos)
        lfo.setSampleRate(sampleRate);

    for (auto& env : envelopes)
        env.setSampleRate(sampleRate);
}

void WavetableSynth::setWavetable(std::shared_ptr<WavetableData> wavetable)
{
    wavetableData = wavetable;

    // Update all voices with new wavetable
    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<WavetableVoice*>(getVoice(i)))
        {
            voice->setWavetable(wavetableData);
        }
    }
}

void WavetableSynth::setNumVoices(int numVoices)
{
    int currentVoices = getNumVoices();

    if (numVoices > currentVoices)
    {
        // Add voices
        for (int i = currentVoices; i < numVoices; ++i)
        {
            auto voice = std::make_unique<WavetableVoice>();
            voice->setWavetable(wavetableData);
            voice->setParams(&legacyParams);
            voice->setSharedParams(sharedParams);
            voice->setModulationMatrix(&modulationMatrix);
            addVoice(voice.release());
        }
    }
    else if (numVoices < currentVoices)
    {
        // Remove voices
        while (getNumVoices() > numVoices)
        {
            removeVoice(getNumVoices() - 1);
        }
    }
}

void WavetableSynth::setupModulationMatrix()
{
    // Set up the value provider function
    modulationMatrix.setModulatorValueProvider([this](ModulationSource source) -> float
    {
        return getModulatorValue(source);
    });
}

void WavetableSynth::processModulations()
{
    // Process LFOs
    for (auto& lfo : lfos)
        lfo.process();

    // Process envelopes
    for (auto& env : envelopes)
        env.process();

    // Update modulation matrix
    modulationMatrix.process();
}

float WavetableSynth::getModulatorValue(ModulationSource source) const
{
    switch (source)
    {
        case ModulationSource::LFO1:
            return lfos[0].getCurrentValue();
        case ModulationSource::LFO2:
            return lfos[1].getCurrentValue();
        case ModulationSource::LFO3:
            return lfos[2].getCurrentValue();
        case ModulationSource::LFO4:
            return lfos[3].getCurrentValue();

        case ModulationSource::Envelope1:
            return envelopes[0].getCurrentValue();
        case ModulationSource::Envelope2:
            return envelopes[1].getCurrentValue();
        case ModulationSource::Envelope3:
            return envelopes[2].getCurrentValue();

        case ModulationSource::Velocity:
            // Would need to track last played velocity
            return 0.8f;

        case ModulationSource::ModWheel:
            return modWheelValue;
        case ModulationSource::PitchBend:
            return pitchBendValue;
        case ModulationSource::Aftertouch:
            return aftertouchValue;

        case ModulationSource::Macro1:
            return macroValues[0];
        case ModulationSource::Macro2:
            return macroValues[1];
        case ModulationSource::Macro3:
            return macroValues[2];
        case ModulationSource::Macro4:
            return macroValues[3];

        default:
            return 0.0f;
    }
}

void WavetableSynth::setMacroValue(int macroIndex, float value)
{
    if (macroIndex >= 0 && macroIndex < 4)
        macroValues[macroIndex] = juce::jlimit(0.0f, 1.0f, value);
}

float WavetableSynth::getMacroValue(int macroIndex) const
{
    if (macroIndex >= 0 && macroIndex < 4)
        return macroValues[macroIndex];
    return 0.0f;
}
