#pragma once

/**
 * CPUProfiler - Performance-Metriken sichtbar machen
 *
 * Phase 6.3: CPU Profiling (Vereinfachte Version)
 *
 * Diese Komponente überwacht die CPU-Auslastung von:
 *   - Per-Track Prozessierung
 *   - Per-Plugin Ausführung
 *   - Globaler Audio-Engine Last
 *
 * Features:
 *   - Real-Time CPU Messung
 *   - Historische Daten
 *   - Warnungen bei hoher Auslastung
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <chrono>
#include <atomic>
#include <array>

//==============================================================================
// CPU Measurement Result
//==============================================================================

struct CPUMeasurement
{
    uint64_t durationNs = 0;
    int blockSize = 0;
    double sampleRate = 0.0;
};

//==============================================================================
// CPU Statistics
//==============================================================================

struct CPUStats
{
    float currentCPU = 0.0f;
    float peakCPU = 0.0f;
    float averageCPU = 0.0f;
    int sampleCount = 0;
    uint64_t totalDurationNs = 0;

    static constexpr int historySize = 50;
    float history[historySize] = {0};
    int historyIndex = 0;

    void update(const CPUMeasurement& measurement, float warningThreshold)
    {
        currentCPU = calculateCPU(measurement);

        if (currentCPU > peakCPU)
            peakCPU = currentCPU;

        history[historyIndex] = currentCPU;
        historyIndex = (historyIndex + 1) % historySize;

        averageCPU = calculateAverage();
    }

    static float calculateCPU(const CPUMeasurement& measurement)
    {
        if (measurement.blockSize <= 0 || measurement.sampleRate <= 0)
            return 0.0f;

        double blockSizeSeconds = static_cast<double>(measurement.blockSize) / measurement.sampleRate;
        uint64_t idealTimeNs = static_cast<uint64_t>(blockSizeSeconds * 1e9);

        if (idealTimeNs == 0)
            return 0.0f;

        float cpu = static_cast<float>(static_cast<double>(measurement.durationNs) / idealTimeNs);
        return juce::jlimit(0.0f, 1.0f, cpu);
    }

    float calculateAverage() const
    {
        if (historyIndex == 0)
            return 0.0f;

        float sum = 0.0f;
        for (int i = 0; i < historyIndex; ++i)
            sum += history[i];

        return sum / historyIndex;
    }

    void reset()
    {
        currentCPU = 0.0f;
        peakCPU = 0.0f;
        averageCPU = 0.0f;
        sampleCount = 0;
        totalDurationNs = 0;
        for (int i = 0; i < historySize; ++i)
            history[i] = 0.0f;
        historyIndex = 0;
    }
};

//==============================================================================
// Profiling Component Type
//==============================================================================

enum class ProfilingComponent
{
    GlobalEngine,
    Track,
    Plugin,
    Effect,
    Timeline,
    MasterOutput,
    Unknown
};

inline juce::String toString(ProfilingComponent component)
{
    switch (component)
    {
        case ProfilingComponent::GlobalEngine: return "Global Engine";
        case ProfilingComponent::Track: return "Track";
        case ProfilingComponent::Plugin: return "Plugin";
        case ProfilingComponent::Effect: return "Effect";
        case ProfilingComponent::Timeline: return "Timeline";
        case ProfilingComponent::MasterOutput: return "Master Output";
        default: return "Unknown";
    }
}

//==============================================================================
// Warning Level
//==============================================================================

enum class WarningLevel
{
    None,
    Low,
    Medium,
    High,
    Critical
};

//==============================================================================
// CPU Profiler Main Class (Simplified)
//==============================================================================

class CPUProfiler
{
public:
    CPUProfiler();
    ~CPUProfiler();

    void initialize();
    void setSampleRate(double sampleRate);
    void setBlockSize(int blockSize);

    void startMeasure(ProfilingComponent type, int index = 0);
    void endMeasure(ProfilingComponent type, int index = 0);

    float getCurrentCPU(ProfilingComponent type, int index = 0) const;
    float getAverageCPU(ProfilingComponent type, int index = 0) const;
    float getPeakCPU(ProfilingComponent type, int index = 0) const;

    void setWarningCallback(std::function<void(ProfilingComponent, int, float)> callback);

    void resetStats(ProfilingComponent type, int index = 0);
    void resetAllStats();

    float getGlobalAverageCPU() const;
    float getGlobalPeakCPU() const;

private:
    std::array<CPUStats, 16> trackStats;
    CPUStats globalStats;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    std::function<void(ProfilingComponent, int, float)> warningCallback;

    static constexpr int maxActiveMeasures = 16;
    std::array<std::chrono::high_resolution_clock::time_point, maxActiveMeasures> activeMeasureStartTimes;
    std::array<bool, maxActiveMeasures> activeMeasureFlags;

    CPUStats* getStats(ProfilingComponent type, int index);
    const CPUStats* getStats(ProfilingComponent type, int index) const;

    float calculateCPU(const CPUMeasurement& measurement);
    void triggerWarning(const CPUStats& stats, float cpu);
};

//==============================================================================
// RAII Helper für automatische Messung
//==============================================================================

class ScopedCPUMeasure
{
public:
    ScopedCPUMeasure(CPUProfiler& profiler, ProfilingComponent type, int index = 0)
        : profiler(profiler), componentType(type), componentIndex(index)
    {
        profiler.startMeasure(type, index);
    }

    ~ScopedCPUMeasure()
    {
        profiler.endMeasure(componentType, componentIndex);
    }

private:
    CPUProfiler& profiler;
    ProfilingComponent componentType;
    int componentIndex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopedCPUMeasure)
};
