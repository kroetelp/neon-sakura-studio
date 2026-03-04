#include "PluginScanner.h"
#include "VSTPluginManager.h"
#include "PluginHostConfiguration.h"

//==============================================================================
PluginScanner::PluginScanner(VSTPluginManager& owner_)
    : owner(owner_)
{
}

PluginScanner::~PluginScanner()
{
    cancelScan();
}

//==============================================================================
// Scanning Control
//==============================================================================

void PluginScanner::startScan()
{
    startScan(getDefaultSearchPath());
}

void PluginScanner::startScan(const juce::FileSearchPath& searchPath)
{
    // Don't start if already scanning
    if (scanning.load())
    {
        DBG("PluginScanner: Scan already in progress");
        return;
    }

    // Cancel any previous scan thread
    cancelScan();

    // Reset state
    scanning.store(true);
    cancelRequested.store(false);
    progress.store(0.0f);
    pluginsFound.store(0);
    filesScanned.store(0);
    setCurrentFile("");
    tempPluginList.clear();
    tempBlacklist.clear();
    lastSearchPath = searchPath;

    // Count files first for accurate progress
    totalFilesToScan.store(countPluginFiles(searchPath));
    DBG("PluginScanner: Starting scan of " + juce::String(totalFilesToScan.load()) + " potential plugin files");

    // Launch background thread
    scanThread = std::thread([this, searchPath]()
    {
        runScan(searchPath);
    });

    // Start timer for UI updates (10 Hz)
    startTimerHz(10);
}

void PluginScanner::cancelScan()
{
    // Signal cancellation
    cancelRequested.store(true);

    // Wait for thread to finish
    if (scanThread.joinable())
    {
        scanThread.join();
    }

    stopTimer();
    scanning.store(false);
}

//==============================================================================
// Progress Tracking
//==============================================================================

juce::String PluginScanner::getCurrentFile() const
{
    std::lock_guard<std::mutex> lock(currentFileMutex);
    return currentFileScanning;
}

void PluginScanner::updateProgress(float newProgress)
{
    progress.store(juce::jlimit(0.0f, 1.0f, newProgress));
}

void PluginScanner::setCurrentFile(const juce::String& filename)
{
    std::lock_guard<std::mutex> lock(currentFileMutex);
    currentFileScanning = filename;
}

void PluginScanner::incrementPluginsFound()
{
    pluginsFound.fetch_add(1);
}

void PluginScanner::incrementFilesScanned()
{
    filesScanned.fetch_add(1);
}

//==============================================================================
// Search Paths
//==============================================================================

juce::FileSearchPath PluginScanner::getDefaultSearchPath() const
{
    juce::FileSearchPath searchPath;

#if JUCE_WINDOWS
    // System VST3 directory (64-bit)
    searchPath.add(juce::File("C:\\Program Files\\Common Files\\VST3"));

    // User VST3 directory
    auto userAppData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    searchPath.add(userAppData.getChildFile("VST3"));

    // Legacy VST2 paths (some users still have plugins here)
    searchPath.add(juce::File("C:\\Program Files\\VSTPlugins"));
    searchPath.add(juce::File("C:\\Program Files\\Steinberg\\VSTPlugins"));

    // User VST2
    searchPath.add(userAppData.getChildFile("VSTPlugins"));

#elif JUCE_MAC
    // System VST3
    searchPath.add(juce::File("/Library/Audio/Plug-Ins/VST3"));

    // System AU (Audio Units)
    searchPath.add(juce::File("/Library/Audio/Plug-Ins/Components"));

    // User VST3
    searchPath.add(juce::File("~").getChildFile("Library/Audio/Plug-Ins/VST3"));

    // User AU
    searchPath.add(juce::File("~").getChildFile("Library/Audio/Plug-Ins/Components"));

#elif JUCE_LINUX
    // System VST3
    searchPath.add(juce::File("/usr/lib/vst3"));
    searchPath.add(juce::File("/usr/local/lib/vst3"));

    // User VST3
    searchPath.add(juce::File("~").getChildFile(".vst3"));

    // VST2 (legacy)
    searchPath.add(juce::File("/usr/lib/vst"));
    searchPath.add(juce::File("/usr/local/lib/vst"));
#endif

    return searchPath;
}

//==============================================================================
// Blacklist Management
//==============================================================================

bool PluginScanner::isBlacklisted(const juce::String& fileOrIdentifier) const
{
    return owner.getKnownPlugins().getBlacklistedFiles().contains(fileOrIdentifier);
}

void PluginScanner::addToBlacklist(const juce::String& fileOrIdentifier)
{
    owner.getKnownPlugins().addToBlacklist(fileOrIdentifier);
    tempBlacklist.add(fileOrIdentifier);
}

//==============================================================================
// Timer Callback (Main Thread)
//==============================================================================

