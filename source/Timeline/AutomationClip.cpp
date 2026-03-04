#include "AutomationClip.h"
#include <algorithm>
#include <cmath>

//==============================================================================
AutomationClip::AutomationClip()
    : clipId(juce::Uuid())
{
}

//==============================================================================
void AutomationClip::addPoint(const AutomationPoint& point)
{
    juce::ScopedLock lock(pointsLock);

    // Find insertion position to keep sorted
    auto it = std::lower_bound(points.begin(), points.end(), point);
    points.insert(it, point);
}

//==============================================================================
void AutomationClip::addPoint(double positionBeat, float value, float curve)
{
    addPoint(AutomationPoint(positionBeat, value, curve));
}

//==============================================================================
void AutomationClip::removePoint(int index)
{
    juce::ScopedLock lock(pointsLock);

    if (index >= 0 && index < static_cast<int>(points.size()))
        points.erase(points.begin() + index);
}

//==============================================================================
void AutomationClip::clearPoints()
{
    juce::ScopedLock lock(pointsLock);
    points.clear();
}

//==============================================================================
double AutomationClip::getLocalBeatPosition(double globalBeat) const
{
    double localBeat = globalBeat - startBeat.load();

    if (loopEnabled.load() && lengthBeats.load() > 0)
    {
        // Wrap within loop length
        localBeat = std::fmod(localBeat, lengthBeats.load());
        if (localBeat < 0)
            localBeat += lengthBeats.load();
    }

    return localBeat;
}

//==============================================================================
float AutomationClip::getValueAtBeat(double beatPosition) const
{
    return getValueAtBeat(beatPosition, interpolationMode);
}

//==============================================================================
float AutomationClip::getValueAtBeat(double beatPosition, AutomationInterpolation mode) const
{
    juce::ScopedLock lock(pointsLock);

    if (points.empty())
        return 0.0f;

    double localBeat = getLocalBeatPosition(beatPosition);

    // Find surrounding points
    int lowerIdx = -1;
    int upperIdx = -1;

    for (int i = 0; i < static_cast<int>(points.size()); ++i)
    {
        if (points[i].positionBeat <= localBeat)
            lowerIdx = i;
        if (points[i].positionBeat >= localBeat && upperIdx == -1)
            upperIdx = i;
    }

    // Edge cases
    if (lowerIdx == -1 && upperIdx == -1)
        return 0.0f;

    if (lowerIdx == -1)
        return points[upperIdx].value;  // Before first point

    if (upperIdx == -1)
        return points[lowerIdx].value;  // After last point

    if (lowerIdx == upperIdx)
        return points[lowerIdx].value;  // Exactly on a point

    // Interpolate between points
    return interpolate(points[lowerIdx], points[upperIdx], localBeat, mode);
}

//==============================================================================
float AutomationClip::interpolate(const AutomationPoint& p1, const AutomationPoint& p2,
                                   double position, AutomationInterpolation mode) const
{
    if (mode == AutomationInterpolation::Step)
    {
        return p1.value;  // Hold previous value
    }

    // Calculate normalized position between points
    double range = p2.positionBeat - p1.positionBeat;
    if (range <= 0)
        return p1.value;

    double t = (position - p1.positionBeat) / range;
    t = juce::jlimit(0.0, 1.0, t);

    if (mode == AutomationInterpolation::Linear)
    {
        return p1.value + (p2.value - p1.value) * static_cast<float>(t);
    }
    else  // Bezier
    {
        // Use average curve tension
        float curve = (p1.curve + p2.curve) * 0.5f;
        return bezierInterpolate(p1.value, p2.value, static_cast<float>(t), curve);
    }
}

//==============================================================================
float AutomationClip::bezierInterpolate(float y0, float y1, float t, float curve) const
{
    // Ease-in-out style bezier with curve parameter
    // curve = -1: ease-in (slow start)
    // curve = 0: linear
    // curve = 1: ease-out (slow end)

    float result;

    if (curve < 0)
    {
        // Ease-in
        float factor = 1.0f + curve;  // 0 to 1
        result = y0 + (y1 - y0) * (t * t * (2 - factor) + factor * t * (1 - t));
    }
    else
    {
        // Ease-out or combination
        float factor = curve;  // 0 to 1
        float t2 = t * t;
        float t3 = t2 * t;
        float mt = 1 - t;
        float mt2 = mt * mt;
        float mt3 = mt2 * mt;

        // Cubic bezier approximation
        result = y0 * mt3 +
                 (y0 + (y1 - y0) * (1 - factor)) * 3 * mt2 * t +
                 (y1 - (y1 - y0) * factor) * 3 * mt * t2 +
                 y1 * t3;
    }

    return result;
}

//==============================================================================
void AutomationClip::createLinearRamp(float startValue, float endValue)
{
    juce::ScopedLock lock(pointsLock);
    points.clear();

    points.emplace_back(0.0, startValue);
    points.emplace_back(lengthBeats.load(), endValue);
}

//==============================================================================
void AutomationClip::createLFOPattern(int cyclesPerClip, float amplitude)
{
    juce::ScopedLock lock(pointsLock);
    points.clear();

    int pointsPerCycle = 8;
    int totalPoints = cyclesPerClip * pointsPerCycle;

    for (int i = 0; i <= totalPoints; ++i)
    {
        double pos = (static_cast<double>(i) / totalPoints) * lengthBeats.load();
        float phase = static_cast<float>(i) / pointsPerCycle * juce::MathConstants<float>::twoPi;
        float value = 0.5f + 0.5f * std::sin(phase) * amplitude;

        points.emplace_back(pos, value);
    }
}

//==============================================================================
void AutomationClip::scaleValues(float factor)
{
    juce::ScopedLock lock(pointsLock);

    for (auto& point : points)
    {
        point.value = juce::jlimit(0.0f, 1.0f, point.value * factor);
    }
}

//==============================================================================
void AutomationClip::offsetValues(float offset)
{
    juce::ScopedLock lock(pointsLock);

    for (auto& point : points)
    {
        point.value = juce::jlimit(0.0f, 1.0f, point.value + offset);
    }
}
