#include "WorkspaceManager.h"
#include "../MainComponent.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../VSTHost/VSTPluginManager.h"

WorkspaceManager::WorkspaceManager()
{
    loadFromFile();
    ensureDefaultPresetsExist();
}

WorkspaceManager::~WorkspaceManager()
{
    saveToFile();
}

WorkspaceManager& WorkspaceManager::getInstance()
{
    static WorkspaceManager instance;
    return instance;
}

juce::StringArray WorkspaceManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& preset : presets)
        names.add(preset.name);
    return names;
}

const WorkspacePreset* WorkspaceManager::getPresetByName(const juce::String& name) const
{
    for (const auto& preset : presets)
    {
        if (preset.name == name)
            return &preset;
    }
    return nullptr;
}

const WorkspacePreset* WorkspaceManager::getPresetByIndex(int index) const
{
    if (index >= 0 && index < static_cast<int>(presets.size()))
        return &presets[index];
    return nullptr;
}

bool WorkspaceManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return false;

    currentPresetIndex = index;
    applyPreset(presets[index]);

    if (onPresetLoaded)
        onPresetLoaded(presets[index]);

    return true;
}

bool WorkspaceManager::loadPreset(const juce::String& name)
{
    for (int i = 0; i < static_cast<int>(presets.size()); ++i)
    {
        if (presets[i].name == name)
            return loadPreset(i);
    }
    return false;
}

void WorkspaceManager::saveCurrentAsPreset(const juce::String& name, const juce::String& description)
{
    WorkspacePreset preset = captureCurrentState();
    preset.name = name;
    preset.description = description;
    preset.savedTime = juce::Time::getCurrentTime();

    presets.push_back(preset);
    currentPresetIndex = static_cast<int>(presets.size()) - 1;

    saveToFile();

    if (onPresetSaved)
        onPresetSaved(preset);

    if (onPresetListChanged)
        onPresetListChanged();
}

void WorkspaceManager::updatePreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    presets[index] = captureCurrentState();
    presets[index].savedTime = juce::Time::getCurrentTime();
    saveToFile();

    if (onPresetSaved)
        onPresetSaved(presets[index]);
}

void WorkspaceManager::deletePreset(int index)
{
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    // Don't delete default presets
    if (presets[index].name == "Default" ||
        presets[index].name == "Production" ||
        presets[index].name == "Live Performance" ||
        presets[index].name == "Composition")
        return;

    presets.erase(presets.begin() + index);

    if (currentPresetIndex >= static_cast<int>(presets.size()))
        currentPresetIndex = static_cast<int>(presets.size()) - 1;

    saveToFile();

    if (onPresetListChanged)
        onPresetListChanged();
}

void WorkspaceManager::renamePreset(int index, const juce::String& newName)
{
    if (index < 0 || index >= static_cast<int>(presets.size()))
        return;

    presets[index].name = newName;
    presets[index].savedTime = juce::Time::getCurrentTime();
    saveToFile();

    if (onPresetListChanged)
        onPresetListChanged();
}

WorkspacePreset WorkspaceManager::captureCurrentState() const
{
    WorkspacePreset preset;

    // Get theme settings from ThemeManager
    auto& theme = ThemeManager::getInstance();
    preset.themeType = theme.getCurrentTheme();
    preset.colorScheme = theme.getColorScheme();

    // If we have a MainComponent reference, capture more state
    if (mainComponent)
    {
        // Capture transport settings (we need to add getters to MainComponent)
        // For now, use defaults
        preset.bpm = 120.0;
        preset.masterVolume = 0.8f;
        preset.loopLength = 16;
    }

    // Capture AudioRoutingGraph state (plugin chains with their states)
    if (audioRoutingGraph)
    {
        auto graphState = audioRoutingGraph->getState();
        if (graphState)
        {
            // Convert to string for storage
            preset.audioRoutingState = graphState->toString();
            DBG("Captured AudioRoutingGraph state for preset");
        }
    }

    preset.savedTime = juce::Time::getCurrentTime();
    return preset;
}

