#include "WavetableVoice.h"
#include <cmath>

WavetableVoice::WavetableVoice()
{
    // Initialize oscillators with default wavetable
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

    // Log note start (throttled to avoid flooding)
    static int lastLogTime = 0;
    int currentTime = juce::Time::getMillisecondCounter();
    if (currentTime - lastLogTime > 500)  // Log every 500ms max
    {
        juce::Logger::writeToLog("WavetableVoice::startNote - Note: " + juce::String(midiNoteNumber) + " Velocity: " + juce::String(velocity) + " Params: " + (params ? "YES" : "NO") + " SharedParams: " + (sharedParams ? "YES" : "NO"));
        lastLogTime = currentTime;
    }
    isActive = true;

    // Reset oscillators
    for (auto& osc : oscillators)
    {
        osc.resetPhase();
    }
    subOscillator.resetPhase();

    // Start envelope - use shared params if available
    if (sharedParams)
    {
        ampEnvelope.noteOn(
            sharedParams->getEnvAttack(),
            sharedParams->getEnvDecay(),
            sharedParams->getEnvSustain(),
            sharedParams->getEnvRelease()
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

    // Update pitch wheel
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
    // Convert 0-16384 to -1 to 1
    pitchBend = (newValue - 8192) / 8192.0f;
}

void WavetableVoice::controllerMoved(int controllerNumber, int newValue)
{
    // Handle CC messages if needed
    (void)controllerNumber;
    (void)newValue;
}

float WavetableVoice::calculateFrequency(int midiNote, float pitchOffset) const
{
    // Apply pitch bend (±2 semitones) and pitch offset
    float bentNote = midiNote + pitchBend * 2.0f + pitchOffset;
    return 440.0f * std::pow(2.0f, (bentNote - 69) / 12.0f);
}

void WavetableVoice::applyModulation(float& left, float& right)
{
    if (!modulationMatrix)
        return;

    // Apply any modulation to the sample (future: filter modulation, etc.)
    (void)left;
    (void)right;
}

void WavetableVoice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!isActive)
        return;

    // Use shared params if available, otherwise fall back to legacy params
    if (!sharedParams && !params)
        return;

    float* leftChannel = buffer.getWritePointer(0, startSample);
    float* rightChannel = buffer.getNumChannels() > 1
        ? buffer.getWritePointer(1, startSample)
        : nullptr;

    // Get parameters - prefer shared params
    float masterLevel = sharedParams ? sharedParams->getMasterLevel() : params->masterLevel.load();

    // Set filter parameters
    float cutoff = sharedParams ? sharedParams->getFilterCutoff() : params->filterCutoff.load();
    float resonance = sharedParams ? sharedParams->getFilterResonance() : params->filterResonance.load();
    float drive = sharedParams ? sharedParams->getFilterDrive() : params->filterDrive.load();
    int mode = sharedParams ? sharedParams->getFilterMode() : params->filterMode.load();

    filter.setCutoff(cutoff);
    filter.setResonance(resonance);
    filter.setDrive(drive);
    filter.setMode(static_cast<WavetableFilter::Mode>(mode));

    for (int i = 0; i < numSamples; ++i)
    {
        // Process envelope
        float envValue = ampEnvelope.process();

        if (!ampEnvelope.isActive() && envValue < 0.0001f)
        {
            isActive = false;
            clearCurrentNote();
            break;
        }

        // Mix oscillators
        float left = 0.0f, right = 0.0f;

        for (int oscIndex = 0; oscIndex < 3; ++oscIndex)
        {
            float level = sharedParams ? sharedParams->getOscLevel(oscIndex) : params->oscLevels[oscIndex].load();
            if (level < 0.001f)
                continue;

            auto& osc = oscillators[oscIndex];

            // Set oscillator parameters
            float pitchOffset = sharedParams ? sharedParams->getOscPitchOffset(oscIndex) : params->oscPitchOffsets[oscIndex].load();
            float freq = calculateFrequency(currentMidiNote, pitchOffset);
            osc.setFrequency(freq);
            osc.setLevel(level);

            float morph = sharedParams ? sharedParams->getOscMorph(oscIndex) : params->oscMorphs[oscIndex].load();
            osc.setMorphPosition(morph);

            float detune = sharedParams ? sharedParams->getOscDetune(oscIndex) : params->oscDetunes[oscIndex].load();
            osc.setDetune(detune);

            int unison = sharedParams ? sharedParams->getOscUnisonCount(oscIndex) : params->oscUnisonCounts[oscIndex].load();
            osc.setUnisonCount(unison);

            float panSpread = sharedParams ? sharedParams->getOscPanSpread(oscIndex) : params->oscPanSpreads[oscIndex].load();
            osc.setPanSpread(panSpread);

            float pan = sharedParams ? sharedParams->getOscPan(oscIndex) : params->oscPans[oscIndex].load();
            osc.setPan(pan);

            // Process oscillator
            float oscLeft = 0.0f, oscRight = 0.0f;
            osc.process(oscLeft, oscRight);

            left += oscLeft;
            right += oscRight;
        }

        // Process sub oscillator
        float subLevel = sharedParams ? sharedParams->getSubLevel() : params->subLevel.load();
        if (subLevel > 0.001f)
        {
            subOscillator.setFrequency(calculateFrequency(currentMidiNote));
            subOscillator.setLevel(subLevel);

            int subOctave = sharedParams ? sharedParams->getSubOctave() : params->subOctave.load();
            subOscillator.setOctave(subOctave);

            int subWave = sharedParams ? sharedParams->getSubWaveform() : params->subWaveform.load();
            subOscillator.setWaveform(static_cast<SubOscillator::Waveform>(subWave));

            float subSample = subOscillator.process();
            left += subSample * 0.707f;  // -3dB for mono
            right += subSample * 0.707f;
        }

        // Apply filter
        filter.process(left, right);

        // Apply envelope and velocity
        float amp = envValue * currentVelocity * masterLevel;
        left *= amp;
        right *= amp;

        // Apply modulation
        applyModulation(left, right);

        // Write to buffer
        leftChannel[i] += left;
        if (rightChannel)
            rightChannel[i] += right;
    }
}

// ADSR implementation
void WavetableVoice::ADSR::noteOn(float a, float d, float s, float r)
{
    attack = a;
    decay = d;
    sustain = s;
    release = r;

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
    if (stage == Stage::Idle)
        return 0.0f;

    samplesInStage++;

    switch (stage)
    {
        case Stage::Attack:
            {
                float progress = static_cast<float>(samplesInStage) / stageLengthSamples;
                value = progress;
                if (samplesInStage >= stageLengthSamples)
                {
                    stage = Stage::Decay;
                    samplesInStage = 0;
                    stageLengthSamples = juce::jmax(1, static_cast<int>(decay * sampleRate));
                    value = 1.0f;
                }
            }
            break;

        case Stage::Decay:
            {
                float progress = static_cast<float>(samplesInStage) / stageLengthSamples;
                value = 1.0f - progress * (1.0f - sustain);
                if (samplesInStage >= stageLengthSamples)
                {
                    stage = Stage::Sustain;
                    value = sustain;
                }
            }
            break;

        case Stage::Sustain:
            value = sustain;
            break;

        case Stage::Release:
            {
                float progress = static_cast<float>(samplesInStage) / stageLengthSamples;
                value = releaseStartValue * (1.0f - progress);
                if (samplesInStage >= stageLengthSamples || value < 0.0001f)
                {
                    stage = Stage::Idle;
                    value = 0.0f;
                }
            }
            break;

        default:
            break;
    }

    return value;
}
