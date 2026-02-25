#pragma once

#include "ModulationSource.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <functional>

class Modulator;  // Forward declaration

/**
 * ModulationRouting - Single routing from source to target
 */
struct ModulationRouting
{
    ModulationSource source;
    ModulationTarget target;
    float amount = 0.0f;    // -1.0 to 1.0
    bool bipolar = true;    // If true, output is -1 to 1; if false, 0 to 1
    bool active = true;
};

/**
 * ModulationMatrix - Routes modulation sources to targets
 *
 * Handles:
 * - Adding/removing routings
 * - Calculating modulation values for each target
 * - Thread-safe parameter updates
 */
class ModulationMatrix
{
public:
    static constexpr int maxRoutings = 64;

    ModulationMatrix();
    ~ModulationMatrix() = default;

    // Routing management
    int addRouting(ModulationSource source, ModulationTarget target, float amount = 0.0f, bool bipolar = true);
    void removeRouting(int index);
    void clearAllRoutings();

    // Modify existing routing
    void setRoutingAmount(int index, float amount);
    void setRoutingBipolar(int index, bool bipolar);
    void setRoutingActive(int index, bool active);

    // Get routing info
    const ModulationRouting& getRouting(int index) const;
    int getNumRoutings() const { return static_cast<int>(routings.size()); }

    // Set the modulator value providers
    using ModulatorValueProvider = std::function<float(ModulationSource)>;
    void setModulatorValueProvider(ModulatorValueProvider provider) { getModulatorValue = provider; }

    // Calculate total modulation for a target
    // Returns the combined modulation value (sum of all sources going to this target)
    float getModulationValue(ModulationTarget target) const;

    // Process all modulations (should be called once per sample before applying modulations)
    void process();

    // Find routing index by source and target
    int findRouting(ModulationSource source, ModulationTarget target) const;

private:
    std::vector<ModulationRouting> routings;
    ModulatorValueProvider getModulatorValue;

    // Cached modulation values per target
    std::array<float, static_cast<int>(ModulationTarget::Count)> targetValues{};
};
