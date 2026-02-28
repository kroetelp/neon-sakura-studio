#include "WavetableData.h"
#include <cmath>
#include <functional>
#include <algorithm>

WavetableData::WavetableData()
    : samplesPerFrame(defaultSamplesPerFrame)
{
    // Register audio formats for loading
    formatManager.registerBasicFormats();
    initializeWithBasicWaveforms();
}

void WavetableData::initializeWithBasicWaveforms()
{
    frames.clear();
    frames.resize(defaultNumFrames);
    wavetableName = "Basic Waveforms";

    // Generate wavetables for different basic waveforms
    // Frame 0-63: Sine
    // Frame 64-127: Triangle
    // Frame 128-191: Saw
    // Frame 192-255: Square

    for (int frame = 0; frame < defaultNumFrames; ++frame)
    {
        frames[frame].resize(samplesPerFrame);

        // Calculate blend between waveforms
        float t = static_cast<float>(frame) / (defaultNumFrames - 1);

        for (int i = 0; i < samplesPerFrame; ++i)
        {
            float phase = static_cast<float>(i) / samplesPerFrame;

            float sample = 0.0f;

            if (t < 0.33f)
            {
                // Sine to Triangle
                float blend = t / 0.33f;
                sample = generateSine(phase) * (1.0f - blend) + generateTriangle(phase) * blend;
            }
            else if (t < 0.66f)
            {
                // Triangle to Saw
                float blend = (t - 0.33f) / 0.33f;
                sample = generateTriangle(phase) * (1.0f - blend) + generateSaw(phase) * blend;
            }
            else
            {
                // Saw to Square to noise
                float blend = (t - 0.66f) / 0.34f;
                sample = generateSaw(phase) * (1.0f - blend) + generateSquare(phase) * blend;
            }

            frames[frame][i] = sample;
        }
    }
}

bool WavetableData::loadFromFile(const juce::File& file)
{
    lastError.clear();

    if (!file.exists())
    {
        lastError = "File does not exist: " + file.getFullPathName();
        return false;
    }

    // Try to create an audio reader
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        lastError = "Could not read audio file: " + file.getFullPathName();
        return false;
    }

    // Check for Serum wavetable format (256 frames x 2048 samples = 524288 samples)
    // Serum stores all frames in one long WAV file
    if (reader->lengthInSamples >= defaultSamplesPerFrame * 200 &&
        reader->lengthInSamples % defaultSamplesPerFrame == 0)
    {
        if (loadSerumWavetable(file, reader.get()))
        {
            wavetableName = file.getFileNameWithoutExtension();
            return true;
        }
    }

    // Standard WAV loading - try to detect single cycles
    return loadStandardWav(file);
}

bool WavetableData::loadStandardWav(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

    if (reader == nullptr)
    {
        lastError = "Could not create reader for: " + file.getFullPathName();
        return false;
    }

    // Read the entire audio file
    juce::AudioBuffer<float> buffer;
    const int totalSamples = static_cast<int>(reader->lengthInSamples);
    buffer.setSize(1, totalSamples);
    reader->read(&buffer, 0, totalSamples, 0, true, false);

    const float* samples = buffer.getReadPointer(0);

    // Detect single cycles (zero-crossing regions)
    auto cycles = detectSingleCycles(samples, totalSamples);

    if (cycles.empty())
    {
        lastError = "Could not detect any single cycle waveforms in the file";
        return false;
    }

    // Find the most stable/representative cycle
    // Use the cycle closest to the middle of the file for stability
    int bestCycleIndex = static_cast<int>(cycles.size() / 2);
    const auto& bestCycle = cycles[bestCycleIndex];

    // Extract the single cycle
    std::vector<float> singleCycle(bestCycle.length);
    for (int i = 0; i < bestCycle.length; ++i)
    {
        singleCycle[i] = samples[bestCycle.startSample + i];
    }

    // Generate wavetable from this single cycle
    createFramesWithSpectralMorph(singleCycle, defaultNumFrames);

    wavetableName = file.getFileNameWithoutExtension();
    lastError.clear();
    return true;
}

