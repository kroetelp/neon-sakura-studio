#include "WavetablePresetManager.h"
#include "WavetableSynth.h"
#include "WavetableVoice.h"
#include "../Modulation/ModulationMatrix.h"
#include "../Modulation/LFOModulator.h"
#include "../Modulation/EnvelopeModulator.h"

WavetablePresetManager::WavetablePresetManager()
{
    // Set up directories
    applicationDirectory = juce::File::getSpecialLocation(juce::File::currentApplicationFile)
        .getParentDirectory();
    userPresetDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile(USER_PRESET_DIR);

    ensureDirectoriesExist();
}

juce::File WavetablePresetManager::getPresetDirectory() const
{
    // Return user directory as default for saving
    return userPresetDirectory;
}

juce::File WavetablePresetManager::getUserPresetDirectory() const
{
    return userPresetDirectory;
}

juce::File WavetablePresetManager::getFactoryPresetDirectory() const
{
    return applicationDirectory.getChildFile(FACTORY_PRESET_DIR);
}

void WavetablePresetManager::ensureDirectoriesExist()
{
    if (!userPresetDirectory.exists())
        userPresetDirectory.createDirectory();

    auto factoryDir = getFactoryPresetDirectory();
    if (!factoryDir.exists())
    {
        factoryDir.createDirectory();
        createFactoryPresets();
    }
}

bool WavetablePresetManager::savePresetToFile(const WavetablePreset& preset, const juce::File& file)
{
    juce::String json = preset.toJSON();

    // Write directly to file (JUCE 8 simplified approach)
    if (file.replaceWithText(json))
        return true;

    return false;
}

WavetablePreset WavetablePresetManager::loadPresetFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return WavetablePreset::createInitPreset();

    juce::String json = file.loadFileAsString();
    return WavetablePreset::fromJSON(json);
}

bool WavetablePresetManager::saveUserPreset(const WavetablePreset& preset)
{
    juce::String safeName = preset.name
        .replaceCharacter('/', '_')
        .replaceCharacter('\\', '_')
        .replaceCharacter(':', '_')
        .replaceCharacter('*', '_')
        .replaceCharacter('?', '_')
        .replaceCharacter('"', '_')
        .replaceCharacter('<', '_')
        .replaceCharacter('>', '_')
        .replaceCharacter('|', '_');

    juce::File file = userPresetDirectory.getChildFile(safeName + PRESET_EXTENSION);
    return savePresetToFile(preset, file);
}

juce::Array<juce::File> WavetablePresetManager::getAllPresetFiles() const
{
    juce::Array<juce::File> files;

    // Add factory presets
    files.addArray(getFactoryPresetFiles());

    // Add user presets
    files.addArray(getUserPresetFiles());

    // Sort by name using lambda
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b) {
        return a.getFileName().compareNatural(b.getFileName()) < 0;
    });

    return files;
}

juce::Array<juce::File> WavetablePresetManager::getFactoryPresetFiles() const
{
    juce::Array<juce::File> files;
    auto factoryDir = getFactoryPresetDirectory();

    if (factoryDir.isDirectory())
        factoryDir.findChildFiles(files, juce::File::findFiles, false, "*" + juce::String(PRESET_EXTENSION));

    return files;
}

juce::Array<juce::File> WavetablePresetManager::getUserPresetFiles() const
{
    juce::Array<juce::File> files;

    if (userPresetDirectory.isDirectory())
        userPresetDirectory.findChildFiles(files, juce::File::findFiles, false, "*" + juce::String(PRESET_EXTENSION));

    return files;
}

juce::Array<WavetablePreset> WavetablePresetManager::loadAllPresets()
{
    juce::Array<WavetablePreset> presets;
    auto files = getAllPresetFiles();

    for (const auto& file : files)
        presets.add(loadPresetFromFile(file));

    return presets;
}