void PluginScanner::timerCallback()
{
    // Call progress callback
    if (progressCallback)
    {
        progressCallback(progress.load());
    }

    // Check if scan is complete
    if (!scanning.load())
    {
        stopTimer();

        // Wait for thread to fully finish
        if (scanThread.joinable())
        {
            scanThread.join();
        }

        // Merge results into owner's KnownPluginList
        DBG("PluginScanner: Scan complete, merging results...");

        // Add blacklisted files
        for (const auto& file : tempBlacklist)
        {
            owner.getKnownPlugins().addToBlacklist(file);
        }

        // Add discovered plugins
        int newPluginCount = 0;
        for (int i = 0; i < tempPluginList.getNumTypes(); ++i)
        {
            if (auto* desc = tempPluginList.getType(i))
            {
                // Check if already known
                if (owner.getKnownPlugins().getTypeForFile(desc->fileOrIdentifier) == nullptr)
                {
                    owner.getKnownPlugins().addType(*desc);
                    newPluginCount++;
                }
            }
        }

        DBG("PluginScanner: Added " + juce::String(newPluginCount) + " new plugins to known list");

        // Call completion callback
        if (completionCallback)
        {
            completionCallback(pluginsFound.load());
        }
    }
}

//==============================================================================
// Background Scanning (Worker Thread)
//==============================================================================

void PluginScanner::runScan(const juce::FileSearchPath& searchPath)
{
    auto& formatManager = owner.getFormatManager();

    DBG("PluginScanner: Background scan started");

    // Scan each format
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        if (cancelRequested.load())
            break;

        auto* format = formatManager.getFormat(i);

        if (format == nullptr || !format->canScanForPlugins())
            continue;

        DBG("PluginScanner: Scanning for " + format->getName() + " plugins...");

        scanForFormat(format, searchPath);
    }

    // Mark scan complete
    scanning.store(false);
    progress.store(1.0f);
    setCurrentFile("");

    DBG("PluginScanner: Background scan finished. Found " + juce::String(pluginsFound.load()) + " plugins");
}

void PluginScanner::scanForFormat(juce::AudioPluginFormat* format, const juce::FileSearchPath& searchPath)
{
    if (format == nullptr || cancelRequested.load())
        return;

    // Get all potential plugin files for this format
    juce::StringArray pluginFiles = format->searchPathsForPlugins(searchPath, true, false);

    DBG("PluginScanner: Found " + juce::String(pluginFiles.size()) + " potential " + format->getName() + " files");

    // Debug: List all found files
    for (const auto& f : pluginFiles)
    {
        DBG("  -> " + f);
    }

    for (const auto& file : pluginFiles)
    {
        if (cancelRequested.load())
            break;

        // Skip blacklisted plugins
        if (isBlacklisted(file))
        {
            DBG("PluginScanner: Skipping blacklisted plugin: " + file);
            incrementFilesScanned();
            continue;
        }

        // Skip if already known
        if (owner.getKnownPlugins().getTypeForFile(file) != nullptr)
        {
            incrementFilesScanned();
            continue;
        }

        scanFile(juce::File(file), format);
    }
}

void PluginScanner::scanFile(const juce::File& file, juce::AudioPluginFormat* format)
{
    if (cancelRequested.load() || format == nullptr)
        return;

    setCurrentFile(file.getFileName());
    DBG("PluginScanner: Scanning " + file.getFullPathName());

    // Check if file/bundle exists
    if (!file.exists())
    {
        DBG("PluginScanner: File does not exist: " + file.getFullPathName());
        incrementFilesScanned();
        return;
    }

    try
    {
        // Try to get plugin descriptions from file
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format->findAllTypesForFile(descriptions, file.getFullPathName());

        DBG("PluginScanner: findAllTypesForFile found " + juce::String(descriptions.size()) + " descriptions");

        if (!descriptions.isEmpty())
        {
            for (auto* desc : descriptions)
            {
                if (desc != nullptr)
                {
                    tempPluginList.addType(*desc);
                    incrementPluginsFound();

                    DBG("PluginScanner: Found plugin: " + desc->name +
                        " by " + desc->manufacturerName +
                        " (" + desc->pluginFormatName + ")");

                    if (pluginFoundCallback)
                    {
                        // Note: Callback will be called on main thread via timer
                    }
                }
            }
        }
        else
        {
            DBG("PluginScanner: No plugin descriptions found for: " + file.getFileName());
        }
    }
    catch (const std::exception& e)
    {
        // Plugin crashed during scan - add to blacklist
        DBG("PluginScanner: Plugin crashed during scan (exception): " + juce::String(e.what()));
        addToBlacklist(file.getFullPathName());
    }
    catch (...)
    {
        // Plugin crashed during scan - add to blacklist
        DBG("PluginScanner: Plugin crashed during scan (unknown), blacklisting: " + file.getFullPathName());
        addToBlacklist(file.getFullPathName());
    }

    incrementFilesScanned();

    // Update progress
    int total = totalFilesToScan.load();
    if (total > 0)
    {
        float prog = static_cast<float>(filesScanned.load()) / static_cast<float>(total);
        updateProgress(prog);
    }
}

//==============================================================================
// Helper: Count Plugin Files
//==============================================================================

int PluginScanner::countPluginFiles(const juce::FileSearchPath& searchPath)
{
    int count = 0;
    auto& formatManager = owner.getFormatManager();

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format != nullptr && format->canScanForPlugins())
        {
            juce::StringArray files = format->searchPathsForPlugins(searchPath, true, false);
            count += files.size();
        }
    }

    return count;
}
