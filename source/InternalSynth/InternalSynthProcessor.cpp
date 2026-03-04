#include "InternalSynthProcessor.h"
#include "InternalSynthEditor.h"
#include "../WavetableSynth/WavetableEngine.h"
#include "../WavetableSynth/WavetableParams.h"

//==============================================================================
// Parameter IDs (static constants)
namespace ParamIDs
{
    // Master
    static const juce::String masterVolume = "masterVolume";

    // OSC 1
    static const juce::String osc1Level = "osc1Level";
    static const juce::String osc1Morph = "osc1Morph";
    static const juce::String osc1Detune = "osc1Detune";
    static const juce::String osc1Pan = "osc1Pan";
    static const juce::String osc1Unison = "osc1Unison";
    static const juce::String osc1PanSpread = "osc1PanSpread";

    // OSC 2
    static const juce::String osc2Level = "osc2Level";
    static const juce::String osc2Morph = "osc2Morph";
    static const juce::String osc2Detune = "osc2Detune";
    static const juce::String osc2Pan = "osc2Pan";
    static const juce::String osc2Unison = "osc2Unison";
    static const juce::String osc2PanSpread = "osc2PanSpread";

    // OSC 3
    static const juce::String osc3Level = "osc3Level";
    static const juce::String osc3Morph = "osc3Morph";
    static const juce::String osc3Detune = "osc3Detune";
    static const juce::String osc3Pan = "osc3Pan";
    static const juce::String osc3Unison = "osc3Unison";
    static const juce::String osc3PanSpread = "osc3PanSpread";

    // Sub Oscillator
    static const juce::String subLevel = "subLevel";
    static const juce::String subOctave = "subOctave";
    static const juce::String subWaveform = "subWaveform";

    // Filter
    static const juce::String filterCutoff = "filterCutoff";
    static const juce::String filterResonance = "filterResonance";
    static const juce::String filterDrive = "filterDrive";
    static const juce::String filterMode = "filterMode";

    // Envelope 1 (Amp)
    static const juce::String attack = "attack";
    static const juce::String decay = "decay";
    static const juce::String sustain = "sustain";
    static const juce::String release = "release";

    // Envelope 2 (Filter)
    static const juce::String env2Attack = "env2Attack";
    static const juce::String env2Decay = "env2Decay";
    static const juce::String env2Sustain = "env2Sustain";
    static const juce::String env2Release = "env2Release";

    // Waveshaper
    static const juce::String shaperMode = "shaperMode";
    static const juce::String shaperAmount = "shaperAmount";
    static const juce::String shaperMix = "shaperMix";

    // FM Modulation
    static const juce::String fm12 = "fm12";
    static const juce::String fm13 = "fm13";
    static const juce::String fm23 = "fm23";

    // AM Modulation
    static const juce::String am12 = "am12";
    static const juce::String am13 = "am13";
    static const juce::String am23 = "am23";

    // Wavetable
    static const juce::String wavetableIndex = "wavetableIndex";

    // LFO 1
    static const juce::String lfo1Waveform = "lfo1Waveform";
    static const juce::String lfo1Rate = "lfo1Rate";
    static const juce::String lfo1Depth = "lfo1Depth";
    static const juce::String lfo1TempoSync = "lfo1TempoSync";
    static const juce::String lfo1TempoRate = "lfo1TempoRate";

    // LFO 2
    static const juce::String lfo2Waveform = "lfo2Waveform";
    static const juce::String lfo2Rate = "lfo2Rate";
    static const juce::String lfo2Depth = "lfo2Depth";
    static const juce::String lfo2TempoSync = "lfo2TempoSync";
    static const juce::String lfo2TempoRate = "lfo2TempoRate";

    // LFO 3
    static const juce::String lfo3Waveform = "lfo3Waveform";
    static const juce::String lfo3Rate = "lfo3Rate";
    static const juce::String lfo3Depth = "lfo3Depth";
    static const juce::String lfo3TempoSync = "lfo3TempoSync";
    static const juce::String lfo3TempoRate = "lfo3TempoRate";