WavetablePreset WavetablePresetManager::createBassPreset()
{
    WavetablePreset preset;
    preset.name = "Bass - Deep";
    preset.description = "Deep, punchy bass sound";

    // OSC 1 - main tone
    preset.oscParams[0].level = 0.9f;
    preset.oscParams[0].morph = 0.3f;
    preset.oscParams[0].detune = 0.1f;
    preset.oscParams[0].unisonCount = 3;

    // OSC 2 - sub layer
    preset.oscParams[1].level = 0.5f;
    preset.oscParams[1].morph = 0.0f;
    preset.oscParams[1].pitchOffset = -12.0f;

    // Sub
    preset.subOscParams.level = 0.4f;
    preset.subOscParams.octave = 1;
    preset.subOscParams.waveform = 0;  // Sine

    // Filter
    preset.filterParams.cutoff = 800.0f;
    preset.filterParams.resonance = 0.3f;
    preset.filterParams.type = 0;  // LP

    // Envelope - punchy
    preset.ampEnvelope.attack = 0.001f;
    preset.ampEnvelope.decay = 0.2f;
    preset.ampEnvelope.sustain = 0.4f;
    preset.ampEnvelope.release = 0.1f;

    preset.masterVolume = 0.85f;

    return preset;
}

WavetablePreset WavetablePresetManager::createLeadPreset()
{
    WavetablePreset preset;
    preset.name = "Lead - Bright";
    preset.description = "Bright, cutting lead sound";

    // OSC 1 - bright
    preset.oscParams[0].level = 0.8f;
    preset.oscParams[0].morph = 0.7f;
    preset.oscParams[0].detune = 0.0f;

    // OSC 2 - detuned
    preset.oscParams[1].level = 0.6f;
    preset.oscParams[1].morph = 0.7f;
    preset.oscParams[1].detune = 0.15f;
    preset.oscParams[1].pan = -0.3f;

    // OSC 3 - octave up
    preset.oscParams[2].level = 0.3f;
    preset.oscParams[2].pitchOffset = 12.0f;
    preset.oscParams[2].pan = 0.3f;

    // Filter
    preset.filterParams.cutoff = 4000.0f;
    preset.filterParams.resonance = 0.2f;
    preset.filterParams.type = 0;  // LP

    // Envelope
    preset.ampEnvelope.attack = 0.01f;
    preset.ampEnvelope.decay = 0.4f;
    preset.ampEnvelope.sustain = 0.6f;
    preset.ampEnvelope.release = 0.3f;

    preset.masterVolume = 0.8f;

    return preset;
}

