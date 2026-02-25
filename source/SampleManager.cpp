#include "SampleManager.h"

SampleManager::SampleManager()
{
}

juce::StringArray SampleManager::getSampleCategories() const
{
    juce::ScopedLock lock(directoryLock);
    return sampleCategories;
}

juce::File SampleManager::getSampleDirectory() const
{
    return sampleDirectory;
}

void SampleManager::loadSampleForCategory(int trackIndex, const juce::String& category)
{
    if (sampleLoadCallback && sampleDirectory.exists())
    {
        sampleLoadCallback(trackIndex, category, sampleDirectory);
    }
}

void SampleManager::setSampleDirectory(const juce::File& directory)
{
    sampleDirectory = directory;

    if (onDirectoryChanged)
        onDirectoryChanged(directory);
}

void SampleManager::scanSampleDirectory(const juce::File& directory)
{
    juce::ScopedLock lock(directoryLock);

    sampleDirectory = directory;
    sampleCategories.clear();

    juce::Array<juce::File> subdirectories;
    directory.findChildFiles(subdirectories, juce::File::findDirectories, false);

    for (auto& dir : subdirectories)
    {
        sampleCategories.add(dir.getFileName());
    }

    if (onCategoriesChanged)
        onCategoriesChanged(sampleCategories);
}

void SampleManager::setSampleLoadCallback(std::function<void(int, const juce::String&, const juce::File&)> callback)
{
    sampleLoadCallback = std::move(callback);
}

juce::File SampleManager::autoDetectSampleDirectory()
{
    juce::File exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    juce::StringArray searchDirs = {
        "Dirt-Samples-master",
        "Dirt-Samples",
        "samples"
    };

    // Search at executable level
    if (searchAtLevel(exeDir, searchDirs))
        return sampleDirectory;

    // Search at parent level
    juce::File parentDir = exeDir.getParentDirectory();
    if (parentDir.exists() && searchAtLevel(parentDir, searchDirs))
        return sampleDirectory;

    // Search at grandparent level
    juce::File grandParentDir = parentDir.getParentDirectory();
    if (grandParentDir.exists() && searchAtLevel(grandParentDir, searchDirs))
        return sampleDirectory;

    return juce::File();
}

bool SampleManager::searchAtLevel(const juce::File& baseDir, const juce::StringArray& searchDirs)
{
    for (const auto& dirName : searchDirs)
    {
        juce::File candidate = baseDir.getChildFile(dirName);
        if (candidate.exists() && candidate.isDirectory())
        {
            scanSampleDirectory(candidate);
            return true;
        }
    }
    return false;
}

void SampleManager::addPendingLoad(int trackIndex, const juce::String& category)
{
    juce::ScopedLock lock(pendingLoadLock);
    pendingSampleLoads.push_back({trackIndex, category});
}

void SampleManager::processPendingLoads()
{
    juce::ScopedLock lock(pendingLoadLock);

    for (const auto& pending : pendingSampleLoads)
    {
        loadSampleForCategory(pending.trackIndex, pending.category);
    }
    pendingSampleLoads.clear();
}

bool SampleManager::hasPendingLoads() const
{
    juce::ScopedLock lock(pendingLoadLock);
    return !pendingSampleLoads.empty();
}