    // LFO 4
    static const juce::String lfo4Waveform = "lfo4Waveform";
    static const juce::String lfo4Rate = "lfo4Rate";
    static const juce::String lfo4Depth = "lfo4Depth";
    static const juce::String lfo4TempoSync = "lfo4TempoSync";
    static const juce::String lfo4TempoRate = "lfo4TempoRate";

    // FX - Chorus
    static const juce::String chorusMix = "chorusMix";
    static const juce::String chorusRate = "chorusRate";
    static const juce::String chorusDepth = "chorusDepth";

    // FX - Delay
    static const juce::String delayMix = "delayMix";
    static const juce::String delayTime = "delayTime";
    static const juce::String delayFeedback = "delayFeedback";

    // FX - Reverb
    static const juce::String reverbMix = "reverbMix";
    static const juce::String reverbSize = "reverbSize";
    static const juce::String reverbDamping = "reverbDamping";
}

//==============================================================================
InternalSynthProcessor::InternalSynthProcessor()
     : AudioProcessor (BusesProperties()
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                       ),
       apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Initialize parameter pointers for fast access
    masterVolumeParam = apvts.getRawParameterValue(ParamIDs::masterVolume);

    // OSC 1
    osc1LevelParam = apvts.getRawParameterValue(ParamIDs::osc1Level);
    osc1MorphParam = apvts.getRawParameterValue(ParamIDs::osc1Morph);
    osc1DetuneParam = apvts.getRawParameterValue(ParamIDs::osc1Detune);
    osc1PanParam = apvts.getRawParameterValue(ParamIDs::osc1Pan);
    osc1UnisonParam = apvts.getRawParameterValue(ParamIDs::osc1Unison);
    osc1PanSpreadParam = apvts.getRawParameterValue(ParamIDs::osc1PanSpread);

    // OSC 2
    osc2LevelParam = apvts.getRawParameterValue(ParamIDs::osc2Level);
    osc2MorphParam = apvts.getRawParameterValue(ParamIDs::osc2Morph);
    osc2DetuneParam = apvts.getRawParameterValue(ParamIDs::osc2Detune);
    osc2PanParam = apvts.getRawParameterValue(ParamIDs::osc2Pan);
    osc2UnisonParam = apvts.getRawParameterValue(ParamIDs::osc2Unison);
    osc2PanSpreadParam = apvts.getRawParameterValue(ParamIDs::osc2PanSpread);

    // OSC 3
    osc3LevelParam = apvts.getRawParameterValue(ParamIDs::osc3Level);
    osc3MorphParam = apvts.getRawParameterValue(ParamIDs::osc3Morph);
    osc3DetuneParam = apvts.getRawParameterValue(ParamIDs::osc3Detune);
    osc3PanParam = apvts.getRawParameterValue(ParamIDs::osc3Pan);
    osc3UnisonParam = apvts.getRawParameterValue(ParamIDs::osc3Unison);
    osc3PanSpreadParam = apvts.getRawParameterValue(ParamIDs::osc3PanSpread);

    // Sub Oscillator
    subLevelParam = apvts.getRawParameterValue(ParamIDs::subLevel);
    subOctaveParam = apvts.getRawParameterValue(ParamIDs::subOctave);
    subWaveformParam = apvts.getRawParameterValue(ParamIDs::subWaveform);

    // Filter
    filterCutoffParam = apvts.getRawParameterValue(ParamIDs::filterCutoff);
    filterResonanceParam = apvts.getRawParameterValue(ParamIDs::filterResonance);
    filterDriveParam = apvts.getRawParameterValue(ParamIDs::filterDrive);
    filterModeParam = apvts.getRawParameterValue(ParamIDs::filterMode);

    // Envelope 1 (Amp)
    attackParam = apvts.getRawParameterValue(ParamIDs::attack);
    decayParam = apvts.getRawParameterValue(ParamIDs::decay);
    sustainParam = apvts.getRawParameterValue(ParamIDs::sustain);
    releaseParam = apvts.getRawParameterValue(ParamIDs::release);

    // Envelope 2 (Filter)
    env2AttackParam = apvts.getRawParameterValue(ParamIDs::env2Attack);
    env2DecayParam = apvts.getRawParameterValue(ParamIDs::env2Decay);
    env2SustainParam = apvts.getRawParameterValue(ParamIDs::env2Sustain);
    env2ReleaseParam = apvts.getRawParameterValue(ParamIDs::env2Release);

    // Waveshaper
    shaperModeParam = apvts.getRawParameterValue(ParamIDs::shaperMode);
    shaperAmountParam = apvts.getRawParameterValue(ParamIDs::shaperAmount);
    shaperMixParam = apvts.getRawParameterValue(ParamIDs::shaperMix);

    // FM Modulation
    fm12Param = apvts.getRawParameterValue(ParamIDs::fm12);
    fm13Param = apvts.getRawParameterValue(ParamIDs::fm13);
    fm23Param = apvts.getRawParameterValue(ParamIDs::fm23);

    // AM Modulation
    am12Param = apvts.getRawParameterValue(ParamIDs::am12);
    am13Param = apvts.getRawParameterValue(ParamIDs::am13);
    am23Param = apvts.getRawParameterValue(ParamIDs::am23);

    // Wavetable
    wavetableIndexParam = apvts.getRawParameterValue(ParamIDs::wavetableIndex);

    // LFO 1
    lfo1WaveformParam = apvts.getRawParameterValue(ParamIDs::lfo1Waveform);
    lfo1RateParam = apvts.getRawParameterValue(ParamIDs::lfo1Rate);
    lfo1DepthParam = apvts.getRawParameterValue(ParamIDs::lfo1Depth);
    lfo1TempoSyncParam = apvts.getRawParameterValue(ParamIDs::lfo1TempoSync);
    lfo1TempoRateParam = apvts.getRawParameterValue(ParamIDs::lfo1TempoRate);

    // LFO 2
    lfo2WaveformParam = apvts.getRawParameterValue(ParamIDs::lfo2Waveform);
    lfo2RateParam = apvts.getRawParameterValue(ParamIDs::lfo2Rate);
    lfo2DepthParam = apvts.getRawParameterValue(ParamIDs::lfo2Depth);
    lfo2TempoSyncParam = apvts.getRawParameterValue(ParamIDs::lfo2TempoSync);
    lfo2TempoRateParam = apvts.getRawParameterValue(ParamIDs::lfo2TempoRate);

    // LFO 3
    lfo3WaveformParam = apvts.getRawParameterValue(ParamIDs::lfo3Waveform);
    lfo3RateParam = apvts.getRawParameterValue(ParamIDs::lfo3Rate);
    lfo3DepthParam = apvts.getRawParameterValue(ParamIDs::lfo3Depth);
    lfo3TempoSyncParam = apvts.getRawParameterValue(ParamIDs::lfo3TempoSync);
    lfo3TempoRateParam = apvts.getRawParameterValue(ParamIDs::lfo3TempoRate);

    // LFO 4
    lfo4WaveformParam = apvts.getRawParameterValue(ParamIDs::lfo4Waveform);
    lfo4RateParam = apvts.getRawParameterValue(ParamIDs::lfo4Rate);
    lfo4DepthParam = apvts.getRawParameterValue(ParamIDs::lfo4Depth);
    lfo4TempoSyncParam = apvts.getRawParameterValue(ParamIDs::lfo4TempoSync);
    lfo4TempoRateParam = apvts.getRawParameterValue(ParamIDs::lfo4TempoRate);

    // FX - Chorus
    chorusMixParam = apvts.getRawParameterValue(ParamIDs::chorusMix);
    chorusRateParam = apvts.getRawParameterValue(ParamIDs::chorusRate);
    chorusDepthParam = apvts.getRawParameterValue(ParamIDs::chorusDepth);

    // FX - Delay
    delayMixParam = apvts.getRawParameterValue(ParamIDs::delayMix);
    delayTimeParam = apvts.getRawParameterValue(ParamIDs::delayTime);
    delayFeedbackParam = apvts.getRawParameterValue(ParamIDs::delayFeedback);

    // FX - Reverb
    reverbMixParam = apvts.getRawParameterValue(ParamIDs::reverbMix);
    reverbSizeParam = apvts.getRawParameterValue(ParamIDs::reverbSize);
    reverbDampingParam = apvts.getRawParameterValue(ParamIDs::reverbDamping);

    // Create the wavetable engine
    wavetableEngine = std::make_unique<WavetableEngine>();

    // Initialize preset names
    initPresetNames();
}

