#include "AutomationLane.h"
#include <algorithm>

//==============================================================================
// AutomationLane Implementation
//==============================================================================

AutomationLane::AutomationLane(const juce::String& paramId)
    : parameterId(paramId)
{
}

//==============================================================================
void AutomationLane::addClip(std::unique_ptr<AutomationClip> clip)
{
    std::lock_guard<std::mutex> lock(laneMutex);

    if (clip)
    {
        clips.push_back(std::move(clip));

        // Sort by start beat
        std::sort(clips.begin(), clips.end(),
            [](const std::unique_ptr<AutomationClip>& a, const std::unique_ptr<AutomationClip>& b) {
                return a->startBeat.load() < b->startBeat.load();
            });
    }
}

//==============================================================================
void AutomationLane::removeClip(const juce::Uuid& clipId)
{
    std::lock_guard<std::mutex> lock(laneMutex);

    clips.erase(
        std::remove_if(clips.begin(), clips.end(),
            [&clipId](const std::unique_ptr<AutomationClip>& clip) {
                return clip && clip->getId() == clipId;
            }),
        clips.end()
    );
}

//==============================================================================
AutomationClip* AutomationLane::getClip(const juce::Uuid& clipId)
{
    std::lock_guard<std::mutex> lock(laneMutex);

    for (auto& clip : clips)
    {
        if (clip && clip->getId() == clipId)
            return clip.get();
    }
    return nullptr;
}

//==============================================================================
std::vector<AutomationClip*> AutomationLane::getClips()
{
    std::lock_guard<std::mutex> lock(laneMutex);

    std::vector<AutomationClip*> result;
    result.reserve(clips.size());

    for (auto& clip : clips)
    {
        if (clip)
            result.push_back(clip.get());
    }

    return result;
}

//==============================================================================
void AutomationLane::clearClips()
{
    std::lock_guard<std::mutex> lock(laneMutex);
    clips.clear();
}

//==============================================================================
AutomationClip* AutomationLane::findActiveClip(double beatPosition) const
{
    for (const auto& clip : clips)
    {
        if (clip && clip->containsBeat(beatPosition) && !clip->loopEnabled.load())
            return clip.get();
    }

    // Check for looping clips if no non-looping clip found
    for (const auto& clip : clips)
    {
        if (clip && clip->loopEnabled.load())
            return clip.get();
    }

    return nullptr;
}

//==============================================================================
float AutomationLane::getValueAtBeat(double beatPosition, float defaultValue) const
{
    if (!isActive)
        return defaultValue;

    std::lock_guard<std::mutex> lock(laneMutex);

    auto* clip = findActiveClip(beatPosition);
    if (clip)
        return clip->getValueAtBeat(beatPosition);

    return defaultValue;
}

//==============================================================================
bool AutomationLane::hasAutomationAt(double beatPosition) const
{
    std::lock_guard<std::mutex> lock(laneMutex);
    return findActiveClip(beatPosition) != nullptr;
}

//==============================================================================
void AutomationLane::startRecording(double startBeat)
{
    std::lock_guard<std::mutex> lock(laneMutex);

    recording = true;
    recordingStartBeat = startBeat;
    recordingClip = std::make_unique<AutomationClip>();
    recordingClip->startBeat.store(startBeat);
    recordingClip->setParameterId(parameterId);
    recordingClip->setParameterName(parameterName);
}

//==============================================================================
void AutomationLane::recordValue(double beatPosition, float value)
{
    std::lock_guard<std::mutex> lock(laneMutex);

    if (recording && recordingClip)
    {
        double localBeat = beatPosition - recordingStartBeat;
        if (localBeat >= 0)
        {
            recordingClip->addPoint(localBeat, value);
            recordingClip->lengthBeats.store(localBeat + 0.1);  // Extend length
        }
    }
}

//==============================================================================
std::unique_ptr<AutomationClip> AutomationLane::stopRecording()
{
    std::lock_guard<std::mutex> lock(laneMutex);

    recording = false;

    if (recordingClip && recordingClip->getNumPoints() > 1)
    {
        return std::move(recordingClip);
    }

    recordingClip.reset();
    return nullptr;
}

