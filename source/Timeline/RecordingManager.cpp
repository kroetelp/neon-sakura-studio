#include "RecordingManager.h"

RecordingManager::RecordingManager(TimelineData& data)
    : timelineData(data)
{
}

void RecordingManager::startRecording(int trackIndex, RecordingSource source)
{
    if (state.load() != RecordingState::Idle)
        return;

    recordingTrackIndex.store(trackIndex);
    currentSource.store(source);
    recordingStartBeat.store(timelineData.playheadBeat.load());
    recordedSamples = 0;
    recordingMidiNotes.clear();

    // Pre-allocate recording buffer (start with 30 seconds capacity)
    recordingAudioBuffer = std::make_unique<juce::AudioBuffer<float>>(2, static_cast<int>(44100.0 * 30.0));
    recordingAudioBuffer->clear();

    state.store(RecordingState::Recording);
}

std::unique_ptr<TimelineClip> RecordingManager::stopRecording()
{
    if (state.load() != RecordingState::Recording)
        return nullptr;

    state.store(RecordingState::Stopping);

    // Create clip from recorded data
    auto clip = std::make_unique<TimelineClip>(currentSource.load() == RecordingSource::AudioInput
        ? TimelineClip::Type::Audio
        : TimelineClip::Type::Midi);

    clip->startBeat.store(recordingStartBeat.load());

    if (currentSource.load() == RecordingSource::AudioInput && recordingAudioBuffer)
    {
        // Trim buffer to actual recorded length
        int recordedSamplesInt = static_cast<int>(recordedSamples);
        auto trimmedBuffer = std::make_shared<juce::AudioBuffer<float>>(
            recordingAudioBuffer->getNumChannels(),
            recordedSamplesInt);

        for (int ch = 0; ch < trimmedBuffer->getNumChannels(); ++ch)
        {
            trimmedBuffer->copyFrom(ch, 0, *recordingAudioBuffer, ch, 0, recordedSamplesInt);
        }

        clip->setAudioBuffer(trimmedBuffer, recordingSampleRate);

        // Calculate length in beats
        double lengthSeconds = static_cast<double>(recordedSamples) / recordingSampleRate;
        double lengthBeats = timelineData.secondsToBeats(lengthSeconds);
        clip->lengthBeats.store(lengthBeats);

        clip->name = "Audio Recording";
    }
    else if (currentSource.load() == RecordingSource::WavetableSynth)
    {
        // Copy MIDI notes
        for (const auto& note : recordingMidiNotes)
        {
            clip->addMidiNote(note);
        }

        // Calculate length from last note
        double maxEndBeat = 0.0;
        for (const auto& note : recordingMidiNotes)
        {
            double noteEnd = note.startBeat + note.lengthBeats;
            if (noteEnd > maxEndBeat)
                maxEndBeat = noteEnd;
        }
        clip->lengthBeats.store(std::max(maxEndBeat, 1.0));

        clip->name = "MIDI Recording";
    }

    // Clean up
    recordingAudioBuffer.reset();
    recordingMidiNotes.clear();
    recordingTrackIndex.store(-1);
    state.store(RecordingState::Idle);

    return clip;
}

