#include "TrackAudioProcessor.h"
#include "TrackModel.h"

TrackAudioProcessor::TrackAudioProcessor(juce::AudioFormatManager& formatManager_, TrackModel& model_)
    : formatManager(formatManager_), model(model_)
{
    // Add more sampler voices to prevent voice stealing during rapid triggers
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new CustomSamplerVoice());
}

TrackAudioProcessor::~TrackAudioProcessor()
{
}

void TrackAudioProcessor::prepareAudio(double sampleRate, int samplesPerBlock)
{
    // Prepare low-pass filter with current cutoff frequency
    auto spec = juce::dsp::ProcessSpec();
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    lowPassFilter.prepare(spec);

    // Set initial filter coefficients (Low-Pass filter)
    const float cutoffFreq = cutoff.load();
    *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutoffFreq);

    // Store sample rate for filter coefficient updates
    currentSampleRate = sampleRate;
}

void TrackAudioProcessor::processAudioBlock(juce::AudioBuffer<float>& buffer)
{
    const float cutoffFreq = cutoff.load();

    // Only update filter coefficients if cutoff actually changed
    if (cutoffFreq != lastCutoff)
    {
        *lowPassFilter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            currentSampleRate, cutoffFreq);
        lastCutoff = cutoffFreq;
    }

    // Apply low-pass filter
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPassFilter.process(context);
}

void TrackAudioProcessor::loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory)
{
    currentCategory = category;
    juce::File categoryDir = sampleDirectory.getChildFile(category);

    if (!categoryDir.exists())
        return;

    // Find all .wav files in directory and store them in model
    juce::Array<juce::File> sampleFiles;
    categoryDir.findChildFiles(sampleFiles, juce::File::findFiles, false, "*.wav");

    if (sampleFiles.isEmpty())
        return;

    model.setCurrentSampleFiles(sampleFiles);
    model.setCurrentSampleIndex(0);

    // Load first sample
    loadSampleAtIndex(0);
}

void TrackAudioProcessor::loadSampleAtIndex(int index)
{
    const auto& sampleFiles = model.getSampleFiles();
    if (index < 0 || index >= sampleFiles.size())
        return;

    model.setCurrentSampleIndex(index);
    juce::File sampleFile = sampleFiles[index];

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));

    if (!reader)
        return;

    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);

    // Remove old sounds and add new one with generous ADSR envelope
    juce::ScopedLock lock(synthLock);
    synth.clearSounds();

    const float attackTime = attack.load();
    const float decayTime = decay.load();
    synth.addSound(new juce::SamplerSound(currentCategory, *reader, allNotes, 60, attackTime, decayTime, 1.0));
}

void TrackAudioProcessor::setAttack(float atk)
{
    attack.store(atk);
    updateDecayEnvelope();
}

void TrackAudioProcessor::setDecay(float dec)
{
    decay.store(dec);
    updateDecayEnvelope();
}

void TrackAudioProcessor::setCutoff(float cut)
{
    cutoff.store(cut);
}

void TrackAudioProcessor::updateDecayEnvelope()
{
    juce::ScopedLock lock(synthLock);

    // Note: In JUCE 8, SamplerSound doesn't have a direct setADSR method
    // The ADSR is set at creation time, so we store values for future sample loads
    // Values will be applied when next sample is loaded
}
