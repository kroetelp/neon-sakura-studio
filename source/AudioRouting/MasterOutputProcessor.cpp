#include "MasterOutputProcessor.h"

//==============================================================================
MasterOutputProcessor::MasterOutputProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

//==============================================================================
MasterOutputProcessor::~MasterOutputProcessor()
{
}

//==============================================================================
void MasterOutputProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    // Initialize reverb
    reverb.setSampleRate(sampleRate);
    updateReverbParams();

    // Prepare DC filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    *dcFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);
    dcFilter.prepare(spec);
}

//==============================================================================
void MasterOutputProcessor::releaseResources()
{
}

//==============================================================================
void MasterOutputProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const float masterVol = masterVolume.load();

    // Apply master volume
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.applyGain(channel, 0, buffer.getNumSamples(), masterVol);
    }

    // Update reverb parameters if changed
    updateReverbParams();

    // Apply reverb if wet level > 0
    if (reverbWetLevel.load() > 0.001f && buffer.getNumChannels() >= 2)
    {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);
        reverb.processStereo(left, right, buffer.getNumSamples());
    }

    // Apply soft clipping / limiting for protection
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            samples[i] = softClip(samples[i]);
        }
    }

    // Update peak metering
    float leftPeak = buffer.getMagnitude(0, buffer.getNumSamples());
    float rightPeak = buffer.getNumChannels() > 1
        ? buffer.getMagnitude(1, buffer.getNumSamples())
        : leftPeak;

    peakLeft.store(leftPeak);
    peakRight.store(rightPeak);
    peakLevel.store(juce::jmax(leftPeak, rightPeak));
}

//==============================================================================
void MasterOutputProcessor::processBlock(juce::AudioBuffer<double>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Convert to float, process, convert back
    juce::AudioBuffer<float> floatBuffer(buffer.getNumChannels(), buffer.getNumSamples());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* src = buffer.getReadPointer(channel);
        auto* dst = floatBuffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            dst[sample] = static_cast<float>(src[sample]);
    }

    processBlock(floatBuffer, midiMessages);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* src = floatBuffer.getReadPointer(channel);
        auto* dst = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            dst[sample] = static_cast<double>(src[sample]);
    }
}

//==============================================================================
void MasterOutputProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("MASTER_OUTPUT");
    xml->setAttribute("masterVolume", masterVolume.load());
    xml->setAttribute("reverbWet", reverbWetLevel.load());
    xml->setAttribute("reverbRoom", reverbRoomSize.load());
    xml->setAttribute("reverbDamping", reverbDamping.load());

    copyXmlToBinary(*xml, destData);
}

//==============================================================================
void MasterOutputProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr)
    {
        masterVolume.store((float)xml->getDoubleAttribute("masterVolume", 0.8));
        reverbWetLevel.store((float)xml->getDoubleAttribute("reverbWet", 0.0));
        reverbRoomSize.store((float)xml->getDoubleAttribute("reverbRoom", 0.5));
        reverbDamping.store((float)xml->getDoubleAttribute("reverbDamping", 0.5));
    }
}

//==============================================================================
void MasterOutputProcessor::setMasterVolume(float volume)
{
    masterVolume.store(juce::jlimit(0.0f, 1.0f, volume));
}

//==============================================================================
void MasterOutputProcessor::setReverbWetLevel(float wetLevel)
{
    reverbWetLevel.store(juce::jlimit(0.0f, 1.0f, wetLevel));
}

//==============================================================================
void MasterOutputProcessor::setReverbRoomSize(float roomSize)
{
    reverbRoomSize.store(juce::jlimit(0.0f, 1.0f, roomSize));
}

//==============================================================================
void MasterOutputProcessor::setReverbDamping(float damping)
{
    reverbDamping.store(juce::jlimit(0.0f, 1.0f, damping));
}

//==============================================================================
void MasterOutputProcessor::updateReverbParams()
{
    reverbParams.roomSize = reverbRoomSize.load();
    reverbParams.damping = reverbDamping.load();
    reverbParams.wetLevel = reverbWetLevel.load();
    reverbParams.dryLevel = 1.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
}

//==============================================================================
float MasterOutputProcessor::softClip(float sample)
{
    // Tanh-based soft clipper for gentle limiting
    return std::tanh(sample * 1.5f);
}
