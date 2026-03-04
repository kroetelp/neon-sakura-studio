#pragma once

/**
 * DelayLine - Thread-safe delay line for latency compensation
 *
 * Used to align audio from tracks with different plugin latencies.
 * Tracks with less latency are delayed to match the track with the most latency.
 *
 * Real-Time Safety:
 *   - All memory is pre-allocated in prepare()
 *   - No heap allocations in process()
 *   - Uses atomic for thread-safe delay updates
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <algorithm>

class DelayLine
{
public:
    DelayLine() = default;
    ~DelayLine() = default;

    /** Prepare the delay line for operation.
        @param sampleRate The sample rate
        @param maxDelaySamples Maximum delay in samples (determines buffer size)
        @param numChannels Number of channels (default: 2 for stereo)
    */
    void prepare(double sampleRate, int maxDelaySamples, int numChannels = 2)
    {
        currentSampleRate = sampleRate;
        maxDelay = maxDelaySamples;
        channels = numChannels;

        // Allocate circular buffer with extra safety margin
        const int bufferSize = juce::jmax(maxDelaySamples + 1, 4096);
        buffer.setSize(numChannels, bufferSize);
        buffer.clear();

        writePosition = 0;
        currentDelay.store(0);
    }

    /** Set the delay amount in samples.
        Thread-safe - can be called from any thread.
        @param delaySamples Number of samples to delay (0 = no delay)
    */
    void setDelay(int delaySamples)
    {
        const int clampedDelay = juce::jlimit(0, maxDelay, delaySamples);
        currentDelay.store(clampedDelay);
    }

    /** Get the current delay in samples. */
    int getDelay() const
    {
        return currentDelay.load();
    }

    /** Process audio through the delay line.
        This reads delayed samples and writes new samples to the circular buffer.

        @param audio Audio buffer to process (modified in-place)
        @param numSamples Number of samples to process
    */
    void process(juce::AudioBuffer<float>& audio, int numSamples)
    {
        const int delay = currentDelay.load();

        if (delay == 0 || buffer.getNumSamples() == 0)
            return;  // No delay needed

        const int bufferLength = buffer.getNumSamples();
        const int numChannelsToProcess = juce::jmin(audio.getNumChannels(), channels);

        for (int ch = 0; ch < numChannelsToProcess; ++ch)
        {
            auto* channelData = audio.getWritePointer(ch);
            auto* delayData = buffer.getWritePointer(ch);

            int readPos = writePosition - delay;
            if (readPos < 0)
                readPos += bufferLength;

            for (int i = 0; i < numSamples; ++i)
            {
                // Store the input sample
                const float inputSample = channelData[i];

                // Read the delayed sample
                const float delayedSample = delayData[readPos];

                // Write input to delay buffer
                delayData[writePosition] = inputSample;

                // Output the delayed sample
                channelData[i] = delayedSample;

                // Advance positions
                writePosition = (writePosition + 1) % bufferLength;
                readPos = (readPos + 1) % bufferLength;
            }
        }

        // Handle case where audio has more channels than delay buffer
        for (int ch = numChannelsToProcess; ch < audio.getNumChannels(); ++ch)
        {
            // Just clear extra channels (no delay data available)
            audio.clear(ch, 0, numSamples);
        }
    }

    /** Process with a separate output buffer (non-destructive).
        Input is read from inputBuffer, delayed output written to outputBuffer.

        @param inputBuffer Source audio buffer
        @param outputBuffer Destination audio buffer (will contain delayed signal)
        @param numSamples Number of samples to process
    */
    void process(const juce::AudioBuffer<float>& inputBuffer,
                 juce::AudioBuffer<float>& outputBuffer,
                 int numSamples)
    {
        const int delay = currentDelay.load();

        if (delay == 0)
        {
            // No delay - just copy
            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
            {
                if (ch < inputBuffer.getNumChannels())
                {
                    outputBuffer.copyFrom(ch, 0, inputBuffer, ch, 0, numSamples);
                }
                else
                {
                    outputBuffer.clear(ch, 0, numSamples);
                }
            }
            return;
        }

        if (buffer.getNumSamples() == 0)
            return;

        const int bufferLength = buffer.getNumSamples();
        const int numChannelsToProcess = juce::jmin(
            inputBuffer.getNumChannels(),
            juce::jmin(outputBuffer.getNumChannels(), channels));

        for (int ch = 0; ch < numChannelsToProcess; ++ch)
        {
            const auto* inputData = inputBuffer.getReadPointer(ch);
            auto* outputData = outputBuffer.getWritePointer(ch);
            auto* delayData = buffer.getWritePointer(ch);

            int readPos = writePosition - delay;
            if (readPos < 0)
                readPos += bufferLength;

            for (int i = 0; i < numSamples; ++i)
            {
                // Write input to delay buffer
                delayData[writePosition] = inputData[i];

                // Read the delayed sample
                outputData[i] = delayData[readPos];

                // Advance positions
                writePosition = (writePosition + 1) % bufferLength;
                readPos = (readPos + 1) % bufferLength;
            }
        }

        // Clear unused output channels
        for (int ch = numChannelsToProcess; ch < outputBuffer.getNumChannels(); ++ch)
        {
            outputBuffer.clear(ch, 0, numSamples);
        }
    }

    /** Clear the delay buffer. */
    void reset()
    {
        buffer.clear();
        writePosition = 0;
    }

    /** Get the maximum possible delay. */
    int getMaxDelay() const { return maxDelay; }

    /** Check if the delay line is prepared. */
    bool isPrepared() const { return buffer.getNumSamples() > 0; }

private:
    juce::AudioBuffer<float> buffer;
    int writePosition = 0;
    int maxDelay = 0;
    int channels = 2;
    double currentSampleRate = 44100.0;
    std::atomic<int> currentDelay{0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayLine)
};
