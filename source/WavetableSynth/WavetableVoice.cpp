#include "WavetableVoice.h"
#include <cmath>

WavetableVoice::WavetableVoice()
{
    auto defaultWavetable = std::make_shared<WavetableData>();
    for (auto& osc : oscillators)
    {
        osc.setWavetable(defaultWavetable);
    }
}

void WavetableVoice::setWavetable(std::shared_ptr<WavetableData> wavetable)
{
    for (auto& osc : oscillators)
    {
        osc.setWavetable(wavetable);
    }
}

void WavetableVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    currentMidiNote = midiNoteNumber;
    currentVelocity = velocity;
    isActive = true;

    for (auto& osc : oscillators) osc.resetPhase();
    subOscillator.resetPhase();

    // Wir laden den Pointer einmal sicher für diese Methode
    auto currentParams = sharedParams.load();

    if (currentParams)
    {
        ampEnvelope.noteOn(
            currentParams->getEnvAttack(),
            currentParams->getEnvDecay(),
            currentParams->getEnvSustain(),
            currentParams->getEnvRelease()
        );
    }
    else if (params)
    {
        ampEnvelope.noteOn(
            params->envAttack.load(),
            params->envDecay.load(),
            params->envSustain.load(),
            params->envRelease.load()
        );
    }
    else
    {
        ampEnvelope.noteOn(0.01f, 0.1f, 0.7f, 0.3f);
    }

    pitchWheelMoved(currentPitchWheelPosition);
}

void WavetableVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampEnvelope.noteOff();
    }
    else
    {
        ampEnvelope.stage = ADSR::Stage::Idle;
        ampEnvelope.value = 0.0f;
        isActive = false;
        clearCurrentNote();
    }
}

void WavetableVoice::pitchWheelMoved(int newValue)
{
    pitchBend = (newValue - 8192) / 8192.0f;
}

void WavetableVoice::controllerMoved(int controllerNumber, int newValue)
{
    (void)controllerNumber;
    (void)newValue;
}

float WavetableVoice::calculateFrequency(int midiNote, float pitchOffset) const
{
    float bentNote = midiNote + pitchBend * 2.0f + pitchOffset;
    return 440.0f * std::pow(2.0f, (bentNote - 69) / 12.0f);
}

void WavetableVoice::applyModulation(float& left, float& right)
{
    if (!modulationMatrix) return;
    (void)left;
    (void)right;
    // Modulation is now applied via createModulationContext() in renderNextBlock
}

ModulationContext WavetableVoice::createModulationContext() const
{
    ModulationContext ctx;
    ctx.velocity = currentVelocity;
    ctx.aftertouch = 0.0f;  // Per-voice aftertouch not yet implemented
    ctx.midiNote = currentMidiNote;
    return ctx;
}