bool WavetableData::loadSerumWavetable(const juce::File& file, juce::AudioFormatReader* reader)
{
    const juce::int64 totalSamples = reader->lengthInSamples;
    const int numFramesDetected = static_cast<int>(totalSamples / defaultSamplesPerFrame);

    // Sanity check - Serum wavetables typically have 256 frames
    if (numFramesDetected < 16 || numFramesDetected > 512)
        return false;

    // Read the entire wavetable
    juce::AudioBuffer<float> buffer;
    buffer.setSize(1, static_cast<int>(totalSamples));
    reader->read(&buffer, 0, static_cast<int>(totalSamples), 0, true, false);

    const float* samples = buffer.getReadPointer(0);

    // Extract each frame
    frames.clear();
    frames.resize(numFramesDetected);
    samplesPerFrame = defaultSamplesPerFrame;

    for (int frame = 0; frame < numFramesDetected; ++frame)
    {
        frames[frame].resize(samplesPerFrame);
        const int frameStart = frame * samplesPerFrame;

        for (int i = 0; i < samplesPerFrame; ++i)
        {
            frames[frame][i] = samples[frameStart + i];
        }
    }

    return true;
}

std::vector<WavetableData::CycleRegion> WavetableData::detectSingleCycles(const float* samples, int numSamples)
{
    std::vector<CycleRegion> cycles;

    if (numSamples < 100)
        return cycles;

    // Find zero-crossings (positive to negative transitions)
    std::vector<int> zeroCrossings;

    for (int i = 1; i < numSamples; ++i)
    {
        if (samples[i - 1] >= 0.0f && samples[i] < 0.0f)
        {
            // Found a positive-to-negative zero crossing
            zeroCrossings.push_back(i);
        }
    }

    if (zeroCrossings.size() < 2)
        return cycles;

    // Analyze intervals between crossings to find consistent periods
    // A single cycle spans from one crossing to the next similar crossing
    std::vector<int> intervals;
    for (size_t i = 1; i < zeroCrossings.size(); ++i)
    {
        intervals.push_back(zeroCrossings[i] - zeroCrossings[i - 1]);
    }

    // Find the fundamental period by looking for repeating interval patterns
    // Group consecutive similar intervals
    const int tolerance = 10; // samples
    int cycleStart = zeroCrossings[0];
    int cycleLength = intervals[0];

    for (size_t i = 1; i < intervals.size(); ++i)
    {
        // Check if this interval is similar to the previous one
        if (std::abs(intervals[i] - cycleLength) <= tolerance)
        {
            // Similar - extend the cycle
            cycleLength += intervals[i];
        }
        else
        {
            // Different interval - save current cycle if valid
            if (cycleLength >= 64 && cycleLength <= 8192)
            {
                cycles.push_back({cycleStart, cycleLength});
            }

            // Start new cycle
            cycleStart = zeroCrossings[i];
            cycleLength = intervals[i];
        }
    }

    // Don't forget the last cycle
    if (cycleLength >= 64 && cycleLength <= 8192)
    {
        cycles.push_back({cycleStart, cycleLength});
    }

    // If no cycles found with the simple method, try a different approach
    // Use autocorrelation-like detection
    if (cycles.empty())
    {
        // Find the most common interval (fundamental frequency)
        std::vector<int> histogram(8192, 0);
        for (int interval : intervals)
        {
            if (interval >= 16 && interval < 8192)
                histogram[interval]++;
        }

        int mostCommonInterval = 0;
        int maxCount = 0;
        for (size_t i = 0; i < histogram.size(); ++i)
        {
            if (histogram[i] > maxCount)
            {
                maxCount = histogram[i];
                mostCommonInterval = static_cast<int>(i);
            }
        }

        if (mostCommonInterval > 0)
        {
            // Create cycles with this interval
            for (size_t i = 0; i + 1 < zeroCrossings.size(); ++i)
            {
                int start = zeroCrossings[i];
                int end = start + mostCommonInterval;

                if (end < numSamples)
                {
                    cycles.push_back({start, mostCommonInterval});
                }
            }
        }
    }

    return cycles;
}