InternalSynthProcessor::~InternalSynthProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout InternalSynthProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::masterVolume, "Master Volume", 0.0f, 1.0f, 0.8f));

    // OSC 1
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc1Level, "OSC 1 Level", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc1Morph, "OSC 1 Morph", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc1Detune, "OSC 1 Detune", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc1Pan, "OSC 1 Pan", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::osc1Unison, "OSC 1 Unison", 1, 8, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc1PanSpread, "OSC 1 Pan Spread", 0.0f, 1.0f, 0.0f));

    // OSC 2
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc2Level, "OSC 2 Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc2Morph, "OSC 2 Morph", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc2Detune, "OSC 2 Detune", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc2Pan, "OSC 2 Pan", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::osc2Unison, "OSC 2 Unison", 1, 8, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc2PanSpread, "OSC 2 Pan Spread", 0.0f, 1.0f, 0.0f));

    // OSC 3
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc3Level, "OSC 3 Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc3Morph, "OSC 3 Morph", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc3Detune, "OSC 3 Detune", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc3Pan, "OSC 3 Pan", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::osc3Unison, "OSC 3 Unison", 1, 8, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::osc3PanSpread, "OSC 3 Pan Spread", 0.0f, 1.0f, 0.0f));

    // Sub Oscillator
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::subLevel, "Sub Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::subOctave, "Sub Octave", 0, 2, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::subWaveform, "Sub Waveform",
        juce::StringArray {"Sine", "Triangle", "Saw", "Square", "Noise"}, 0));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::filterCutoff, "Filter Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.5f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::filterResonance, "Filter Resonance", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::filterDrive, "Filter Drive", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::filterMode, "Filter Mode",
        juce::StringArray {"Low-pass", "High-pass", "Band-pass", "Notch"}, 0));

    // Envelope 1 (Amp)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::attack, "Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::decay, "Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::sustain, "Sustain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::release, "Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.5f), 0.3f));

    // Envelope 2 (Filter)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::env2Attack, "Filter Env Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::env2Decay, "Filter Env Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.5f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::env2Sustain, "Filter Env Sustain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::env2Release, "Filter Env Release",
        juce::NormalisableRange<float>(0.001f, 10.0f, 0.001f, 0.5f), 0.3f));

    // Waveshaper
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::shaperMode, "Shaper Mode",
        juce::StringArray {"Off", "Soft Clip", "Hard Clip", "Foldback", "Bitcrush"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::shaperAmount, "Shaper Amount", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::shaperMix, "Shaper Mix", 0.0f, 1.0f, 0.0f));

    // FM Modulation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::fm12, "FM 1>2", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::fm13, "FM 1>3", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::fm23, "FM 2>3", 0.0f, 1.0f, 0.0f));

    // AM Modulation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::am12, "AM 1>2", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::am13, "AM 1>3", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::am23, "AM 2>3", 0.0f, 1.0f, 0.0f));

    // Wavetable Selection
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        ParamIDs::wavetableIndex, "Wavetable Index", 0, 63, 0));

    // LFO 1
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lfo1Waveform, "LFO 1 Waveform",
        juce::StringArray {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo1Rate, "LFO 1 Rate", 0.01f, 20.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo1Depth, "LFO 1 Depth", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::lfo1TempoSync, "LFO 1 Tempo Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo1TempoRate, "LFO 1 Tempo Rate",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.25f), 0.25f));

    // LFO 2
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lfo2Waveform, "LFO 2 Waveform",
        juce::StringArray {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo2Rate, "LFO 2 Rate", 0.01f, 20.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo2Depth, "LFO 2 Depth", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::lfo2TempoSync, "LFO 2 Tempo Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo2TempoRate, "LFO 2 Tempo Rate",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.25f), 0.5f));

    // LFO 3
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lfo3Waveform, "LFO 3 Waveform",
        juce::StringArray {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo3Rate, "LFO 3 Rate", 0.01f, 20.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo3Depth, "LFO 3 Depth", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::lfo3TempoSync, "LFO 3 Tempo Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo3TempoRate, "LFO 3 Tempo Rate",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.25f), 1.0f));

    // LFO 4
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::lfo4Waveform, "LFO 4 Waveform",
        juce::StringArray {"Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo4Rate, "LFO 4 Rate", 0.01f, 20.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo4Depth, "LFO 4 Depth", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::lfo4TempoSync, "LFO 4 Tempo Sync", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lfo4TempoRate, "LFO 4 Tempo Rate",
        juce::NormalisableRange<float>(0.25f, 4.0f, 0.25f), 0.25f));

    // FX - Chorus
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusMix, "Chorus Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusRate, "Chorus Rate", 0.1f, 10.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::chorusDepth, "Chorus Depth", 0.0f, 1.0f, 0.25f));

    // FX - Delay
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::delayMix, "Delay Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::delayTime, "Delay Time", 0.01f, 2.0f, 0.33f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::delayFeedback, "Delay Feedback", 0.0f, 0.95f, 0.4f));

    // FX - Reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::reverbMix, "Reverb Mix", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::reverbSize, "Reverb Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::reverbDamping, "Reverb Damping", 0.0f, 1.0f, 0.5f));

    return { params.begin(), params.end() };
}

//==============================================================================
void InternalSynthProcessor::initPresetNames()
{
    programNames.add("Init");
    programNames.add("Bass 1");
    programNames.add("Lead 1");
    programNames.add("Pad 1");
    programNames.add("Pluck 1");
}

//==============================================================================
void InternalSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    wavetableEngine->prepareToPlay(samplesPerBlock, sampleRate);
    updateEngineParameters();
}

