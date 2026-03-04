#include "VSTPluginManager.h"
#include "PluginScanner.h"
#include "PluginInstance.h"
#include <fstream>

//==============================================================================
// Simple File Logger for debugging
//==============================================================================
namespace
{
    std::unique_ptr<std::ofstream> logFile;
    std::atomic<bool> logInitialized{false};

    void initLogger()
    {
        if (logInitialized.exchange(true))
            return;

        auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
        auto file = tempDir.getChildFile("NeonSakuraStudio_debug.log");
        logFile = std::make_unique<std::ofstream>(file.getFullPathName().toStdString(), std::ios::trunc);
    }

    void LOG(const juce::String& message)
    {
        if (!logInitialized)
            initLogger();

        if (logFile && logFile->is_open())
        {
            auto timestamp = juce::Time::getCurrentTime().formatted("%H:%M:%S");
            *logFile << "[" << timestamp << "] " << message << std::endl;
            logFile->flush();
        }
    }
}

//==============================================================================
VSTPluginManager::VSTPluginManager()
{
    pluginGraph = std::make_unique<juce::AudioProcessorGraph>();
    scanner = std::make_unique<PluginScanner>(*this);
}

VSTPluginManager::~VSTPluginManager()
{
    releaseResources();

    // Save plugin list before shutdown
    savePluginList();
}

//==============================================================================
void VSTPluginManager::initialize()
{
    if (initialized)
        return;

    // Add default formats based on platform
    formatManager.addDefaultFormats();

    // Phase 4.4: Add LV2 plugin format support (JUCE 7.0+)
    // LV2 is not included in addDefaultFormats(), so we add it manually
#if JUCE_PLUGINHOST_LV2 && !JUCE_DISABLE_FEATURE_ALL
    if (auto* lv2Format = new juce::LV2PluginFormat())
    {
        formatManager.addFormat(lv2Format);
        DBG("LV2 plugin format added");

        // Configure LV2 search paths
        configureLV2SearchPaths(lv2Format);
    }
#else
    DBG("LV2 format not available (JUCE built without LV2 support)");
#endif

    // Log available formats
    juce::StringArray formats = getAvailableFormats();
    DBG("VSTPluginManager initialized with " + juce::String(formats.size()) + " formats:");
    for (const auto& format : formats)
        DBG("  - " + format);

    // Ensure AppData directory exists
    ensureAppDataDirectoryExists();

    // Try to load cached plugin list
    if (!loadPluginList())
    {
        DBG("No cached plugin list found. Will scan on first request.");
    }
    else
    {
        DBG("Loaded " + juce::String(knownPlugins.getNumTypes()) + " plugins from cache.");

        // Log blacklisted plugins
        auto blacklist = knownPlugins.getBlacklistedFiles();
        if (blacklist.size() > 0)
        {
            DBG("Blacklisted plugins (" + juce::String(blacklist.size()) + "):");
            for (const auto& file : blacklist)
            {
                DBG("  - " + file);
            }
        }
    }

    initialized = true;
}

//==============================================================================
juce::StringArray VSTPluginManager::getAvailableFormats() const
{
    juce::StringArray formats;
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
        formats.add(formatManager.getFormat(i)->getName());
    return formats;
}

juce::AudioPluginFormat* VSTPluginManager::getFormat(const juce::String& formatName) const
{
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format->getName().equalsIgnoreCase(formatName))
            return format;
    }
    return nullptr;
}