WavetablePreset WavetablePresetManager::createPadPreset()
{
    WavetablePreset preset;
    preset.name = "Pad - Atmospheric";
    preset.description = "Lush, atmospheric pad";

    // All oscillators with slow morph
    preset.oscParams[0].level = 0.6f;
    preset.oscParams[0].morph = 0.5f;
    preset.oscParams[0].detune = 0.05f;
    preset.oscParams[0].unisonCount = 5;
    preset.oscParams[0].panSpread = 0.8f;

    preset.oscParams[1].level = 0.5f;
    preset.oscParams[1].morph = 0.6f;
    preset.oscParams[1].detune = 0.08f;
    preset.oscParams[1].pan = -0.4f;

    preset.oscParams[2].level = 0.4f;
    preset.oscParams[2].morph = 0.4f;
    preset.oscParams[2].detune = 0.03f;
    preset.oscParams[2].pan = 0.4f;

    // Sub
    preset.subOscParams.level = 0.3f;

    // Filter
    preset.filterParams.cutoff = 2000.0f;
    preset.filterParams.resonance = 0.1f;

    // Slow envelope
    preset.ampEnvelope.attack = 0.8f;
    preset.ampEnvelope.decay = 0.5f;
    preset.ampEnvelope.sustain = 0.8f;
    preset.ampEnvelope.release = 1.0f;

    // LFO for subtle movement
    preset.lfoParams[0].rate = 0.2f;
    preset.lfoParams[0].depth = 0.3f;
    preset.lfoParams[0].waveform = 0;  // Sine

    // Modulation: LFO1 -> Filter Cutoff
    WavetablePreset::ModRouting routing;
    routing.source = static_cast<int>(ModulationSource::LFO1);
    routing.target = static_cast<int>(ModulationTarget::Filter_Cutoff);
    routing.amount = 0.3f;
    preset.modRoutings.add(routing);

    preset.masterVolume = 0.7f;

    return preset;
}
WavetablePreset WavetablePresetManager::createPluckPreset()
{
    WavetablePreset preset;
    preset.name = "Pluck - Digital";
    preset.description = "Short, digital pluck sound";

    // OSC 1
    preset.oscParams[0].level = 0.8f;
    preset.oscParams[0].morph = 0.2f;

    // OSC 2 - pitch envelope effect via high offset
    preset.oscParams[1].level = 0.4f;
    preset.oscParams[1].pitchOffset = 7.0f;  // Fifth
    // Filter - fast decay
    preset.filterParams.cutoff = 5000.0f;
    preset.filterParams.resonance = 0.4f;
    // Fast envelope for pluck
    preset.ampEnvelope.attack = 0.001f;
    preset.ampEnvelope.decay = 0.15f;
    preset.ampEnvelope.sustain = 0.0f;
    preset.ampEnvelope.release = 0.1f;
    preset.masterVolume = 0.8f;
    return preset;
}
WavetablePreset WavetablePresetManager::createWobblePreset()
{
    WavetablePreset preset;
    preset.name = "Wobble - LFO";
    preset.description = "LFO-modulated wobble bass";
    // OSC 1 - main
    preset.oscParams[0].level = 0.85f;
    preset.oscParams[0].morph = 0.5f;
    preset.oscParams[0].unisonCount = 3;
    // OSC 2
    preset.oscParams[1].level = 0.5f;
    preset.oscParams[1].detune = 0.1f;
    // Sub
    preset.subOscParams.level = 0.4f;
    // Filter
    preset.filterParams.cutoff = 1000.0f;
    preset.filterParams.resonance = 0.5f;
    // Envelope
    preset.ampEnvelope.attack = 0.005f;
    preset.ampEnvelope.decay = 0.3f;
    preset.ampEnvelope.sustain = 0.6f;
    preset.ampEnvelope.release = 0.2f;
    // LFO1 for wobble - tempo synced
    preset.lfoParams[0].rate = 2.0f;
    preset.lfoParams[0].depth = 0.8f;
    preset.lfoParams[0].waveform = 3;  // Square
    preset.lfoParams[0].tempoSync = true;
    preset.lfoParams[0].syncRate = 1;  // 1/8
    // Modulation: LFO1 -> Filter Cutoff
    WavetablePreset::ModRouting routing;
    routing.source = static_cast<int>(ModulationSource::LFO1);
    routing.target = static_cast<int>(ModulationTarget::Filter_Cutoff);
    routing.amount = 0.7f;
    preset.modRoutings.add(routing);
    preset.masterVolume = 0.85f;
    return preset;
}

