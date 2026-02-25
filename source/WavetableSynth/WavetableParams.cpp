#include "WavetableParams.h"

WavetableParams::WavetableParams()
{
    resetToDefaults();
}

void WavetableParams::addListener(Listener* listener)
{
    listeners.add(listener);
}

void WavetableParams::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void WavetableParams::notifyChange()
{
    listeners.call([](Listener& l) { l.onParamsChanged(); });
}

void WavetableParams::notifyOscChange(int oscIndex, int paramId)
{
    listeners.call([oscIndex, paramId](Listener& l) { l.onOscParamChanged(oscIndex, paramId); });
}

void WavetableParams::notifyFilterChange()
{
    listeners.call([](Listener& l) { l.onFilterParamChanged(); });
}

void WavetableParams::notifyEnvelopeChange()
{
    listeners.call([](Listener& l) { l.onEnvelopeParamChanged(); });
}

// ============================================================
// Oscillator Parameters
// ============================================================

float WavetableParams::getOscLevel(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscLevels[oscIndex].load();
    return 0.0f;
}

float WavetableParams::getOscMorph(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscMorphs[oscIndex].load();
    return 0.0f;
}

float WavetableParams::getOscDetune(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscDetunes[oscIndex].load();
    return 0.0f;
}

int WavetableParams::getOscUnisonCount(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscUnisonCounts[oscIndex].load();
    return 1;
}

float WavetableParams::getOscPanSpread(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscPanSpreads[oscIndex].load();
    return 0.0f;
}

float WavetableParams::getOscPan(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscPans[oscIndex].load();
    return 0.0f;
}

float WavetableParams::getOscPitchOffset(int oscIndex) const
{
    if (oscIndex >= 0 && oscIndex < 3)
        return oscPitchOffsets[oscIndex].load();
    return 0.0f;
}

void WavetableParams::setOscLevel(int oscIndex, float value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscLevels[oscIndex].store(juce::jlimit(0.0f, 1.0f, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::Level));
    }
}

void WavetableParams::setOscMorph(int oscIndex, float value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscMorphs[oscIndex].store(juce::jlimit(0.0f, 1.0f, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::Morph));
    }
}

void WavetableParams::setOscDetune(int oscIndex, float value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscDetunes[oscIndex].store(juce::jlimit(0.0f, 100.0f, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::Detune));
    }
}

void WavetableParams::setOscUnisonCount(int oscIndex, int value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscUnisonCounts[oscIndex].store(juce::jlimit(1, 8, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::UnisonCount));
    }
}

void WavetableParams::setOscPanSpread(int oscIndex, float value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscPanSpreads[oscIndex].store(juce::jlimit(0.0f, 1.0f, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::PanSpread));
    }
}

void WavetableParams::setOscPan(int oscIndex, float value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscPans[oscIndex].store(juce::jlimit(-1.0f, 1.0f, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::Pan));
    }
}

void WavetableParams::setOscPitchOffset(int oscIndex, float value)
{
    if (oscIndex >= 0 && oscIndex < 3)
    {
        oscPitchOffsets[oscIndex].store(juce::jlimit(-24.0f, 24.0f, value));
        notifyOscChange(oscIndex, static_cast<int>(OscParam::PitchOffset));
    }
}

// ============================================================
// Sub Oscillator Parameters
// ============================================================

float WavetableParams::getSubLevel() const
{
    return subLevel.load();
}

int WavetableParams::getSubOctave() const
{
    return subOctave.load();
}

int WavetableParams::getSubWaveform() const
{
    return subWaveform.load();
}

void WavetableParams::setSubLevel(float value)
{
    subLevel.store(juce::jlimit(0.0f, 1.0f, value));
    notifyChange();
}

void WavetableParams::setSubOctave(int value)
{
    subOctave.store(juce::jlimit(-2, 2, value));
    notifyChange();
}

void WavetableParams::setSubWaveform(int value)
{
    subWaveform.store(juce::jlimit(0, 3, value));
    notifyChange();
}

// ============================================================
// Filter Parameters
// ============================================================

float WavetableParams::getFilterCutoff() const
{
    return filterCutoff.load();
}

float WavetableParams::getFilterResonance() const
{
    return filterResonance.load();
}

float WavetableParams::getFilterDrive() const
{
    return filterDrive.load();
}

int WavetableParams::getFilterMode() const
{
    return filterMode.load();
}

void WavetableParams::setFilterCutoff(float value)
{
    filterCutoff.store(juce::jlimit(20.0f, 20000.0f, value));
    notifyFilterChange();
}