std::vector<float> WavetableData::resampleWaveform(const float* input, int inputLength, int outputLength)
{
    std::vector<float> output(outputLength);

    if (inputLength <= 0 || outputLength <= 0)
        return output;

    const double ratio = static_cast<double>(inputLength) / outputLength;

    for (int i = 0; i < outputLength; ++i)
    {
        // Linear interpolation
        double srcPos = i * ratio;
        int srcIndex = static_cast<int>(srcPos);
        double frac = srcPos - srcIndex;

        if (srcIndex + 1 < inputLength)
        {
            output[i] = static_cast<float>(
                input[srcIndex] * (1.0 - frac) +
                input[srcIndex + 1] * frac
            );
        }
        else if (srcIndex < inputLength)
        {
            output[i] = input[srcIndex];
        }
        else
        {
            // Wrap around for seamless loops
            srcIndex = srcIndex % inputLength;
            int nextIndex = (srcIndex + 1) % inputLength;
            output[i] = static_cast<float>(
                input[srcIndex] * (1.0 - frac) +
                input[nextIndex] * frac
            );
        }
    }

    return output;
}

void WavetableData::generateFromFrames(const std::vector<std::vector<float>>& loadedFrames)
{
    if (loadedFrames.empty())
        return;

    frames = loadedFrames;
    samplesPerFrame = static_cast<int>(loadedFrames[0].size());
}

void WavetableData::createFramesWithSpectralMorph(const std::vector<float>& baseCycle, int numFrames)
{
    if (baseCycle.empty())
        return;

    frames.clear();
    frames.resize(numFrames);
    samplesPerFrame = defaultSamplesPerFrame;

    // Resample base cycle to our standard size
    std::vector<float> resampled = resampleWaveform(
        baseCycle.data(),
        static_cast<int>(baseCycle.size()),
        samplesPerFrame
    );

    // Create frames with spectral morphing (brightness/harmonic content changes)
    for (int frame = 0; frame < numFrames; ++frame)
    {
        frames[frame].resize(samplesPerFrame);

        // Morph parameter: 0 to 1 across frames
        float t = static_cast<float>(frame) / (numFrames - 1);

        // Create different spectral variations
        // Frame 0: Dark (low-passed)
        // Frame 0.5: Original
        // Frame 1: Bright (emphasized harmonics)

        for (int i = 0; i < samplesPerFrame; ++i)
        {
            float sample = resampled[i];

            // Apply simple spectral shaping using multiple passes
            if (t < 0.33f)
            {
                // Darken: Simple moving average smoothing
                float blend = t / 0.33f;
                int smoothWidth = static_cast<int>(5.0f * (1.0f - blend) + 1);

                float sum = 0.0f;
                int count = 0;
                for (int j = -smoothWidth; j <= smoothWidth; ++j)
                {
                    int idx = (i + j + samplesPerFrame) % samplesPerFrame;
                    sum += resampled[idx];
                    count++;
                }
                sample = sum / count;
            }
            else if (t > 0.66f)
            {
                // Brighten: Add some harmonic content
                float blend = (t - 0.66f) / 0.34f;
                float phase = static_cast<float>(i) / samplesPerFrame;

                // Add second harmonic (octave)
                sample = resampled[i] * (1.0f - blend * 0.3f);
                sample += std::sin(phase * 2.0f * juce::MathConstants<float>::pi * 2.0f)
                         * blend * 0.2f * std::abs(resampled[i]);
            }
            else
            {
                // Original with slight variation
                sample = resampled[i];
            }

            frames[frame][i] = sample;
        }
    }
}