// New Preset 1: evolving pad
WavetablePreset WavetablePresetManager::createEvolvingPadPreset()
{
    WavetablePreset preset;
    preset.name = "Evolving Pad";
    preset.description = "Slowly evolving ambient texture";

    // All 3 oscillators blended
    preset.oscParams[0].level = 0.5f;
    preset.oscParams[0].morph = 0.3f;
    preset.oscParams[0].detune = 0.02f;
    preset.oscParams[0].unisonCount = 7;
    preset.oscParams[0].panSpread = 0.6f;

    preset.oscParams[1].level = 0.5f;
    preset.oscParams[1].morph = 0.5f;
    preset.oscParams[1].detune = 0.05f;
    preset.oscParams[1].pan = -0.3f;

    preset.oscParams[2].level = 0.4f;
    preset.oscParams[2].morph = 0.7f;
    preset.oscParams[2].detune = 0.08f;
    preset.oscParams[2].pan = 0.3f;

    // Sub for warmth
    preset.subOscParams.level = 0.2f;
    preset.subOscParams.octave = -1;

    // Wide open filter
    preset.filterParams.cutoff = 12000.0f;
    preset.filterParams.resonance = 0.05f;

    // Long envelope
    preset.ampEnvelope.attack = 1.5f;
    preset.ampEnvelope.decay = 0.8f;
    preset.ampEnvelope.sustain = 0.7f;
    preset.ampEnvelope.release = 2.0f;

    // LFO for slow morphing
    preset.lfoParams[0].rate = 0.05f;
    preset.lfoParams[0].depth = 0.4f;
    preset.lfoParams[0].waveform = 0;  // Sine
    preset.lfoParams[0].tempoSync = false;

    // LFO2 for filter movement
    preset.lfoParams[1].rate = 0.1f;
    preset.lfoParams[1].depth = 0.5f;
    preset.lfoParams[1].waveform = 1;  // Triangle
    preset.lfoParams[1].tempoSync = false;

    // Modulation routings
    WavetablePreset::ModRouting routing1;
    routing1.source = static_cast<int>(ModulationSource::LFO1);
    routing1.target = static_cast<int>(ModulationTarget::Osc1_Morph);
    routing1.amount = 0.4f;
    preset.modRoutings.add(routing1);

    WavetablePreset::ModRouting routing2;
    routing2.source = static_cast<int>(ModulationSource::LFO2);
    routing2.target = static_cast<int>(ModulationTarget::Filter_Cutoff);
    routing2.amount = 0.5f;
    preset.modRoutings.add(routing2);

    preset.masterVolume = 0.6f;

    return preset;
}
// New Preset 2: aggressive lead
WavetablePreset WavetablePresetManager::createAggressiveLeadPreset()
{
    WavetablePreset preset;
    preset.name = "Aggressive Lead";
    preset.description = "Sharp, aggressive lead with unison";
    // OSC 1 - main with unison
    preset.oscParams[0].level = 0.9f;
    preset.oscParams[0].morph = 0.8f;
    preset.oscParams[0].detune = 0.3f;
    preset.oscParams[0].unisonCount = 5;
    preset.oscParams[0].panSpread = 0.9f;

    // OSC 2 - fifth above
    preset.oscParams[1].level = 0.3f;
    preset.oscParams[1].morph = 0.9f;
    preset.oscParams[1].pitchOffset = 7.0f;

    // OSC 3 - octave above
    preset.oscParams[2].level = 0.2f;
    preset.oscParams[2].morph = 0.95f;
    preset.oscParams[2].pitchOffset = 12.0f;

    // Aggressive filter
    preset.filterParams.cutoff = 3000.0f;
    preset.filterParams.resonance = 0.6f;
    preset.filterParams.drive = 0.3f;
    preset.filterParams.type = 0;  // LP

    // Snappy envelope
    preset.ampEnvelope.attack = 0.005f;
    preset.ampEnvelope.decay = 0.1f;
    preset.ampEnvelope.sustain = 0.5f;
    preset.ampEnvelope.release = 0.2f;

    preset.masterVolume = 0.75f;

    return preset;
}
// New Preset 3: sub bass
WavetablePreset WavetablePresetManager::createSubBassPreset()
{
    WavetablePreset preset;
    preset.name = "Sub Bass";
    preset.description = "Deep sub oscillator focused bass";
    // OSC 1 - subtle top
    preset.oscParams[0].level = 0.3f;
    preset.oscParams[0].morph = 0.1f;

    // OSC 2 - mid layer
    preset.oscParams[1].level = 0.4f;
    preset.oscParams[1].morph = 0.2f;
    preset.oscParams[1].pitchOffset = -5.0f;

    // Heavy sub oscillator
    preset.subOscParams.level = 0.8f;
    preset.subOscParams.octave = -2;
    preset.subOscParams.waveform = 1;  // Square

    // Tight filter for sub focus
    preset.filterParams.cutoff = 400.0f;
    preset.filterParams.resonance = 0.7f;
    preset.filterParams.type = 0;  // LP

    // Punchy envelope
    preset.ampEnvelope.attack = 0.002f;
    preset.ampEnvelope.decay = 0.1f;
    preset.ampEnvelope.sustain = 0.6f;
    preset.ampEnvelope.release = 0.15f;

    preset.masterVolume = 0.9f;

    return preset;
}
void WavetablePresetManager::createFactoryPresets()
{
    auto factoryDir = getFactoryPresetDirectory();
    savePresetToFile(createBassPreset(), factoryDir.getChildFile("Bass - Deep.wtpreset"));
    savePresetToFile(createLeadPreset(), factoryDir.getChildFile("Lead - Bright.wtpreset"));
    savePresetToFile(createPadPreset(), factoryDir.getChildFile("Pad - Atmospheric.wtpreset"));
    savePresetToFile(createPluckPreset(), factoryDir.getChildFile("Pluck - Digital.wtpreset"));
    savePresetToFile(createWobblePreset(), factoryDir.getChildFile("Wobble - LFO.wtpreset"));
    savePresetToFile(createEvolvingPadPreset(), factoryDir.getChildFile("Evolving Pad.wtpreset"));
    savePresetToFile(createAggressiveLeadPreset(), factoryDir.getChildFile("Aggressive Lead.wtpreset"));
    savePresetToFile(createSubBassPreset(), factoryDir.getChildFile("Sub Bass.wtpreset"));
    savePresetToFile(WavetablePreset::createInitPreset(), factoryDir.getChildFile("Init.wtpreset"));
}

