#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>
#include <array>
#include <memory>

/**
 * WavetableData - Stores and manages wavetable data for oscillators
 *
 * A wavetable consists of multiple "frames" (single-cycle waveforms)
 * that can be morphed between for evolving timbres.
 *
 * Supported formats:
 * - Standard WAV files (single-cycle detection)
 * - Serum wavetables (256 frames x 2048 samples)
 * - Vital wavetables
 */
class WavetableData
{
public:
    static constexpr int defaultSamplesPerFrame = 2048;
    static constexpr int defaultNumFrames = 256;

    WavetableData();
    ~WavetableData() = default;

    // Initialize with basic waveforms
    void initializeWithBasicWaveforms();

    // Load wavetable from file (WAV, AIFF, FLAC, etc.)
    bool loadFromFile(const juce::File& file);

    // Generate wavetable from a single cycle waveform
    void generateFromSingleCycle(const std::vector<float>& singleCycle, int numFrames = 256);

    // Generate wavetable from multiple frames (e.g., loaded from Serum format)
    void generateFromFrames(const std::vector<std::vector<float>>& loadedFrames);

    // Accessors
    int getNumFrames() const { return static_cast<int>(frames.size()); }
    int getSamplesPerFrame() const { return samplesPerFrame; }

    // Get a sample with linear interpolation between frames
    float getSample(float phase, float morphPosition) const;

    // Get a frame's data for visualization
    const std::vector<float>& getFrame(int frameIndex) const;
    std::vector<float> getInterpolatedFrame(float morphPosition) const;

    // Get the current morphed waveform for display
    void getCurrentWaveform(float morphPosition, std::vector<float>& outWaveform) const;

    // Wavetable name for display
    void setName(const juce::String& name) { wavetableName = name; }
    juce::String getName() const { return wavetableName; }

    // Get the last error message (for UI feedback)
    juce::String getLastError() const { return lastError; }

private:
    // Each frame is a single-cycle waveform
    std::vector<std::vector<float>> frames;
    int samplesPerFrame;

    juce::String wavetableName;
    juce::String lastError;

    // Audio format manager for loading files
    juce::AudioFormatManager formatManager;

    // Basic waveform generators
    static float generateSine(float phase);
    static float generateTriangle(float phase);
    static float generateSaw(float phase);
    static float generateSquare(float phase);
    static void generateWavetableFromBaseWave(std::vector<std::vector<float>>& table,
                                               int numFrames, int samplesPerFrame,
                                               std::function<float(float)> baseWave);

    // WAV loading helpers
    bool loadStandardWav(const juce::File& file);
    bool loadSerumWavetable(const juce::File& file, juce::AudioFormatReader* reader);

    // Find single cycle regions in audio (zero-crossing detection)
    struct CycleRegion {
        int startSample;
        int length;
    };
    std::vector<CycleRegion> detectSingleCycles(const float* samples, int numSamples);

    // Resample a waveform to target length
    std::vector<float> resampleWaveform(const float* input, int inputLength, int outputLength);

    // Create wavetable frames from a single cycle with spectral morphing
    void createFramesWithSpectralMorph(const std::vector<float>& baseCycle, int numFrames);
};