void RecordingManager::processRecording(const juce::AudioBuffer<float>& inputBuffer,
                                         const juce::AudioBuffer<float>& synthBuffer,
                                         const juce::MidiBuffer& midiMessages,
                                         double sampleRate,
                                         double currentBeat)
{
    if (state.load() != RecordingState::Recording)
        return;

    recordingSampleRate = sampleRate;
    int numSamples = inputBuffer.getNumSamples();

    if (currentSource.load() == RecordingSource::AudioInput)
    {
        // Ensure buffer capacity
        ensureBufferCapacity(static_cast<int>(recordedSamples) + numSamples);

        // Copy input audio to recording buffer
        int numChannels = std::min(inputBuffer.getNumChannels(), recordingAudioBuffer->getNumChannels());
        for (int ch = 0; ch < numChannels; ++ch)
        {
            recordingAudioBuffer->copyFrom(ch, static_cast<int>(recordedSamples),
                                           inputBuffer, ch, 0, numSamples);
        }

        // Add metronome click if enabled
        if (metronomeEnabled.load())
        {
            // Check if we crossed a beat boundary
            double beatsPerSecond = timelineData.getBPM() / 60.0;
            double samplesPerBeat = sampleRate / beatsPerSecond;
            double samplesSinceLastBeat = std::fmod(recordedSamples, samplesPerBeat);

            if (samplesSinceLastBeat + numSamples >= samplesPerBeat)
            {
                int clickSample = static_cast<int>(samplesPerBeat - samplesSinceLastBeat);
                generateMetronomeClick(*recordingAudioBuffer,
                                       static_cast<int>(recordedSamples) + clickSample,
                                       static_cast<int>(sampleRate * 0.05)); // 50ms click
            }
        }

        recordedSamples += numSamples;
    }
    else if (currentSource.load() == RecordingSource::WavetableSynth)
    {
        // Record MIDI messages
        double recordingStart = recordingStartBeat.load();

        for (const auto metadata : midiMessages)
        {
            const auto& msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                // Calculate sample position relative to recording start
                double noteStartBeat = currentBeat -
                    (static_cast<double>(numSamples - metadata.samplePosition) / sampleRate) *
                    (timelineData.getBPM() / 60.0);
                noteStartBeat -= recordingStart;

                TimelineClip::MidiNote note;
                note.noteNumber = msg.getNoteNumber();
                note.velocity = msg.getVelocity() / 127.0f;
                note.startBeat = noteStartBeat;
                note.lengthBeats = 1.0; // Will be updated when note-off received
                note.channel = msg.getChannel();

                recordingMidiNotes.push_back(note);
            }
            else if (msg.isNoteOff())
            {
                // Find matching note-on and update length
                double noteEndBeat = currentBeat -
                    (static_cast<double>(numSamples - metadata.samplePosition) / sampleRate) *
                    (timelineData.getBPM() / 60.0);
                noteEndBeat -= recordingStart;

                for (auto& note : recordingMidiNotes)
                {
                    if (note.noteNumber == msg.getNoteNumber() &&
                        note.channel == msg.getChannel() &&
                        note.lengthBeats == 1.0) // Unmatched note
                    {
                        note.lengthBeats = noteEndBeat - note.startBeat;
                        break;
                    }
                }
            }
        }
    }
}

void RecordingManager::ensureBufferCapacity(int samplesNeeded)
{
    if (!recordingAudioBuffer)
        return;

    if (samplesNeeded > recordingAudioBuffer->getNumSamples())
    {
        // Grow buffer by 50%
        int newSize = static_cast<int>(samplesNeeded * 1.5);
        auto newBuffer = std::make_unique<juce::AudioBuffer<float>>(
            recordingAudioBuffer->getNumChannels(), newSize);
        newBuffer->clear();

        // Copy existing data
        for (int ch = 0; ch < recordingAudioBuffer->getNumChannels(); ++ch)
        {
            newBuffer->copyFrom(ch, 0, *recordingAudioBuffer, ch, 0,
                               recordingAudioBuffer->getNumSamples());
        }

        recordingAudioBuffer = std::move(newBuffer);
    }
}

void RecordingManager::generateMetronomeClick(juce::AudioBuffer<float>& buffer,
                                               int startSample, int numSamples)
{
    // Simple sine wave click
    float frequency = 1000.0f; // 1kHz click
    float sampleRate = static_cast<float>(recordingSampleRate);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (startSample + i < buffer.getNumSamples())
            {
                float envelope = 1.0f - static_cast<float>(i) / numSamples;
                float sample = std::sin(2.0f * juce::MathConstants<float>::pi * frequency *
                                        static_cast<float>(i) / sampleRate) * envelope * 0.3f;
                buffer.addSample(ch, startSample + i, sample);
            }
        }
    }
}