void WavetableVoice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!isActive)
        return;

    // Einmaliges, sicheres Laden der Parameter am Anfang des Audio-Blocks!
    auto currentParams = sharedParams.load();

    if (!currentParams && !params)
        return;

    // Create per-voice modulation context for velocity-sensitive modulation
    ModulationContext modContext = createModulationContext();

    float* leftChannel = buffer.getWritePointer(0, startSample);
    float* rightChannel = buffer.getNumChannels() > 1
        ? buffer.getWritePointer(1, startSample)
        : nullptr;

    float masterLevel = currentParams ? currentParams->getMasterLevel() : params->masterLevel.load();

    // Get base filter parameters
    float cutoff = currentParams ? currentParams->getFilterCutoff() : params->filterCutoff.load();
    float resonance = currentParams ? currentParams->getFilterResonance() : params->filterResonance.load();
    float drive = currentParams ? currentParams->getFilterDrive() : params->filterDrive.load();
    int mode = currentParams ? currentParams->getFilterMode() : params->filterMode.load();

    // Apply velocity modulation to filter cutoff if modulation matrix is available
    if (modulationMatrix)
    {
        float cutoffMod = modulationMatrix->getModulationValue(ModulationTarget::Filter_Cutoff, modContext);
        if (std::abs(cutoffMod) > 0.001f)
        {
            // Apply modulation as a multiplier (semitones or frequency scaling)
            cutoff = juce::jlimit(20.0f, 20000.0f, cutoff * std::pow(2.0f, cutoffMod * 2.0f));
        }

        float resonanceMod = modulationMatrix->getModulationValue(ModulationTarget::Filter_Resonance, modContext);
        if (std::abs(resonanceMod) > 0.001f)
        {
            resonance = juce::jlimit(0.0f, 1.0f, resonance + resonanceMod * 0.5f);
        }

        float driveMod = modulationMatrix->getModulationValue(ModulationTarget::Filter_Drive, modContext);
        if (std::abs(driveMod) > 0.001f)
        {
            drive = juce::jlimit(0.0f, 10.0f, drive + driveMod * 2.0f);
        }
    }

    filter.setCutoff(cutoff);
    filter.setResonance(resonance);
    filter.setDrive(drive);
    filter.setMode(static_cast<WavetableFilter::Mode>(mode));

    for (int i = 0; i < numSamples; ++i)
    {
        float envValue = ampEnvelope.process();

        if (!ampEnvelope.isActive() && envValue < 0.0001f)
        {
            isActive = false;
            clearCurrentNote();
            break;
        }

        float left = 0.0f, right = 0.0f;

        for (int oscIndex = 0; oscIndex < 3; ++oscIndex)
        {
            float level = currentParams ? currentParams->getOscLevel(oscIndex) : params->oscLevels[oscIndex].load();
            if (level < 0.001f) continue;

            auto& osc = oscillators[oscIndex];

            float pitchOffset = currentParams ? currentParams->getOscPitchOffset(oscIndex) : params->oscPitchOffsets[oscIndex].load();
            float freq = calculateFrequency(currentMidiNote, pitchOffset);
            osc.setFrequency(freq);
            osc.setLevel(level);

            float morph = currentParams ? currentParams->getOscMorph(oscIndex) : params->oscMorphs[oscIndex].load();
            osc.setMorphPosition(morph);

            float detune = currentParams ? currentParams->getOscDetune(oscIndex) : params->oscDetunes[oscIndex].load();
            osc.setDetune(detune);

            int unison = currentParams ? currentParams->getOscUnisonCount(oscIndex) : params->oscUnisonCounts[oscIndex].load();
            osc.setUnisonCount(unison);

            float panSpread = currentParams ? currentParams->getOscPanSpread(oscIndex) : params->oscPanSpreads[oscIndex].load();
            osc.setPanSpread(panSpread);

            float pan = currentParams ? currentParams->getOscPan(oscIndex) : params->oscPans[oscIndex].load();
            osc.setPan(pan);

            float oscLeft = 0.0f, oscRight = 0.0f;
            osc.process(oscLeft, oscRight);

            left += oscLeft;
            right += oscRight;
        }

        float subLevel = currentParams ? currentParams->getSubLevel() : params->subLevel.load();
        if (subLevel > 0.001f)
        {
            subOscillator.setFrequency(calculateFrequency(currentMidiNote));
            subOscillator.setLevel(subLevel);

            int subOctave = currentParams ? currentParams->getSubOctave() : params->subOctave.load();
            subOscillator.setOctave(subOctave);

            int subWave = currentParams ? currentParams->getSubWaveform() : params->subWaveform.load();
            subOscillator.setWaveform(static_cast<SubOscillator::Waveform>(subWave));

            float subSample = subOscillator.process();
            left += subSample * 0.707f;
            right += subSample * 0.707f;
        }

        filter.process(left, right);

        float amp = envValue * currentVelocity * masterLevel;
        left *= amp;
        right *= amp;

        applyModulation(left, right);

        leftChannel[i] += left;
        if (rightChannel)
            rightChannel[i] += right;
    }
}

void WavetableVoice::ADSR::noteOn(float a, float d, float s, float r)
{
    attack = a; decay = d; sustain = s; release = r;
    stage = Stage::Attack;
    value = 0.0f;
    samplesInStage = 0;
    stageLengthSamples = juce::jmax(1, static_cast<int>(attack * sampleRate));
}

void WavetableVoice::ADSR::noteOff()
{
    stage = Stage::Release;
    releaseStartValue = value;
    samplesInStage = 0;
    stageLengthSamples = juce::jmax(1, static_cast<int>(release * sampleRate));
}

float WavetableVoice::ADSR::process()
{
    if (stage == Stage::Idle) return 0.0f;

    samplesInStage++;

    switch (stage)
    {
        case Stage::Attack:
            value = static_cast<float>(samplesInStage) / stageLengthSamples;
            if (samplesInStage >= stageLengthSamples)
            {
                stage = Stage::Decay;
                samplesInStage = 0;
                stageLengthSamples = juce::jmax(1, static_cast<int>(decay * sampleRate));
                value = 1.0f;
            }
            break;

        case Stage::Decay:
            value = 1.0f - (static_cast<float>(samplesInStage) / stageLengthSamples) * (1.0f - sustain);
            if (samplesInStage >= stageLengthSamples)
            {
                stage = Stage::Sustain;
                value = sustain;
            }
            break;

        case Stage::Sustain:
            value = sustain;
            break;

        case Stage::Release:
            value = releaseStartValue * (1.0f - static_cast<float>(samplesInStage) / stageLengthSamples);
            if (samplesInStage >= stageLengthSamples || value < 0.0001f)
            {
                stage = Stage::Idle;
                value = 0.0f;
            }
            break;
    }
    return value;
}