//==============================================================================
void VSTPluginManager::loadPlugin(const juce::File& pluginPath,
                                   std::function<void(std::unique_ptr<PluginInstance>)> callback)
{
    if (!initialized)
    {
        DBG("VSTPluginManager not initialized!");
        callback(nullptr);
        return;
    }

    auto path = pluginPath.getFullPathName();
    DBG("Loading plugin from file: " + path);

    // Check if path is a VST3 bundle (directory with .vst3 extension on Windows)
    bool isVST3Bundle = pluginPath.isDirectory() && path.endsWith(".vst3");
    DBG("Is VST3 Bundle: " + juce::String(isVST3Bundle ? "yes" : "no"));

    // First check if we already know this plugin
    for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
    {
        if (auto* desc = knownPlugins.getType(i))
        {
            if (desc->fileOrIdentifier == path)
            {
                DBG("Found in known plugins cache");
                loadPlugin(*desc, callback);
                return;
            }
        }
    }

    // Try each format
    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);
        if (format == nullptr)
            continue;

        DBG("Checking format: " + format->getName());
        DBG("  canScanForPlugins: " + juce::String(format->canScanForPlugins() ? "yes" : "no"));
        DBG("  fileMightContainThisPluginType: " + juce::String(format->fileMightContainThisPluginType(path) ? "yes" : "no"));

        // For VST3 bundles, skip fileMightContainThisPluginType check and try directly
        if (isVST3Bundle && format->getName() == "VST3")
        {
            DBG("========================================");
            DBG("VST3 Bundle Detection");
            DBG("========================================");
            DBG("Trying VST3 format directly for bundle...");
            DBG("  Plugin path: " + path);
            DBG("  Plugin exists: " + juce::String(juce::File(path).exists() ? "yes" : "no"));
            DBG("  Plugin is directory: " + juce::String(juce::File(path).isDirectory() ? "yes" : "no"));

            // Debug: Print bundle structure
            juce::File bundlePath(path);
            DBG("  Bundle directory contents:");
            juce::Array<juce::File> bundleFiles = bundlePath.findChildFiles(
                juce::File::findFilesAndDirectories, false);
            for (const auto& f : bundleFiles)
                DBG("    - " + f.getFileName());

            // Check Contents directory
            auto contentsDir = bundlePath.getChildFile("Contents");
            DBG("  Contents directory exists: " + juce::String(contentsDir.exists() ? "yes" : "no"));

            if (contentsDir.exists())
            {
                DBG("  Contents directory contents:");
                juce::Array<juce::File> contentsFiles = contentsDir.findChildFiles(
                    juce::File::findFilesAndDirectories, false);
                for (const auto& f : contentsFiles)
                    DBG("    - " + f.getFileName());

                // Check x86_64-win directory
                auto platformDir = contentsDir.getChildFile("x86_64-win");
                DBG("  x86_64-win directory exists: " + juce::String(platformDir.exists() ? "yes" : "no"));

                if (platformDir.exists())
                {
                    DBG("  x86_64-win directory contents:");
                    juce::Array<juce::File> platformFiles = platformDir.findChildFiles(
                        juce::File::findFiles, false, "*.vst3");
                    for (const auto& f : platformFiles)
                        DBG("    - " + f.getFileName() + " (size: " + juce::String(f.getSize()) + " bytes)");
                }
            }

            juce::OwnedArray<juce::PluginDescription> descriptions;
            format->findAllTypesForFile(descriptions, path);

            DBG("  Found " + juce::String(descriptions.size()) + " descriptions");

            if (!descriptions.isEmpty())
            {
                DBG("========================================");
                DBG("  Plugin Description Details:");
                DBG("========================================");
                DBG("  Name: " + descriptions[0]->name);
                DBG("  Manufacturer: " + descriptions[0]->manufacturerName);
                DBG("  Category: " + descriptions[0]->category);
                DBG("  fileOrIdentifier: " + descriptions[0]->fileOrIdentifier);
                DBG("  pluginFormatName: " + descriptions[0]->pluginFormatName);
                DBG("  numInputChannels: " + juce::String(descriptions[0]->numInputChannels));
                DBG("  numOutputChannels: " + juce::String(descriptions[0]->numOutputChannels));
                DBG("========================================");

                knownPlugins.addType(*descriptions[0]);
                loadPlugin(*descriptions[0], callback);
                return;
            }
            else
            {
                DBG("  No descriptions found for VST3 bundle");
                DBG("  Trying alternative: look for DLL in Contents/x86_64-win...");

                // Try to find the actual DLL in the bundle structure
                // Note: On Windows, the DLL file inside the bundle ALSO has .vst3 extension
                auto dllPath = juce::File(path).getChildFile("Contents").getChildFile("x86_64-win").getChildFile("Serum2.vst3");
                DBG("  Checking DLL path: " + dllPath.getFullPathName());
                DBG("  DLL exists: " + juce::String(dllPath.exists() ? "yes" : "no"));

                if (dllPath.exists())
                {
                    DBG("  DLL size: " + juce::String(dllPath.getSize()) + " bytes");
                    DBG("  Loading DLL...");

                    format->findAllTypesForFile(descriptions, dllPath.getFullPathName());
                    DBG("  Found " + juce::String(descriptions.size()) + " descriptions from DLL");

                    if (!descriptions.isEmpty())
                    {
                        DBG("  Successfully loaded from DLL!");
                        DBG("  Adding description: " + descriptions[0]->name);
                        knownPlugins.addType(*descriptions[0]);
                        loadPlugin(*descriptions[0], callback);
                        return;
                    }
                }
                else
                {
                    DBG("  ERROR: DLL not found at expected path!");
                    DBG("  Searching for any .vst3 files in bundle...");

                    // Try to find any .vst3 file in the bundle
                    juce::Array<juce::File> vst3Files = juce::File(path).findChildFiles(
                        juce::File::findFiles, true, "*.vst3");
                    DBG("  Found " + juce::String(vst3Files.size()) + " .vst3 files in bundle:");
                    for (const auto& f : vst3Files)
                        DBG("    - " + f.getFullPathName());

                    if (!vst3Files.isEmpty())
                    {
                        DBG("  Trying with first found file: " + vst3Files[0].getFullPathName());
                        format->findAllTypesForFile(descriptions, vst3Files[0].getFullPathName());

                        if (!descriptions.isEmpty())
                        {
                            DBG("  Successfully loaded!");
                            knownPlugins.addType(*descriptions[0]);
                            loadPlugin(*descriptions[0], callback);
                            return;
                        }
                    }
                }
            }
            DBG("========================================");
        }
        else if (format->canScanForPlugins() && format->fileMightContainThisPluginType(path))
        {
            DBG("Found matching format: " + format->getName());

            juce::OwnedArray<juce::PluginDescription> descriptions;
            format->findAllTypesForFile(descriptions, path);

            DBG("  Found " + juce::String(descriptions.size()) + " descriptions");

            if (!descriptions.isEmpty())
            {
                DBG("  Adding description: " + descriptions[0]->name);
                knownPlugins.addType(*descriptions[0]);

                loadPlugin(*descriptions[0], callback);
                return;
            }
        }
        // Special handling for VST3 bundles that might not be detected by fileMightContainThisPluginType
        // (because it checks for files, not directories)
        else if (format->getName() == "VST3" && juce::File(path).isDirectory())
        {
            DBG("Found VST3 format directory, trying direct scan...");
            juce::OwnedArray<juce::PluginDescription> descriptions;
            format->findAllTypesForFile(descriptions, path);

            DBG("  Found " + juce::String(descriptions.size()) + " descriptions from directory");

            if (!descriptions.isEmpty())
            {
                DBG("  Adding description: " + descriptions[0]->name);
                knownPlugins.addType(*descriptions[0]);

                loadPlugin(*descriptions[0], callback);
                return;
            }
        }
    }

    DBG("Failed to find plugin format for: " + path);
    callback(nullptr);
}