void InternalSynthProcessor::releaseResources()
{
    wavetableEngine->releaseResources();
}

bool InternalSynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
void InternalSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    updateEngineParameters();
    buffer.clear();
    wavetableEngine->processBlock(buffer, midiMessages);
}

void InternalSynthProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::AudioBuffer<float> floatBuffer(buffer.getNumChannels(), buffer.getNumSamples());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* dest = floatBuffer.getWritePointer(channel);
        auto* src = buffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            dest[sample] = static_cast<float>(src[sample]);
    }

    processBlock(floatBuffer, midiMessages);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* dest = buffer.getWritePointer(channel);
        auto* src = floatBuffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            dest[sample] = static_cast<double>(src[sample]);
    }
}

//==============================================================================
void InternalSynthProcessor::updateEngineParameters()
{
    if (!wavetableEngine)
        return;

    float masterVol = masterVolumeParam ? masterVolumeParam->load() : 0.8f;
    wavetableEngine->setMasterVolume(masterVol);

    auto& synth = wavetableEngine->getSynthesiser();
    auto params = synth.getSharedParams();

    if (params)
    {
        // OSC 1
        if (osc1LevelParam) params->setOscLevel(0, osc1LevelParam->load());
        if (osc1MorphParam) params->setOscMorph(0, osc1MorphParam->load());
        if (osc1DetuneParam) params->setOscDetune(0, osc1DetuneParam->load());
        if (osc1PanParam) params->setOscPan(0, osc1PanParam->load());
        if (osc1UnisonParam) params->setOscUnisonCount(0, static_cast<int>(osc1UnisonParam->load()));
        if (osc1PanSpreadParam) params->setOscPanSpread(0, osc1PanSpreadParam->load());

        // OSC 2
        if (osc2LevelParam) params->setOscLevel(1, osc2LevelParam->load());
        if (osc2MorphParam) params->setOscMorph(1, osc2MorphParam->load());
        if (osc2DetuneParam) params->setOscDetune(1, osc2DetuneParam->load());
        if (osc2PanParam) params->setOscPan(1, osc2PanParam->load());
        if (osc2UnisonParam) params->setOscUnisonCount(1, static_cast<int>(osc2UnisonParam->load()));
        if (osc2PanSpreadParam) params->setOscPanSpread(1, osc2PanSpreadParam->load());

        // OSC 3
        if (osc3LevelParam) params->setOscLevel(2, osc3LevelParam->load());
        if (osc3MorphParam) params->setOscMorph(2, osc3MorphParam->load());
        if (osc3DetuneParam) params->setOscDetune(2, osc3DetuneParam->load());
        if (osc3PanParam) params->setOscPan(2, osc3PanParam->load());
        if (osc3UnisonParam) params->setOscUnisonCount(2, static_cast<int>(osc3UnisonParam->load()));
        if (osc3PanSpreadParam) params->setOscPanSpread(2, osc3PanSpreadParam->load());

        // Sub Oscillator
        if (subLevelParam) params->setSubLevel(subLevelParam->load());
        if (subOctaveParam) params->setSubOctave(static_cast<int>(subOctaveParam->load()));
        if (subWaveformParam) params->setSubWaveform(static_cast<int>(subWaveformParam->load()));

        // Filter
        if (filterCutoffParam) params->setFilterCutoff(filterCutoffParam->load());
        if (filterResonanceParam) params->setFilterResonance(filterResonanceParam->load());
        if (filterDriveParam) params->setFilterDrive(filterDriveParam->load());
        if (filterModeParam) params->setFilterMode(static_cast<int>(filterModeParam->load()));

        // Envelope 1 (Amp)
        if (attackParam) params->setEnvAttack(attackParam->load());
        if (decayParam) params->setEnvDecay(decayParam->load());
        if (sustainParam) params->setEnvSustain(sustainParam->load());
        if (releaseParam) params->setEnvRelease(releaseParam->load());

        // Waveshaper
        if (shaperModeParam) params->setShaperMode(static_cast<int>(shaperModeParam->load()));
        if (shaperAmountParam) params->setShaperAmount(shaperAmountParam->load());
        if (shaperMixParam) params->setShaperMix(shaperMixParam->load());

        // FM Modulation
        if (fm12Param) params->setFMAmount12(fm12Param->load());
        if (fm13Param) params->setFMAmount13(fm13Param->load());
        if (fm23Param) params->setFMAmount23(fm23Param->load());

        // AM Modulation
        if (am12Param) params->setAMAmount12(am12Param->load());
        if (am13Param) params->setAMAmount13(am13Param->load());
        if (am23Param) params->setAMAmount23(am23Param->load());

        // Wavetable Index
        if (wavetableIndexParam) params->setWavetableIndex(static_cast<int>(wavetableIndexParam->load()));
    }

    // FX parameters
    auto& fxParams = wavetableEngine->getFXParams();

    if (chorusMixParam) fxParams.chorusMix.store(chorusMixParam->load());
    if (chorusRateParam) fxParams.chorusRate.store(chorusRateParam->load());
    if (chorusDepthParam) fxParams.chorusDepth.store(chorusDepthParam->load());

    if (delayMixParam) fxParams.delayMix.store(delayMixParam->load());
    if (delayTimeParam) fxParams.delayTime.store(delayTimeParam->load());
    if (delayFeedbackParam) fxParams.delayFeedback.store(delayFeedbackParam->load());

    if (reverbMixParam) fxParams.reverbMix.store(reverbMixParam->load());
    if (reverbSizeParam) fxParams.reverbSize.store(reverbSizeParam->load());
    if (reverbDampingParam) fxParams.reverbDamping.store(reverbDampingParam->load());
}

//==============================================================================
std::shared_ptr<WavetableParams> InternalSynthProcessor::getWavetableParams()
{
    if (wavetableEngine)
        return wavetableEngine->getSynthesiser().getSharedParams();
    return nullptr;
}

//==============================================================================
juce::AudioProcessorEditor* InternalSynthProcessor::createEditor()
{
    return new InternalSynthEditor(*this);
}

//==============================================================================
int InternalSynthProcessor::getNumPrograms()
{
    return programNames.size();
}

int InternalSynthProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void InternalSynthProcessor::setCurrentProgram(int index)
{
    if (index >= 0 && index < programNames.size())
        currentProgramIndex = index;
}

const juce::String InternalSynthProcessor::getProgramName(int index)
{
    if (index >= 0 && index < programNames.size())
        return programNames[index];
    return {};
}

void InternalSynthProcessor::changeProgramName(int index, const juce::String& newName)
{
    if (index >= 0 && index < programNames.size())
        programNames.set(index, newName);
}

//==============================================================================
void InternalSynthProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
        copyXmlToBinary(*xml, destData);
}

void InternalSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
    {
        auto newState = juce::ValueTree::fromXml(*xmlState);
        if (newState.isValid())
            apvts.replaceState(newState);
    }
}