void WorkspaceManager::applyPreset(const WorkspacePreset& preset)
{
    // Apply theme
    auto& theme = ThemeManager::getInstance();
    theme.setCurrentTheme(preset.themeType);
    theme.setColorScheme(preset.colorScheme);

    // If we have a MainComponent reference, apply more settings
    if (mainComponent)
    {
        // Apply transport and layout settings
        // This would require adding setter methods to MainComponent
        // For now, the theme is applied
    }

    // Restore AudioRoutingGraph state (plugin chains with their states)
    if (audioRoutingGraph && preset.audioRoutingState.isNotEmpty())
    {
        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(preset.audioRoutingState));

        if (xml && xml->getTagName() == "AUDIO_ROUTING_GRAPH")
        {
            // Get VSTPluginManager reference from MainComponent to load plugins
            auto& vstManager = mainComponent->getVSTPluginManager();

            // Create plugin load callback
            auto pluginLoadCallback = [&vstManager](const juce::String& fileOrIdentifier,
                                                   const juce::String& formatName)
            {
                return vstManager.loadPluginForState(fileOrIdentifier, formatName);
            };

            // Restore AudioRoutingGraph state
            audioRoutingGraph->setState(*xml, pluginLoadCallback);
            DBG("Restored AudioRoutingGraph state from preset");
        }
    }
}

juce::File WorkspaceManager::getPresetsFile() const
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto appFolder = appData.getChildFile("NeonSakuraStudio");

    if (!appFolder.exists())
        appFolder.createDirectory();

    return appFolder.getChildFile("workspace_presets.xml");
}

void WorkspaceManager::saveToFile()
{
    juce::ValueTree root("WorkspacePresets");

    for (const auto& preset : presets)
    {
        root.appendChild(preset.saveState(), nullptr);
    }

    auto file = getPresetsFile();
    std::unique_ptr<juce::XmlElement> xml(root.createXml());
    if (xml)
        xml->writeTo(file);
}

void WorkspaceManager::loadFromFile()
{
    presets.clear();

    auto file = getPresetsFile();
    if (!file.exists())
    {
        initializeDefaultPresets();
        return;
    }

    std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
    if (!xml)
    {
        initializeDefaultPresets();
        return;
    }

    juce::ValueTree root = juce::ValueTree::fromXml(*xml);
    if (!root.isValid() || root.getType() != juce::Identifier("WorkspacePresets"))
    {
        initializeDefaultPresets();
        return;
    }

    for (int i = 0; i < root.getNumChildren(); ++i)
    {
        WorkspacePreset preset;
        preset.restoreState(root.getChild(i));
        presets.push_back(preset);
    }

    if (presets.empty())
        initializeDefaultPresets();
}

void WorkspaceManager::initializeDefaultPresets()
{
    presets.push_back(WorkspacePreset::createDefault());
    presets.push_back(WorkspacePreset::createProduction());
    presets.push_back(WorkspacePreset::createLivePerformance());
    presets.push_back(WorkspacePreset::createComposition());
}

void WorkspaceManager::ensureDefaultPresetsExist()
{
    // Check if default presets exist
    bool hasDefault = false;
    bool hasProduction = false;
    bool hasLive = false;
    bool hasComposition = false;

    for (const auto& preset : presets)
    {
        if (preset.name == "Default") hasDefault = true;
        if (preset.name == "Production") hasProduction = true;
        if (preset.name == "Live Performance") hasLive = true;
        if (preset.name == "Composition") hasComposition = true;
    }

    // Add missing default presets at the beginning
    std::vector<WorkspacePreset> defaultPresets;

    if (!hasComposition)
        defaultPresets.push_back(WorkspacePreset::createComposition());
    if (!hasLive)
        defaultPresets.push_back(WorkspacePreset::createLivePerformance());
    if (!hasProduction)
        defaultPresets.push_back(WorkspacePreset::createProduction());
    if (!hasDefault)
        defaultPresets.push_back(WorkspacePreset::createDefault());

    // Insert at beginning (reverse order so Default is first)
    for (auto it = defaultPresets.rbegin(); it != defaultPresets.rend(); ++it)
    {
        presets.insert(presets.begin(), *it);
    }
}

void WorkspaceManager::resetToDefaults()
{
    presets.clear();
    initializeDefaultPresets();
    currentPresetIndex = 0;
    saveToFile();

    if (onPresetListChanged)
        onPresetListChanged();
}
