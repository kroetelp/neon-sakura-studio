#pragma once

/**
 * AutomationLane - Manages automation clips for a single track
 *
 * Each track can have multiple automation lanes, one per automated parameter.
 * This class manages the collection of automation clips and provides
 * value lookup for the audio engine.
 */

#include "AutomationClip.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

/**
 * AutomationLane - Container for automation clips of a single parameter
 *
 * Multiple clips can exist on a lane for different time regions.
 * Only one clip can be active at a given time position.
 */
class AutomationLane
{
public:
    AutomationLane(const juce::String& parameterId = {});
    ~AutomationLane() = default;

    // === Parameter Info ===
    void setParameterId(const juce::String& id) { parameterId = id; }
    juce::String getParameterId() const { return parameterId; }

    void setParameterName(const juce::String& name) { parameterName = name; }
    juce::String getParameterName() const { return parameterName; }

    // === Clip Management ===
    /** Add an automation clip to this lane */
    void addClip(std::unique_ptr<AutomationClip> clip);

    /** Remove a clip by ID */
    void removeClip(const juce::Uuid& clipId);

    /** Get clip by ID */
    AutomationClip* getClip(const juce::Uuid& clipId);

    /** Get all clips (for UI rendering) */
    std::vector<AutomationClip*> getClips();

    /** Get clip count */
    int getNumClips() const { return static_cast<int>(clips.size()); }

    /** Clear all clips */
    void clearClips();

    // === Value Lookup ===
    /**
     * Get the automation value at a given beat position.
     * Finds the active clip and returns its interpolated value.
     *
     * @param beatPosition Absolute beat position in timeline
     * @return Normalized value (0.0 to 1.0), or defaultValue if no automation
     */
    float getValueAtBeat(double beatPosition, float defaultValue = 0.5f) const;

    /**
     * Check if there's active automation at a given position
     */
    bool hasAutomationAt(double beatPosition) const;

    // === Active State ===
    /** Enable/disable this lane (for UI toggle) */
    void setActive(bool active) { isActive = active; }
    bool getActive() const { return isActive; }

    // === Recording Support ===
    /** Start recording automation at the given position */
    void startRecording(double startBeat);

    /** Record a value at the current position */
    void recordValue(double beatPosition, float value);

    /** Stop recording and finalize the clip */
    std::unique_ptr<AutomationClip> stopRecording();

    /** Check if currently recording */
    bool isRecording() const { return recording; }

private:
    juce::String parameterId;
    juce::String parameterName;

    std::vector<std::unique_ptr<AutomationClip>> clips;
    bool isActive = true;

    // Recording state
    bool recording = false;
    std::unique_ptr<AutomationClip> recordingClip;
    double recordingStartBeat = 0.0;

    mutable std::mutex laneMutex;

    /** Find clip that contains the given beat position */
    AutomationClip* findActiveClip(double beatPosition) const;
};

//==============================================================================

/**
 * AutomationManager - Manages all automation for a track
 *
 * Coordinates multiple automation lanes and applies values to parameters.
 * Integrates with TrackProcessor for real-time parameter updates.
 */
class AutomationManager
{
public:
    AutomationManager();
    ~AutomationManager() = default;

    // === Lane Management ===
    /** Create a lane for a parameter (if not exists) */
    AutomationLane* createLane(const juce::String& parameterId, const juce::String& parameterName = {});

    /** Get lane for a parameter */
    AutomationLane* getLane(const juce::String& parameterId);
    const AutomationLane* getLane(const juce::String& parameterId) const;

    /** Check if a parameter has automation */
    bool hasAutomation(const juce::String& parameterId) const;

    /** Get all lanes (for UI) */
    std::vector<AutomationLane*> getAllLanes();

    /** Remove a lane */
    void removeLane(const juce::String& parameterId);

    /** Clear all lanes */
    void clearAllLanes();

    // === Value Processing ===
    /**
     * Process automation for current playhead position.
     * Updates all parameter values based on automation clips.
     * Returns a map of parameter IDs to values.
     */
    std::unordered_map<juce::String, float> processAutomation(double currentBeat);

    /**
     * Get value for a specific parameter at current position
     */
    float getParameterValue(const juce::String& parameterId, double beat, float defaultValue = 0.5f) const;

    // === Recording ===
    /** Start recording for a parameter */
    void startRecording(const juce::String& parameterId, double startBeat);

    /** Record a value */
    void recordValue(const juce::String& parameterId, double beat, float value);

    /** Stop all recording */
    void stopAllRecording();

    /** Check if any lane is recording */
    bool isAnyRecording() const;

    // === State ===
    /** Enable/disable all automation */
    void setEnabled(bool enabled) { automationEnabled = enabled; }
    bool isEnabled() const { return automationEnabled; }

private:
    std::unordered_map<juce::String, std::unique_ptr<AutomationLane>> lanes;
    bool automationEnabled = true;

    mutable std::mutex managerMutex;
};
