#include "TrackManager.h"

TrackManager::TrackManager(juce::AudioFormatManager& formatManager)
    : formatManager(formatManager)
{
    // Create track components
    for (int i = 0; i < numTracks; ++i)
    {
        tracks[i] = std::make_unique<TrackComponent>(i, formatManager);
    }
}

// ITrackDataProvider Implementation
juce::Synthesiser& TrackManager::getSynthesiser(int trackIndex)
{
    return tracks[trackIndex]->getSynthesiser();
}

WavetableSynth* TrackManager::getWavetableSynth(int trackIndex)
{
    return tracks[trackIndex]->getWavetableSynth();
}

bool TrackManager::isStepActive(int trackIndex, int step) const
{
    if (!isValidTrackIndex(trackIndex))
        return false;
    return tracks[trackIndex]->isStepActive(step);
}

StepModifierState TrackManager::getStepState(int trackIndex, int step) const
{
    if (!isValidTrackIndex(trackIndex))
        return StepModifierState{};
    return tracks[trackIndex]->getStepState(step);
}

float TrackManager::getVolume(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return 0.0f;
    return tracks[trackIndex]->getVolume();
}

int TrackManager::getPitch(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return 0;
    return tracks[trackIndex]->getPitch();
}

float TrackManager::getCutoff(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return 20000.0f;
    return tracks[trackIndex]->getCutoff();
}

bool TrackManager::getMuted(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return true;
    return tracks[trackIndex]->getMuted();
}

bool TrackManager::getSolo(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return false;
    return tracks[trackIndex]->getSolo();
}

bool TrackManager::getWavetableModulationEnabled(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return false;
    return tracks[trackIndex]->getWavetableModulationEnabled();
}

int TrackManager::getTrackLoopLength(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return 16;
    return tracks[trackIndex]->getTrackLoopLength();
}

TrackType TrackManager::getTrackType(int trackIndex) const
{
    if (!isValidTrackIndex(trackIndex))
        return TrackType::Sampler;
    return tracks[trackIndex]->getTrackType();
}

void TrackManager::processAudioBlock(int trackIndex, juce::AudioBuffer<float>& buffer)
{
    if (isValidTrackIndex(trackIndex))
    {
        tracks[trackIndex]->processAudioBlock(buffer);
    }
}

// Track array access
TrackComponent& TrackManager::getTrack(int index)
{
    return *tracks[index];
}

const TrackComponent& TrackManager::getTrack(int index) const
{
    return *tracks[index];
}

// Track operations
void TrackManager::clearAllTracks()
{
    for (int i = 0; i < numTracks; ++i)
    {
        tracks[i]->clearAllSteps();
        tracks[i]->setMuted(false);
        tracks[i]->setSolo(false);
    }
}

void TrackManager::prepareAudio(double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < numTracks; ++i)
    {
        tracks[i]->getSynthesiser().setCurrentPlaybackSampleRate(sampleRate);
        tracks[i]->prepareAudio(sampleRate, samplesPerBlock);
    }
}