//==============================================================================
// AutomationManager Implementation
//==============================================================================

AutomationManager::AutomationManager()
{
}

//==============================================================================
AutomationLane* AutomationManager::createLane(const juce::String& parameterId, const juce::String& parameterName)
{
    std::lock_guard<std::mutex> lock(managerMutex);

    auto& lane = lanes[parameterId];
    if (!lane)
    {
        lane = std::make_unique<AutomationLane>(parameterId);
        lane->setParameterName(parameterName.isEmpty() ? parameterId : parameterName);
    }

    return lane.get();
}

//==============================================================================
AutomationLane* AutomationManager::getLane(const juce::String& parameterId)
{
    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = lanes.find(parameterId);
    if (it != lanes.end())
        return it->second.get();

    return nullptr;
}

//==============================================================================
const AutomationLane* AutomationManager::getLane(const juce::String& parameterId) const
{
    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = lanes.find(parameterId);
    if (it != lanes.end())
        return it->second.get();

    return nullptr;
}

//==============================================================================
bool AutomationManager::hasAutomation(const juce::String& parameterId) const
{
    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = lanes.find(parameterId);
    return it != lanes.end() && it->second->getNumClips() > 0;
}

//==============================================================================
std::vector<AutomationLane*> AutomationManager::getAllLanes()
{
    std::lock_guard<std::mutex> lock(managerMutex);

    std::vector<AutomationLane*> result;
    result.reserve(lanes.size());

    for (auto& pair : lanes)
    {
        if (pair.second)
            result.push_back(pair.second.get());
    }

    return result;
}

//==============================================================================
void AutomationManager::removeLane(const juce::String& parameterId)
{
    std::lock_guard<std::mutex> lock(managerMutex);
    lanes.erase(parameterId);
}

//==============================================================================
void AutomationManager::clearAllLanes()
{
    std::lock_guard<std::mutex> lock(managerMutex);
    lanes.clear();
}

//==============================================================================
std::unordered_map<juce::String, float> AutomationManager::processAutomation(double currentBeat)
{
    std::unordered_map<juce::String, float> values;

    if (!automationEnabled)
        return values;

    std::lock_guard<std::mutex> lock(managerMutex);

    for (auto& pair : lanes)
    {
        if (pair.second && pair.second->getActive())
        {
            float value = pair.second->getValueAtBeat(currentBeat, 0.5f);
            values[pair.first] = value;
        }
    }

    return values;
}

//==============================================================================
float AutomationManager::getParameterValue(const juce::String& parameterId, double beat, float defaultValue) const
{
    if (!automationEnabled)
        return defaultValue;

    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = lanes.find(parameterId);
    if (it != lanes.end() && it->second)
        return it->second->getValueAtBeat(beat, defaultValue);

    return defaultValue;
}

//==============================================================================
void AutomationManager::startRecording(const juce::String& parameterId, double startBeat)
{
    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = lanes.find(parameterId);
    if (it != lanes.end() && it->second)
    {
        it->second->startRecording(startBeat);
    }
    else
    {
        // Create lane if it doesn't exist
        auto& lane = lanes[parameterId];
        lane = std::make_unique<AutomationLane>(parameterId);
        lane->startRecording(startBeat);
    }
}

//==============================================================================
void AutomationManager::recordValue(const juce::String& parameterId, double beat, float value)
{
    std::lock_guard<std::mutex> lock(managerMutex);

    auto it = lanes.find(parameterId);
    if (it != lanes.end() && it->second)
        it->second->recordValue(beat, value);
}

//==============================================================================
void AutomationManager::stopAllRecording()
{
    std::lock_guard<std::mutex> lock(managerMutex);

    for (auto& pair : lanes)
    {
        if (pair.second)
        {
            auto clip = pair.second->stopRecording();
            if (clip)
                pair.second->addClip(std::move(clip));
        }
    }
}

//==============================================================================
bool AutomationManager::isAnyRecording() const
{
    std::lock_guard<std::mutex> lock(managerMutex);

    for (const auto& pair : lanes)
    {
        if (pair.second && pair.second->isRecording())
            return true;
    }

    return false;
}