void VSTPluginManager::loadPlugin(const juce::PluginDescription& desc,
                                   std::function<void(std::unique_ptr<PluginInstance>)> callback)
{
    if (!initialized)
    {
        DBG("VSTPluginManager: ERROR - Manager not initialized!");
        callback(nullptr);
        return;
    }

    DBG("========================================");
    DBG("Loading Plugin Instance");
    DBG("========================================");
    DBG("  Plugin name: " + desc.name);
    DBG("  Format: " + desc.pluginFormatName);
    DBG("  fileOrIdentifier: " + desc.fileOrIdentifier);
    DBG("  Category: " + desc.category);
    DBG("  Manufacturer: " + desc.manufacturerName);
    DBG("  sampleRate: " + juce::String(currentSampleRate));
    DBG("  blockSize: " + juce::String(currentBlockSize));
    DBG("  numInputChannels: " + juce::String(desc.numInputChannels));
    DBG("  numOutputChannels: " + juce::String(desc.numOutputChannels));
    DBG("========================================");

    // Debug: Check if file exists
    juce::File pluginFile(desc.fileOrIdentifier);
    DBG("File check:");
    DBG("  File exists: " + juce::String(pluginFile.exists() ? "yes" : "no"));
    DBG("  Is directory: " + juce::String(pluginFile.isDirectory() ? "yes" : "no"));
    DBG("  Is file: " + juce::String(pluginFile.existsAsFile() ? "yes" : "no"));
    if (pluginFile.existsAsFile())
        DBG("  File size: " + juce::String(pluginFile.getSize()) + " bytes");

    formatManager.createPluginInstanceAsync(
        desc,
        currentSampleRate,
        currentBlockSize,
        [this, callback, desc, pluginFile](std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
            DBG("========================================");
            DBG("Plugin Load Result");
            DBG("========================================");
            if (instance)
            {
                DBG("SUCCESS!");
                DBG("  Name: " + instance->getName());
                DBG("  numInputChannels: " + juce::String(instance->getTotalNumInputChannels()));
                DBG("  numOutputChannels: " + juce::String(instance->getTotalNumOutputChannels()));
                DBG("  numParameters: " + juce::String(instance->getParameters().size()));
                DBG("  hasEditor: " + juce::String(instance->hasEditor() ? "yes" : "no"));
                DBG("========================================");
                callback(std::make_unique<PluginInstance>(std::move(instance)));
            }
            else
            {
                DBG("FAILED!");
                DBG("  Error: " + error);
                DBG("  Plugin name: " + desc.name);
                DBG("  Plugin file: " + desc.fileOrIdentifier);

                // Debug: Check what formats are available
                DBG("  Available formats in formatManager:");
                for (int i = 0; i < formatManager.getNumFormats(); ++i)
                {
                    auto* fmt = formatManager.getFormat(i);
                    DBG("    - " + fmt->getName());
                }

                // Don't add to blacklist - this allows retry for debugging
                // knownPlugins.addToBlacklist(desc.fileOrIdentifier);
                callback(nullptr);
            }
            DBG("========================================");
        }
    );
}

