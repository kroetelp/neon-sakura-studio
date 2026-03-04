#pragma once

#include "TimelineClip.h"
#include "AutomationLane.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <vector>
#include <memory>
#include <mutex>
#include <algorithm>

class TimelineTrack
{
public:
    TimelineTrack(int index) : trackIndex(index), automationManager(std::make_unique<AutomationManager>()) {}

    // Identity
    int getTrackIndex() const { return trackIndex; }

    // Properties
    std::atomic<float> volume{1.0f};
    std::atomic<float> pan{0.0f};           // -1.0 to 1.0
    std::atomic<bool> muted{false};
    std::atomic<bool> soloed{false};
    std::atomic<bool> armed{false};         // For recording
    juce::String name{"Track"};

    // Clip management (thread-safe)
    void addClip(std::unique_ptr<TimelineClip> clip)
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        clips.push_back(std::move(clip));
        sortClips();
    }

    void removeClip(const juce::Uuid& clipId)
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        clips.erase(
            std::remove_if(clips.begin(), clips.end(),
                [&clipId](const std::unique_ptr<TimelineClip>& clip) {
                    return clip->getId() == clipId;
                }),
            clips.end()
        );
    }

    TimelineClip* findClip(const juce::Uuid& clipId)
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        for (auto& clip : clips)
        {
            if (clip->getId() == clipId)
                return clip.get();
        }
        return nullptr;
    }

    TimelineClip* findClipAtBeat(double beat)
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        for (auto& clip : clips)
        {
            if (clip->containsBeat(beat) && !clip->muted.load())
                return clip.get();
        }
        return nullptr;
    }

    // Get all clips (for rendering/minimap)
    std::vector<TimelineClip*> getClips() const
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        std::vector<TimelineClip*> result;
        for (const auto& clip : clips)
        {
            result.push_back(clip.get());
        }
        return result;
    }

    // Get clip count
    size_t getNumClips() const
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        return clips.size();
    }

    // Iterate clips (for UI)
    template<typename Func>
    void forEachClip(Func&& func)
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        for (auto& clip : clips)
        {
            func(*clip);
        }
    }

    // Get track end position
    double getEndBeat() const
    {
        std::lock_guard<std::mutex> lock(trackMutex);
        double endBeat = 0.0;
        for (const auto& clip : clips)
        {
            double clipEnd = clip->startBeat.load() + clip->lengthBeats.load();
            if (clipEnd > endBeat)
                endBeat = clipEnd;
        }
        return endBeat;
    }

    // === Automation ===
    /** Get automation manager for this track */
    AutomationManager* getAutomationManager() { return automationManager.get(); }
    const AutomationManager* getAutomationManager() const { return automationManager.get(); }

    /**
     * Process automation for current playhead position.
     * Returns map of parameter IDs to values.
     */
    std::unordered_map<juce::String, float> processAutomation(double currentBeat)
    {
        if (automationManager)
            return automationManager->processAutomation(currentBeat);
        return {};
    }

private:
    int trackIndex;
    std::vector<std::unique_ptr<TimelineClip>> clips;
    std::unique_ptr<AutomationManager> automationManager;
    mutable std::mutex trackMutex;

    void sortClips()
    {
        std::sort(clips.begin(), clips.end(),
            [](const std::unique_ptr<TimelineClip>& a, const std::unique_ptr<TimelineClip>& b) {
                return a->startBeat.load() < b->startBeat.load();
            });
    }
};