void WavetableData::generateFromSingleCycle(const std::vector<float>& singleCycle, int numFrames)
{
    if (singleCycle.empty())
        return;

    frames.clear();
    frames.resize(numFrames);

    // Use the single cycle as the base
    samplesPerFrame = static_cast<int>(singleCycle.size());

    for (int frame = 0; frame < numFrames; ++frame)
    {
        frames[frame].resize(samplesPerFrame);

        float morphT = static_cast<float>(frame) / (numFrames - 1);

        for (int i = 0; i < samplesPerFrame; ++i)
        {
            // For a single cycle, we just copy it
            // More sophisticated generation could add harmonics
            float baseSample = singleCycle[i];

            // Add some variation based on frame
            float phase = static_cast<float>(i) / samplesPerFrame;
            float brightness = 1.0f - morphT * 0.5f;  // Dim harmonics as we progress

            frames[frame][i] = baseSample * brightness;
        }
    }
}

float WavetableData::getSample(float phase, float morphPosition) const
{
    if (frames.empty())
        return 0.0f;

    // Clamp morph position
    morphPosition = juce::jlimit(0.0f, 1.0f, morphPosition);

    // Wrap phase
    phase = phase - std::floor(phase);

    // Calculate frame indices for interpolation
    float framePos = morphPosition * (frames.size() - 1);
    int frame0 = static_cast<int>(framePos);
    int frame1 = std::min(frame0 + 1, static_cast<int>(frames.size() - 1));
    float frameAlpha = framePos - frame0;

    // Calculate sample indices for interpolation
    float samplePos = phase * samplesPerFrame;
    int sample0 = static_cast<int>(samplePos);
    int sample1 = (sample0 + 1) % samplesPerFrame;
    float sampleAlpha = samplePos - sample0;

    // Bilinear interpolation
    float s00 = frames[frame0][sample0];
    float s01 = frames[frame0][sample1];
    float s10 = frames[frame1][sample0];
    float s11 = frames[frame1][sample1];

    float sample0Interp = s00 * (1.0f - sampleAlpha) + s01 * sampleAlpha;
    float sample1Interp = s10 * (1.0f - sampleAlpha) + s11 * sampleAlpha;

    return sample0Interp * (1.0f - frameAlpha) + sample1Interp * frameAlpha;
}

const std::vector<float>& WavetableData::getFrame(int frameIndex) const
{
    static std::vector<float> empty;
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size()))
        return empty;
    return frames[frameIndex];
}

std::vector<float> WavetableData::getInterpolatedFrame(float morphPosition) const
{
    std::vector<float> result(samplesPerFrame);

    if (frames.empty())
        return result;

    morphPosition = juce::jlimit(0.0f, 1.0f, morphPosition);
    float framePos = morphPosition * (frames.size() - 1);
    int frame0 = static_cast<int>(framePos);
    int frame1 = std::min(frame0 + 1, static_cast<int>(frames.size() - 1));
    float frameAlpha = framePos - frame0;

    for (int i = 0; i < samplesPerFrame; ++i)
    {
        result[i] = frames[frame0][i] * (1.0f - frameAlpha) + frames[frame1][i] * frameAlpha;
    }

    return result;
}

void WavetableData::getCurrentWaveform(float morphPosition, std::vector<float>& outWaveform) const
{
    outWaveform = getInterpolatedFrame(morphPosition);
}

float WavetableData::generateSine(float phase)
{
    return std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
}

float WavetableData::generateTriangle(float phase)
{
    // Triangle wave
    float t = phase * 4.0f;
    if (t < 2.0f)
        return t - 1.0f;
    else
        return 3.0f - t;
}

float WavetableData::generateSaw(float phase)
{
    // Sawtooth wave
    return 2.0f * phase - 1.0f;
}

float WavetableData::generateSquare(float phase)
{
    // Square wave
    return (phase < 0.5f) ? 1.0f : -1.0f;
}