std::unique_ptr<PluginInstance> VSTPluginManager::loadPluginSync(const juce::PluginDescription& desc)
{
    if (!initialized)
        return nullptr;

    juce::String error;
    auto instance = formatManager.createPluginInstance(desc, currentSampleRate, currentBlockSize, error);

    if (instance)
    {
        DBG("Successfully loaded (sync): " + instance->getName());
        return std::make_unique<PluginInstance>(std::move(instance));
    }

    DBG("Failed to load plugin (sync): " + error);
    knownPlugins.addToBlacklist(desc.fileOrIdentifier);
    return nullptr;
}

//==============================================================================
std::unique_ptr<juce::AudioPluginInstance> VSTPluginManager::loadPluginForState(
    const juce::String& fileOrIdentifier,
    const juce::String& formatName)
{
    if (!initialized)
        return nullptr;

    DBG("Loading plugin for state restoration: " + fileOrIdentifier);

    // First check if we have this plugin in the known list
    juce::PluginDescription targetDesc;
    bool foundInCache = false;

    for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
    {
        auto* desc = knownPlugins.getType(i);
        if (desc && desc->fileOrIdentifier == fileOrIdentifier)
        {
            targetDesc = *desc;
            foundInCache = true;
            DBG("  Found in plugin cache: " + desc->name);
            break;
        }
    }

    // If not in cache, try to load by creating a minimal description
    if (!foundInCache)
    {
        DBG("  Plugin not in cache, attempting direct load...");
        targetDesc.fileOrIdentifier = fileOrIdentifier;
        targetDesc.pluginFormatName = formatName;
    }

    // Try to load the plugin
    juce::String error;
    auto instance = formatManager.createPluginInstance(targetDesc, currentSampleRate, currentBlockSize, error);

    if (instance)
    {
        DBG("  Successfully loaded plugin for state: " + instance->getName());
        return instance;
    }

    DBG("  Failed to load plugin for state: " + error);
    knownPlugins.addToBlacklist(fileOrIdentifier);
    return nullptr;
}