void WavetablePresetManager::applyPresetToSynth(const WavetablePreset& preset, WavetableSynth& synth)
{
    auto& params = synth.getParams();

    // Apply oscillator parameters
    for (int i = 0; i < 3; ++i)
    {
        const auto& oscP = preset.oscParams[i];
        params.oscLevels[i].store(oscP.level);
        params.oscMorphs[i].store(oscP.morph);
        params.oscDetunes[i].store(oscP.detune);
        params.oscUnisonCounts[i].store(oscP.unisonCount);
        params.oscPanSpreads[i].store(oscP.panSpread);
        params.oscPans[i].store(oscP.pan);
        params.oscPitchOffsets[i].store(oscP.pitchOffset);
    }

    // Apply sub oscillator
    params.subLevel.store(preset.subOscParams.level);
    params.subOctave.store(preset.subOscParams.octave);
    params.subWaveform.store(preset.subOscParams.waveform);

    // Apply filter
    params.filterCutoff.store(preset.filterParams.cutoff);
    params.filterResonance.store(preset.filterParams.resonance);
    params.filterDrive.store(preset.filterParams.drive);
    params.filterMode.store(preset.filterParams.type);

    // Apply envelope
    params.envAttack.store(preset.ampEnvelope.attack);
    params.envDecay.store(preset.ampEnvelope.decay);
    params.envSustain.store(preset.ampEnvelope.sustain);
    params.envRelease.store(preset.ampEnvelope.release);

    // Apply master
    params.masterLevel.store(preset.masterVolume);

    // Apply LFOs
    for (int i = 0; i < 4 && i < WavetableSynth::numLFOs; ++i)
    {
        const auto& lfoP = preset.lfoParams[i];
        auto& lfo = synth.getLFO(i);
        lfo.setRate(lfoP.rate);
        lfo.setDepth(lfoP.depth);
        lfo.setWaveform(static_cast<LFOModulator::Waveform>(lfoP.waveform));
        lfo.setTempoSync(lfoP.tempoSync);
        // Note: syncRate converted to tempoRateBeats (simplified mapping)
        // syncRate: 0=1/4, 1=1/8, 2=1/16, 3=1/32
        float tempoRates[] = {4.0f, 2.0f, 1.0f, 0.5f};  // In beats
        if (lfoP.syncRate >= 0 && lfoP.syncRate < 4)
            lfo.setTempoRate(tempoRates[lfoP.syncRate]);
    }

    // Apply modulation routings
    auto& modMatrix = synth.getModulationMatrix();
    modMatrix.clearAllRoutings();

    for (const auto& routing : preset.modRoutings)
    {
        modMatrix.addRouting(
            static_cast<ModulationSource>(routing.source),
            static_cast<ModulationTarget>(routing.target),
            routing.amount
        );
    }
}