void WavetableData::generateWavetableFromBaseWave(std::vector<std::vector<float>>& table,
                                                   int numFrames, int samplesPerFrame,
                                                   std::function<float(float)> baseWave)
{
    table.resize(numFrames);
    for (int frame = 0; frame < numFrames; ++frame)
    {
        table[frame].resize(samplesPerFrame);
        for (int i = 0; i < samplesPerFrame; ++i)
        {
            float phase = static_cast<float>(i) / samplesPerFrame;
            table[frame][i] = baseWave(phase);
        }
    }
}

// ============================================================
// Wavetable Editing Methods
// ============================================================

void WavetableData::setSample(int frameIndex, int sampleIndex, float value)
{
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size()) &&
        sampleIndex >= 0 && sampleIndex < samplesPerFrame)
    {
        frames[frameIndex][sampleIndex] = juce::jlimit(-1.0f, 1.0f, value);
    }
}

void WavetableData::setFrame(int frameIndex, const std::vector<float>& waveform)
{
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size()) &&
        waveform.size() == static_cast<size_t>(samplesPerFrame))
    {
        frames[frameIndex] = waveform;
    }
}

void WavetableData::setCurrentEditFrame(const std::vector<float>& waveform)
{
    if (!frames.empty() && waveform.size() == static_cast<size_t>(samplesPerFrame))
    {
        frames[0] = waveform;  // Edit frame 0 by default
    }
}

std::vector<float>& WavetableData::getCurrentEditFrame()
{
    static std::vector<float> empty;
    if (frames.empty())
        return empty;
    return frames[0];  // Edit frame 0 by default
}

const std::vector<float>& WavetableData::getCurrentEditFrame() const
{
    static std::vector<float> empty;
    if (frames.empty())
        return empty;
    return frames[0];
}

void WavetableData::createEmptyWavetable(int numFrames, int samples)
{
    frames.clear();
    frames.resize(numFrames);
    samplesPerFrame = samples;

    for (auto& frame : frames)
    {
        frame.resize(samplesPerFrame, 0.0f);
    }

    wavetableName = "Custom Drawing";
}

void WavetableData::generateAllFramesFromDrawing(const std::vector<float>& drawnWaveform)
{
    if (drawnWaveform.empty())
        return;

    // Resample to our frame size if needed
    std::vector<float> resampled;
    if (static_cast<int>(drawnWaveform.size()) != samplesPerFrame)
    {
        resampled = resampleWaveform(drawnWaveform.data(),
                                      static_cast<int>(drawnWaveform.size()),
                                      samplesPerFrame);
    }
    else
    {
        resampled = drawnWaveform;
    }

    // Use spectral morphing to generate all frames
    createFramesWithSpectralMorph(resampled, static_cast<int>(frames.size()));

    wavetableName = "Custom Drawing";
}

void WavetableData::interpolateDrawnPoints(std::vector<float>& waveform,
                                             const std::vector<std::pair<int, float>>& points)
{
    if (points.empty() || waveform.empty())
        return;

    // Sort points by index
    auto sortedPoints = points;
    std::sort(sortedPoints.begin(), sortedPoints.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    int size = static_cast<int>(waveform.size());

    // Interpolate between points
    for (size_t i = 0; i < sortedPoints.size(); ++i)
    {
        int idx0 = sortedPoints[i].first;
        float val0 = sortedPoints[i].second;

        int idx1;
        float val1;

        if (i + 1 < sortedPoints.size())
        {
            idx1 = sortedPoints[i + 1].first;
            val1 = sortedPoints[i + 1].second;
        }
        else
        {
            // Wrap around to first point for seamless loop
            idx1 = sortedPoints[0].first + size;
            val1 = sortedPoints[0].second;
        }

        // Linear interpolation between points
        for (int j = idx0; j < idx1 && j < size; ++j)
        {
            float t = static_cast<float>(j - idx0) / (idx1 - idx0);
            int actualIdx = j % size;
            waveform[actualIdx] = val0 * (1.0f - t) + val1 * t;
        }
    }
}