//==============================================================================
juce::AudioProcessorGraph::NodeID VSTPluginManager::addPluginToGraph(std::unique_ptr<PluginInstance> instance)
{
    if (!instance || !instance->getAudioPluginInstance())
        return {};

    auto* plugin = instance->getAudioPluginInstance();
    plugin->prepareToPlay(currentSampleRate, currentBlockSize);

    // Create AudioProcessorGraph node - transfer ownership
    // Note: We need to release the plugin from the wrapper
    auto pluginPtr = instance->releasePlugin();
    auto node = pluginGraph->addNode(std::move(pluginPtr));

    DBG("Added plugin to graph: " + node->getProcessor()->getName() + " (NodeID: " + juce::String((int)node->nodeID.uid) + ")");

    return node->nodeID;
}

void VSTPluginManager::removePluginFromGraph(juce::AudioProcessorGraph::NodeID nodeId)
{
    if (auto node = pluginGraph->getNodeForId(nodeId))
    {
        DBG("Removing plugin from graph: " + node->getProcessor()->getName());
    }
    pluginGraph->removeNode(nodeId);
}

juce::AudioProcessorGraph::Node::Ptr VSTPluginManager::getPluginNode(juce::AudioProcessorGraph::NodeID nodeId) const
{
    return pluginGraph->getNodeForId(nodeId);
}

void VSTPluginManager::clearGraph()
{
    pluginGraph->clear();
}

//==============================================================================
void VSTPluginManager::scanForPlugins()
{
    if (scanner && !scanner->isScanning())
    {
        DBG("Starting plugin scan...");
        scanner->startScan();
    }
}

void VSTPluginManager::scanForPlugins(const juce::FileSearchPath& searchPath)
{
    if (scanner && !scanner->isScanning())
    {
        DBG("Starting plugin scan in custom path...");
        scanner->startScan(searchPath);
    }
}

bool VSTPluginManager::isScanning() const
{
    return scanner && scanner->isScanning();
}

float VSTPluginManager::getScanProgress() const
{
    return scanner ? scanner->getProgress() : 0.0f;
}

//==============================================================================
juce::Array<juce::PluginDescription> VSTPluginManager::searchPlugins(const juce::String& searchTerm) const
{
    juce::Array<juce::PluginDescription> results;

    if (searchTerm.isEmpty())
    {
        // Return all plugins
        for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
            results.add(*knownPlugins.getType(i));
        return results;
    }

    // Case-insensitive search
    auto term = searchTerm.toLowerCase();

    for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
    {
        auto* desc = knownPlugins.getType(i);
        if (desc->name.toLowerCase().contains(term) ||
            desc->manufacturerName.toLowerCase().contains(term) ||
            desc->category.toLowerCase().contains(term) ||
            desc->descriptiveName.toLowerCase().contains(term))
        {
            results.add(*desc);
        }
    }

    return results;
}

