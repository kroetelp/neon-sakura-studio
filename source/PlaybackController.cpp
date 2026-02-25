#include "PlaybackController.h"

PlaybackController::PlaybackController()
{
}

void PlaybackController::startPlayback()
{
    playing.store(true);

    if (onPlaybackStateChanged)
        onPlaybackStateChanged(true);
}

void PlaybackController::stopPlayback()
{
    playing.store(false);
    samplePosition.store(0);

    if (onPlaybackStateChanged)
        onPlaybackStateChanged(false);

    if (onReset)
        onReset();
}

void PlaybackController::resetPlaybackPosition()
{
    samplePosition.store(0);

    if (onReset)
        onReset();
}

void PlaybackController::setBPM(double newBpm)
{
    bpm.store(newBpm);
    calculateSamplesPerStep();

    if (onBpmChanged)
        onBpmChanged(newBpm);
}

void PlaybackController::setSwingAmount(float swing)
{
    swingAmount.store(swing);
}

void PlaybackController::setLoopLength(int steps)
{
    loopLength.store(steps);
}

void PlaybackController::setMasterVolume(float volume)
{
    masterVolume.store(volume);
}

void PlaybackController::setReverbWetLevel(float wetLevel)
{
    reverbWetLevel.store(wetLevel);
}

void PlaybackController::advancePosition(uint64_t samples)
{
    samplePosition.store(samplePosition.load() + samples);
}

void PlaybackController::updateSamplesPerStep(int samples)
{
    samplesPerStep.store(samples);
}

void PlaybackController::setSamplePosition(uint64_t position)
{
    samplePosition.store(position);
}

void PlaybackController::setSampleRate(double rate)
{
    sampleRate.store(rate);
    calculateSamplesPerStep();
}

void PlaybackController::calculateSamplesPerStep()
{
    const double bpmValue = bpm.load();
    const double rate = sampleRate.load();
    const double secondsPerBeat = 60.0 / bpmValue;
    const double secondsPerStep = secondsPerBeat / 4.0;  // 16th notes
    samplesPerStep.store(static_cast<int>(rate * secondsPerStep));
}