void WavetableParams::setFilterResonance(float value)
{
    filterResonance.store(juce::jlimit(0.0f, 1.0f, value));
    notifyFilterChange();
}

void WavetableParams::setFilterDrive(float value)
{
    filterDrive.store(juce::jlimit(0.0f, 1.0f, value));
    notifyFilterChange();
}

void WavetableParams::setFilterMode(int value)
{
    filterMode.store(juce::jlimit(0, 3, value));
    notifyFilterChange();
}

// ============================================================
// Envelope Parameters
// ============================================================

float WavetableParams::getEnvAttack() const
{
    return envAttack.load();
}

float WavetableParams::getEnvDecay() const
{
    return envDecay.load();
}

float WavetableParams::getEnvSustain() const
{
    return envSustain.load();
}

float WavetableParams::getEnvRelease() const
{
    return envRelease.load();
}

void WavetableParams::setEnvAttack(float value)
{
    envAttack.store(juce::jlimit(0.001f, 10.0f, value));
    notifyEnvelopeChange();
}

void WavetableParams::setEnvDecay(float value)
{
    envDecay.store(juce::jlimit(0.001f, 10.0f, value));
    notifyEnvelopeChange();
}

void WavetableParams::setEnvSustain(float value)
{
    envSustain.store(juce::jlimit(0.0f, 1.0f, value));
    notifyEnvelopeChange();
}

void WavetableParams::setEnvRelease(float value)
{
    envRelease.store(juce::jlimit(0.001f, 10.0f, value));
    notifyEnvelopeChange();
}

// ============================================================
// Master Parameters
// ============================================================

float WavetableParams::getMasterLevel() const
{
    return masterLevel.load();
}

void WavetableParams::setMasterLevel(float value)
{
    masterLevel.store(juce::jlimit(0.0f, 1.0f, value));
    notifyChange();
}

// ============================================================
// Wavetable Selection
// ============================================================

int WavetableParams::getWavetableIndex() const
{
    return wavetableIndex.load();
}

void WavetableParams::setWavetableIndex(int index)
{
    wavetableIndex.store(juce::jmax(0, index));
    notifyChange();
}

// ============================================================
// Preset Support
// ============================================================

void WavetableParams::copyFrom(const WavetableParams& other)
{
    for (int i = 0; i < 3; ++i)
    {
        oscLevels[i].store(other.oscLevels[i].load());
        oscMorphs[i].store(other.oscMorphs[i].load());
        oscDetunes[i].store(other.oscDetunes[i].load());
        oscUnisonCounts[i].store(other.oscUnisonCounts[i].load());
        oscPanSpreads[i].store(other.oscPanSpreads[i].load());
        oscPans[i].store(other.oscPans[i].load());
        oscPitchOffsets[i].store(other.oscPitchOffsets[i].load());
    }

    subLevel.store(other.subLevel.load());
    subOctave.store(other.subOctave.load());
    subWaveform.store(other.subWaveform.load());

    filterCutoff.store(other.filterCutoff.load());
    filterResonance.store(other.filterResonance.load());
    filterDrive.store(other.filterDrive.load());
    filterMode.store(other.filterMode.load());

    envAttack.store(other.envAttack.load());
    envDecay.store(other.envDecay.load());
    envSustain.store(other.envSustain.load());
    envRelease.store(other.envRelease.load());

    masterLevel.store(other.masterLevel.load());
    wavetableIndex.store(other.wavetableIndex.load());

    notifyChange();
}

void WavetableParams::resetToDefaults()
{
    // OSC 1 enabled, others off by default
    for (int i = 0; i < 3; ++i)
    {
        oscLevels[i].store(i == 0 ? 1.0f : 0.0f);
        oscMorphs[i].store(0.0f);
        oscDetunes[i].store(0.0f);
        oscUnisonCounts[i].store(1);
        oscPanSpreads[i].store(0.0f);
        oscPans[i].store(0.0f);
        oscPitchOffsets[i].store(0.0f);
    }

    subLevel.store(0.0f);
    subOctave.store(1);
    subWaveform.store(0);

    filterCutoff.store(1000.0f);
    filterResonance.store(0.0f);
    filterDrive.store(0.0f);
    filterMode.store(0);

    envAttack.store(0.01f);
    envDecay.store(0.1f);
    envSustain.store(0.7f);
    envRelease.store(0.3f);

    masterLevel.store(1.0f);
    wavetableIndex.store(0);
}