juce::Array<juce::PluginDescription> VSTPluginManager::getPluginsByManufacturer(const juce::String& manufacturer) const
{
    juce::Array<juce::PluginDescription> results;

    for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
    {
        auto* desc = knownPlugins.getType(i);
        if (desc->manufacturerName.equalsIgnoreCase(manufacturer))
            results.add(*desc);
    }

    return results;
}

juce::Array<juce::PluginDescription> VSTPluginManager::getPluginsByCategory(const juce::String& category) const
{
    juce::Array<juce::PluginDescription> results;

    for (int i = 0; i < knownPlugins.getNumTypes(); ++i)
    {
        auto* desc = knownPlugins.getType(i);
        if (desc->category.containsIgnoreCase(category))
            results.add(*desc);
    }

    return results;
}

//==============================================================================
// Persistence (Save/Load Plugin List)
//==============================================================================

juce::File VSTPluginManager::getAppDataDirectory() const
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    return appData.getChildFile(PluginHostConfig::appDataFolderName);
}

juce::File VSTPluginManager::getPluginListFile() const
{
    return getAppDataDirectory().getChildFile(PluginHostConfig::pluginListFileName);
}

bool VSTPluginManager::ensureAppDataDirectoryExists()
{
    auto dir = getAppDataDirectory();
    if (!dir.exists())
    {
        auto result = dir.createDirectory();
        if (result.failed())
        {
            DBG("Failed to create AppData directory: " + result.getErrorMessage());
            return false;
        }
        DBG("Created AppData directory: " + dir.getFullPathName());
    }
    return true;
}

bool VSTPluginManager::savePluginList()
{
    ensureAppDataDirectoryExists();

    auto file = getPluginListFile();

    // Create XML from KnownPluginList
    std::unique_ptr<juce::XmlElement> xml(knownPlugins.createXml());

    if (xml == nullptr)
    {
        DBG("Failed to create XML from plugin list");
        return false;
    }

    // Add metadata
    xml->setAttribute("appVersion", "1.0.0");
    xml->setAttribute("saveTime", juce::Time::getCurrentTime().toISO8601(true));
    xml->setAttribute("numPlugins", knownPlugins.getNumTypes());

    // Write to file
    if (!xml->writeTo(file))
    {
        DBG("Failed to write plugin list to: " + file.getFullPathName());
        return false;
    }

    DBG("Saved " + juce::String(knownPlugins.getNumTypes()) + " plugins to: " + file.getFullPathName());
    return true;
}

bool VSTPluginManager::loadPluginList()
{
    auto file = getPluginListFile();

    if (!file.exists())
    {
        DBG("Plugin list file does not exist: " + file.getFullPathName());
        return false;
    }

    // Parse XML
    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));

    if (xml == nullptr)
    {
        DBG("Failed to parse plugin list XML");
        return false;
    }

    // Load into KnownPluginList (recreateFromXml returns void)
    knownPlugins.recreateFromXml(*xml);

    // Log info
    auto saveTime = xml->getStringAttribute("saveTime", "Unknown");
    int numPlugins = xml->getIntAttribute("numPlugins", 0);

    DBG("Loaded plugin list from " + saveTime);
    DBG("  - " + juce::String(knownPlugins.getNumTypes()) + " plugins");
    DBG("  - " + juce::String(knownPlugins.getBlacklistedFiles().size()) + " blacklisted");

    return knownPlugins.getNumTypes() > 0 || numPlugins == 0;
}

bool VSTPluginManager::hasCachedPluginList() const
{
    return getPluginListFile().exists();
}

void VSTPluginManager::clearPluginListCache()
{
    auto file = getPluginListFile();
    if (file.exists())
    {
        file.deleteFile();
        DBG("Cleared plugin list cache");
    }
    knownPlugins.clear();
    knownPlugins.clearBlacklistedFiles();
}

juce::StringArray VSTPluginManager::getBlacklist() const
{
    return knownPlugins.getBlacklistedFiles();
}

