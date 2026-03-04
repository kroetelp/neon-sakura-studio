#pragma once

/**
 * AutomationClip - Timeline clip for parameter automation
 *
 * Stores automation points that can modulate parameters over time.
 * Supports multiple interpolation types for smooth transitions.
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>
#include <cmath>

/** Interpolation types for automation curves */
enum class AutomationInterpolation
{
    Step,       // Hold value until next point (discrete)
    Linear,     // Straight line between points
    Bezier      // Smooth curve with control points
};

/** A single automation point on the curve */
struct AutomationPoint
{
    double positionBeat = 0.0;      // Position in beats (relative to clip start)
    float value = 0.0f;             // Parameter value (0.0 to 1.0 normalized)
    float curve = 0.0f;             // Bezier curve tension (-1 to 1, 0 = linear)

    AutomationPoint() = default;
    AutomationPoint(double pos, float val, float crv = 0.0f)
        : positionBeat(pos), value(val), curve(crv) {}

    bool operator<(const AutomationPoint& other) const
    {
        return positionBeat < other.positionBeat;
    }
};

/**
 * AutomationClip - A clip containing automation data for one parameter
 *
 * Features:
 * - Multiple interpolation modes
 * - Bezier curves for smooth transitions
 * - Thread-safe value lookup
 * - Loop support for repeating patterns
 */
class AutomationClip
{
public:
    AutomationClip();
    ~AutomationClip() = default;

    // === Identity ===
    juce::Uuid getId() const { return clipId; }

    // === Parameter Info ===
    /** Set the parameter this clip automates (e.g., "filterCutoff", "oscMorph") */
    void setParameterId(const juce::String& paramId) { parameterId = paramId; }
    juce::String getParameterId() const { return parameterId; }

    /** Human-readable name for UI */
    void setParameterName(const juce::String& name) { parameterName = name; }
    juce::String getParameterName() const { return parameterName; }

    // === Timing ===
    std::atomic<double> startBeat{0.0};
    std::atomic<double> lengthBeats{4.0};
    std::atomic<bool> loopEnabled{false};

    double getEndBeat() const { return startBeat.load() + lengthBeats.load(); }
    bool containsBeat(double beat) const {
        return beat >= startBeat.load() && beat < getEndBeat();
    }

    // === Interpolation ===
    void setInterpolation(AutomationInterpolation mode) { interpolationMode = mode; }
    AutomationInterpolation getInterpolation() const { return interpolationMode; }

    // === Point Management ===
    /** Add an automation point (automatically sorted by position) */
    void addPoint(const AutomationPoint& point);

    /** Add a point with simplified parameters */
    void addPoint(double positionBeat, float value, float curve = 0.0f);

    /** Remove point at index */
    void removePoint(int index);

    /** Remove all points */
    void clearPoints();

    /** Get all points (for UI rendering) */
    const std::vector<AutomationPoint>& getPoints() const { return points; }

    /** Get point count */
    int getNumPoints() const { return static_cast<int>(points.size()); }

    // === Value Lookup ===
    /**
     * Get the automation value at a given beat position.
     * Handles looping and interpolation automatically.
     *
     * @param beatPosition Absolute beat position in timeline
     * @return Normalized value (0.0 to 1.0)
     */
    float getValueAtBeat(double beatPosition) const;

    /**
     * Get value at beat with custom interpolation mode.
     * Useful for preview rendering.
     */
    float getValueAtBeat(double beatPosition, AutomationInterpolation mode) const;

    // === Utility ===
    /** Create a linear ramp from start to end value */
    void createLinearRamp(float startValue, float endValue);

    /** Create an LFO-style pattern (sine wave) */
    void createLFOPattern(int cyclesPerClip, float amplitude = 1.0f);

    /** Scale all values by a factor */
    void scaleValues(float factor);

    /** Offset all values by a constant */
    void offsetValues(float offset);

private:
    juce::Uuid clipId;
    juce::String parameterId;
    juce::String parameterName;

    std::vector<AutomationPoint> points;
    AutomationInterpolation interpolationMode = AutomationInterpolation::Linear;

    mutable juce::CriticalSection pointsLock;

    /** Interpolate between two points */
    float interpolate(const AutomationPoint& p1, const AutomationPoint& p2,
                      double position, AutomationInterpolation mode) const;

    /** Bezier interpolation helper */
    float bezierInterpolate(float y0, float y1, float t, float curve) const;

    /** Get local beat position within clip (handles looping) */
    double getLocalBeatPosition(double globalBeat) const;
};
