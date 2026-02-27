#include "WavetableSynth.h"

WavetableSynth::WavetableSynth()
{
    wavetableData = std::make_shared<WavetableData>();
    auto initialParams = std::make_shared<WavetableParams>();
    sharedParams.store(initialParams);

    for (int i = 0; i < maxVoices; ++i)
    {
        auto voice = std::make_unique<WavetableVoice>();
        voice->setWavetable(wavetableData);
        voice->setParams(&legacyParams);
        voice->setSharedParams(initialParams);
        voice->setModulationMatrix(&modulationMatrix);
        addVoice(voice.release());
    }

    addSound(new WavetableSound());
    setupModulationMatrix();
}

void WavetableSynth::setSharedParams(std::shared_ptr<WavetableParams> params)
{
    sharedParams.store(params);

    for (int i = 0; i < getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<WavetableVoice*>(getVoice(i)))
        {
            voice->setSharedParams(params);
        }
    }
}

void WavetableSynth::setCurrentPlaybackSampleRate(double sampleRate)
{
    // Wichtig: JUCE Basisklasse aufrufen!
    juce::Synthesiser::setCurrentPlaybackSampleRate(sampleRate);
    currentSampleRate = sampleRate;

    for (auto& lfo : lfos)
        lfo.setSampleRate(sampleRate);

    for (auto& env : envelopes)
        env.setSampleRate(sampleRate);
}

void WavetableSynth::setWavetable(std::shared_ptr<WavetableData> wavetable)
{
    wavetableData = wavetable;

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
    // Thread-safe: UI thread requests change, audio thread applies it
    // Clamp to valid range
    numVoices = juce::jlimit(1, maxVoices, numVoices);
    pendingVoiceCount.store(numVoices);
}

void WavetableSynth::applyPendingVoiceChanges()
{
    // Called from audio thread only - safely apply voice count changes
    int requestedVoices = pendingVoiceCount.exchange(-1);

    if (requestedVoices < 0)
        return;  // No pending change

    int currentVoices = getNumVoices();

    if (requestedVoices > currentVoices)
    {
        // Add voices
        for (int i = currentVoices; i < requestedVoices; ++i)
        {
            auto voice = std::make_unique<WavetableVoice>();
            voice->setWavetable(wavetableData);
            voice->setParams(&legacyParams);
            voice->setSharedParams(sharedParams.load());
            voice->setModulationMatrix(&modulationMatrix);
            voice->setCurrentPlaybackSampleRate(currentSampleRate);
            addVoice(voice.release());
        }
    }
    else if (requestedVoices < currentVoices)
    {
        // Remove voices (from the end)
        while (getNumVoices() > requestedVoices)
        {
            removeVoice(getNumVoices() - 1);
        }
    }
}

void WavetableSynth::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
    // Store global velocity for UI/fallback (non-voice context usage)
    lastGlobalVelocity.store(velocity);

    // JUCE voice allocator will call WavetableVoice::startNote()
    // with this velocity, where it gets stored per-voice
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
}

void WavetableSynth::setupModulationMatrix()
{
    modulationMatrix.setModulatorValueProvider([this](ModulationSource source, const ModulationContext& ctx) -> float
    {
        return getModulatorValue(source, ctx);
    });
}

void WavetableSynth::processModulations()
{
    // Apply pending voice count changes (thread-safe, audio thread only)
    applyPendingVoiceChanges();

    for (auto& lfo : lfos) lfo.process();
    for (auto& env : envelopes) env.process();
    modulationMatrix.process();
}

float WavetableSynth::getModulatorValue(ModulationSource source, const ModulationContext& ctx) const
{
    switch (source)
    {
        // Global LFOs
        case ModulationSource::LFO1: return lfos[0].getCurrentValue();
        case ModulationSource::LFO2: return lfos[1].getCurrentValue();
        case ModulationSource::LFO3: return lfos[2].getCurrentValue();
        case ModulationSource::LFO4: return lfos[3].getCurrentValue();

        // Global Envelopes
        case ModulationSource::Envelope1: return envelopes[0].getCurrentValue();
        case ModulationSource::Envelope2: return envelopes[1].getCurrentValue();
        case ModulationSource::Envelope3: return envelopes[2].getCurrentValue();

        // Per-voice sources - use context if available, fallback to global
        case ModulationSource::Velocity:
            // Use per-voice velocity from context, fallback to global
            return ctx.isValid() ? ctx.velocity : lastGlobalVelocity.load();

        case ModulationSource::Aftertouch:
            // Use per-voice aftertouch from context
            return ctx.aftertouch;

        // Global MIDI sources
        case ModulationSource::ModWheel: return modWheelValue;
        case ModulationSource::PitchBend: return pitchBendValue;

        // Macros
        case ModulationSource::Macro1: return macroValues[0];
        case ModulationSource::Macro2: return macroValues[1];
        case ModulationSource::Macro3: return macroValues[2];
        case ModulationSource::Macro4: return macroValues[3];

        default: return 0.0f;
    }
}

void WavetableSynth::setMacroValue(int macroIndex, float value)
{
    if (macroIndex >= 0 && macroIndex < 4)
        macroValues[macroIndex] = juce::jlimit(0.0f, 1.0f, value);
}

float WavetableSynth::getMacroValue(int macroIndex) const
{
    if (macroIndex >= 0 && macroIndex < 4) return macroValues[macroIndex];
    return 0.0f;
}