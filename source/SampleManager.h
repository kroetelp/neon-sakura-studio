#pragma once

/**
 * SampleManager - Centralized sample directory management
 *
 * Responsibilities:
 * - Sample directory management
 * - Category scanning
 * - Sample loading coordination
 * - Auto-detection of sample directories
 */

#include "ISampleProvider.h"
#include <juce_core/juce_core.h>
#include <atomic>
#include <vector>
#include <functional>

class SampleManager : public ISampleProvider
{
public:
    struct PendingSampleLoad
    {
        int trackIndex;
        juce::String category;
    };

    SampleManager();
    ~SampleManager() override = default;

    // ISampleProvider Implementation
    juce::StringArray getSampleCategories() const override;
    juce::File getSampleDirectory() const override;
    void loadSampleForCategory(int trackIndex, const juce::String& category) override;
    void setSampleDirectory(const juce::File& directory) override;
    void scanSampleDirectory(const juce::File& directory) override;

    // Sample Loading Callback (MainComponent -> TrackComponent)
    void setSampleLoadCallback(std::function<void(int, const juce::String&, const juce::File&)> callback);

    // Auto-Detection - searches for common sample directories
    juce::File autoDetectSampleDirectory();

    // Pending loads handling (for deferred loading)
    void addPendingLoad(int trackIndex, const juce::String& category);
    void processPendingLoads();
    bool hasPendingLoads() const;

    // Notifications
    std::function<void(const juce::StringArray&)> onCategoriesChanged;
    std::function<void(const juce::File&)> onDirectoryChanged;

private:
    juce::File sampleDirectory;
    juce::StringArray sampleCategories;
    juce::CriticalSection directoryLock;

    std::vector<PendingSampleLoad> pendingSampleLoads;
    juce::CriticalSection pendingLoadLock;

    std::function<void(int, const juce::String&, const juce::File&)> sampleLoadCallback;

    // Helper to search for sample directory at various levels
    bool searchAtLevel(const juce::File& baseDir, const juce::StringArray& searchDirs);
};
