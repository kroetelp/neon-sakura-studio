#include "CPUProfiler.h"

//==============================================================================
// CPUProfiler Implementation (Simplified)
//==============================================================================

CPUProfiler::CPUProfiler()
{
}

CPUProfiler::~CPUProfiler()
{
    resetAllStats();
}

//==============================================================================
// Initialization
//==============================================================================

void CPUProfiler::initialize()
{
    resetAllStats();
}

void CPUProfiler::setSampleRate(double sampleRate)
{
    currentSampleRate = sampleRate;
}

void CPUProfiler::setBlockSize(int blockSize)
{
    currentBlockSize = blockSize;
}

//==============================================================================
// Profiling Control (Called from Audio Thread)
//==============================================================================

void CPUProfiler::startMeasure(ProfilingComponent type, int index)
{
    int slotIndex = 0;
    for (auto& flag : activeMeasureFlags)
    {
        if (!flag)
        {
            activeMeasureFlags[slotIndex] = true;
            activeMeasureStartTimes[slotIndex] = std::chrono::high_resolution_clock::now();
            return;
        }
        slotIndex++;
    }

    // Alle Slots belegt
    DBG("CPUProfiler: All measurement slots in use!");
}

void CPUProfiler::endMeasure(ProfilingComponent type, int index)
{
    int slotIndex = 0;
    for (auto& flag : activeMeasureFlags)
    {
        if (flag && (static_cast<ProfilingComponent>(slotIndex) == type))
        {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                endTime - activeMeasureStartTimes[slotIndex]).count();

            CPUMeasurement measurement;
            measurement.durationNs = static_cast<uint64_t>(duration);
            measurement.blockSize = currentBlockSize;
            measurement.sampleRate = currentSampleRate;

            if (index >= 0 && index < 16)
            {
                trackStats[index].update(measurement, 0.70f);
            }
            else if (type == ProfilingComponent::GlobalEngine)
            {
                globalStats.update(measurement, 0.70f);
            }

            activeMeasureFlags[slotIndex] = false;
            return;
        }
        slotIndex++;
    }

    DBG("CPUProfiler: No matching active measurement found!");
}

//==============================================================================
// Statistics Access (Called from UI Thread)
//==============================================================================

float CPUProfiler::getCurrentCPU(ProfilingComponent type, int index) const
{
    if (index >= 0 && index < 16)
        return trackStats[index].currentCPU;
    return globalStats.currentCPU;
}

float CPUProfiler::getAverageCPU(ProfilingComponent type, int index) const
{
    if (index >= 0 && index < 16)
        return trackStats[index].averageCPU;
    return globalStats.averageCPU;
}

float CPUProfiler::getPeakCPU(ProfilingComponent type, int index) const
{
    if (index >= 0 && index < 16)
        return trackStats[index].peakCPU;
    return globalStats.peakCPU;
}

void CPUProfiler::setWarningCallback(std::function<void(ProfilingComponent, int, float)> callback)
{
    warningCallback = callback;
}

void CPUProfiler::resetStats(ProfilingComponent type, int index)
{
    if (index >= 0 && index < 16)
        trackStats[index].reset();
}

void CPUProfiler::resetAllStats()
{
    for (auto& stats : trackStats)
        stats.reset();
    globalStats.reset();

    for (auto& flag : activeMeasureFlags)
        flag = false;
}

float CPUProfiler::getGlobalAverageCPU() const
{
    return globalStats.averageCPU;
}

float CPUProfiler::getGlobalPeakCPU() const
{
    return globalStats.peakCPU;
}

//==============================================================================
// Internal Methods
//==============================================================================

CPUStats* CPUProfiler::getStats(ProfilingComponent type, int index)
{
    if (index >= 0 && index < 16)
        return &trackStats[index];
    return &globalStats;
}

const CPUStats* CPUProfiler::getStats(ProfilingComponent type, int index) const
{
    if (index >= 0 && index < 16)
        return &trackStats[index];
    return &globalStats;
}

float CPUProfiler::calculateCPU(const CPUMeasurement& measurement)
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

void CPUProfiler::triggerWarning(const CPUStats& stats, float cpu)
{
    if (cpu > 0.90f && warningCallback)
    {
        juce::String componentStr = (&stats == &globalStats) ? "Global Engine" : "Track";
        DBG("CPU Warning - Component: " + componentStr + ", CPU: " + juce::String(cpu * 100.0f, 1) + "%");

        warningCallback(ProfilingComponent::GlobalEngine, 0, cpu);
    }
}