WavetablePreset WavetablePresetManager::extractPresetFromSynth(WavetableSynth& synth, const juce::String& name)
{
    WavetablePreset preset;
    preset.name = name;

    auto& params = synth.getParams();

    // Extract oscillator parameters
    for (int i = 0; i < 3; ++i)
    {
        preset.oscParams[i].level = params.oscLevels[i].load();
        preset.oscParams[i].morph = params.oscMorphs[i].load();
        preset.oscParams[i].detune = params.oscDetunes[i].load();
        preset.oscParams[i].unisonCount = params.oscUnisonCounts[i].load();
        preset.oscParams[i].panSpread = params.oscPanSpreads[i].load();
        preset.oscParams[i].pan = params.oscPans[i].load();
        preset.oscParams[i].pitchOffset = params.oscPitchOffsets[i].load();
    }

    // Extract sub oscillator
    preset.subOscParams.level = params.subLevel.load();
    preset.subOscParams.octave = params.subOctave.load();
    preset.subOscParams.waveform = params.subWaveform.load();

    // Extract filter
    preset.filterParams.cutoff = params.filterCutoff.load();
    preset.filterParams.resonance = params.filterResonance.load();
    preset.filterParams.drive = params.filterDrive.load();
    preset.filterParams.type = params.filterMode.load();

    // Extract envelope
    preset.ampEnvelope.attack = params.envAttack.load();
    preset.ampEnvelope.decay = params.envDecay.load();
    preset.ampEnvelope.sustain = params.envSustain.load();
    preset.ampEnvelope.release = params.envRelease.load();

    // Extract master
    preset.masterVolume = params.masterLevel.load();

    // Extract LFOs (only available getters)
    for (int i = 0; i < 4 && i < WavetableSynth::numLFOs; ++i)
    {
        const auto& lfo = synth.getLFO(i);
        preset.lfoParams[i].rate = lfo.getRate();
        preset.lfoParams[i].depth = lfo.getDepth();
        preset.lfoParams[i].waveform = static_cast<int>(lfo.getWaveform());
        // Note: tempoSync, syncRate, bipolar not available as getters in LFOModulator
        preset.lfoParams[i].tempoSync = false;
        preset.lfoParams[i].syncRate = 0;
        preset.lfoParams[i].bipolar = true;
    }

    // Extract modulation routings
    const auto& modMatrix = synth.getModulationMatrix();
    int numRoutings = modMatrix.getNumRoutings();
    for (int i = 0; i < numRoutings; ++i)
    {
        const auto& routing = modMatrix.getRouting(i);
        WavetablePreset::ModRouting mr;
        mr.source = static_cast<int>(routing.source);
        mr.target = static_cast<int>(routing.target);
        mr.amount = routing.amount;
        mr.bipolar = routing.bipolar;
        preset.modRoutings.add(mr);
    }

    return preset;
}

juce::String WavetablePresetManager::getPresetNameFromFile(const juce::File& file) const
{
    return file.getFileNameWithoutExtension();
}

bool WavetablePresetManager::deletePreset(const juce::File& file)
{
    if (file.hasFileExtension(PRESET_EXTENSION))
        return file.deleteFile();

    return false;
}

bool WavetablePresetManager::renamePreset(const juce::File& file, const juce::String& newName)
{
    if (!file.existsAsFile())
        return false;

    juce::String safeName = newName
        .replaceCharacter('/', '_')
        .replaceCharacter('\\', '_')
        .replaceCharacter(':', '_')
        .replaceCharacter('*', '_')
        .replaceCharacter('?', '_')
        .replaceCharacter('"', '_')
        .replaceCharacter('<', '_')
        .replaceCharacter('>', '_')
        .replaceCharacter('|', '_');

    juce::File newFile = file.getSiblingFile(safeName + PRESET_EXTENSION);
    return file.moveFileTo(newFile);
}
