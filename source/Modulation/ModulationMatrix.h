#pragma once

#include "ModulationSource.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include <functional>

class Modulator;  // Forward declaration

/**
 * ModulationContext - Per-voice modulation source values
 *
 * Passed to ModulationMatrix when retrieving modulation values.
 * Contains voice-specific values (velocity, aftertouch, note number)
 * and can include global values for reference.
 *
 * This enables per-voice modulation where each voice can have its own
 * velocity, aftertouch, etc. that doesn't affect other voices.
 */
struct ModulationContext
{
    // Per-voice MIDI values
    float velocity = 0.0f;      // Voice-specific velocity (0.0 - 1.0)
    float aftertouch = 0.0f;     // Per-note aftertouch (0.0 - 1.0)
    int midiNote = 60;           // Current note number (for keytrack, etc.)

    // Global MIDI values (synth-level - for reference only)
    float modWheel = 0.0f;       // Mod wheel (0.0 - 1.0)
    float pitchBend = 0.0f;      // Pitch bend (-1.0 to 1.0)

    // Helper methods
    bool isValid() const { return velocity > 0.0f; }
    static ModulationContext invalid() { return {}; }
};

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
 * - Per-voice modulation through ModulationContext
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

    // Set the modulator value provider (now with context support)
    using ModulatorValueProvider = std::function<float(ModulationSource, const ModulationContext&)>;
    void setModulatorValueProvider(ModulatorValueProvider provider) { getModulatorValue = provider; }

    // Get modulation value for a source with voice context
    float getSourceValue(ModulationSource source, const ModulationContext& context = {}) const;

    // Calculate total modulation for a target with voice context
    // Returns the combined modulation value (sum of all sources going to this target)
    float getModulationValue(ModulationTarget target, const ModulationContext& context) const;

    // Legacy overload for backward compatibility (uses empty context)
    float getModulationValue(ModulationTarget target) const;

    // Process all modulations with default context (should be called once per buffer)
    void process();

    // Process all modulations with voice context (per-voice modulation)
    void processWithContext(const ModulationContext& context);

    // Find routing index by source and target
    int findRouting(ModulationSource source, ModulationTarget target) const;

private:
    std::vector<ModulationRouting> routings;
    ModulatorValueProvider getModulatorValue;

    // Cached modulation values per target (for legacy process() calls)
    std::array<float, static_cast<int>(ModulationTarget::Count)> targetValues{};
};