void VSTPluginManager::removeFromBlacklist(const juce::String& fileOrIdentifier)
{
    knownPlugins.removeFromBlacklist(fileOrIdentifier);
    DBG("Removed from blacklist: " + fileOrIdentifier);
}

//==============================================================================
void VSTPluginManager::prepareToPlay(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize;

    if (pluginGraph)
        pluginGraph->prepareToPlay(sampleRate, blockSize);
}

void VSTPluginManager::releaseResources()
{
    if (pluginGraph)
        pluginGraph->releaseResources();
}

void VSTPluginManager::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (pluginGraph)
        pluginGraph->processBlock(buffer, midiMessages);
}

//==============================================================================
// Callbacks
//==============================================================================

void VSTPluginManager::setScanProgressCallback(std::function<void(float)> callback)
{
    if (scanner)
        scanner->setProgressCallback(callback);
}

void VSTPluginManager::setScanCompleteCallback(std::function<void(int)> callback)
{
    if (scanner)
        scanner->setCompletionCallback([this, callback](int numPlugins)
        {
            onScanComplete(numPlugins);
            if (callback)
                callback(numPlugins);
        });
}

void VSTPluginManager::setPluginListChangedCallback(std::function<void()> callback)
{
    pluginListChangedCallback = callback;
}

void VSTPluginManager::onScanComplete(int numPluginsFound)
{
    DBG("Scan complete! Found " + juce::String(numPluginsFound) + " new plugins.");

    // Save the updated plugin list
    savePluginList();

    // Notify listeners
    if (pluginListChangedCallback)
        pluginListChangedCallback();
}

//==============================================================================
// LV2 Support (Phase 4.4)
//==============================================================================

void VSTPluginManager::configureLV2SearchPaths(juce::AudioPluginFormat* lv2Format)
{
#if JUCE_PLUGINHOST_LV2 && !JUCE_DISABLE_FEATURE_ALL
    juce::FileSearchPath lv2SearchPath;

    // Platform-specific LV2 paths
#if JUCE_WINDOWS
    // Windows LV2 paths
    auto programFiles = juce::File::getSpecialLocation(juce::File::globalApplicationsDirectory);
    auto localAppData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);

    lv2SearchPath.addIfNotAlready(programFiles.getChildFile("LV2"));
    lv2SearchPath.addIfNotAlready(localAppData.getChildFile("lv2"));

    DBG("LV2 search paths (Windows):");
#elif JUCE_MAC
    // macOS LV2 paths
    auto libraryPath = juce::File("/Library/Audio/Plug-Ins/LV2");
    auto userLibraryPath = juce::File::getUserHomeDirectory().getChildFile("Library/Audio/Plug-Ins/LV2");

    lv2SearchPath.addIfNotAlready(libraryPath);
    lv2SearchPath.addIfNotAlready(userLibraryPath);

    DBG("LV2 search paths (macOS):");
#elif JUCE_LINUX
    // Linux LV2 paths (following LV2 spec)
    auto homeDir = juce::File::getUserHomeDirectory();
    auto usrLocalLib = juce::File("/usr/local/lib/lv2");
    auto usrLib = juce::File("/usr/lib/lv2");
    auto optLv2 = juce::File("/opt/lv2");

    lv2SearchPath.addIfNotAlready(homeDir.getChildFile(".lv2"));
    lv2SearchPath.addIfNotAlready(usrLocalLib);
    lv2SearchPath.addIfNotAlready(usrLib);
    lv2SearchPath.addIfNotAlready(optLv2);

    DBG("LV2 search paths (Linux):");
#endif

    // Log all configured paths
    for (int i = 0; i < lv2SearchPath.getNumPaths(); ++i)
    {
        DBG("  - " + lv2SearchPath[i].getFullPathName());
    }

    // Note: LV2 search paths are typically configured via environment variables
    // LV2_PATH on Linux/macOS, or registry on Windows
    // The LV2PluginFormat will automatically scan standard locations

    DBG("LV2 search paths configured");
#endif
}
