#include "WorkspacePreset.h"

juce::ValueTree WorkspacePreset::saveState() const
{
    juce::ValueTree state("WorkspacePreset");

    // Metadata
    state.setProperty("name", name, nullptr);
    state.setProperty("description", description, nullptr);
    state.setProperty("savedTime", savedTime.toMilliseconds(), nullptr);

    // Theme
    state.setProperty("themeType", static_cast<int>(themeType), nullptr);
    state.setProperty("colorScheme", static_cast<int>(colorScheme), nullptr);

    // Panel visibility
    state.setProperty("showWavetable", showWavetable, nullptr);
    state.setProperty("showTimeline", showTimeline, nullptr);
    state.setProperty("showRhythmExplorer", showRhythmExplorer, nullptr);
    state.setProperty("showMelodyPanel", showMelodyPanel, nullptr);
    state.setProperty("showPluginBrowser", showPluginBrowser, nullptr);

    // Layout
    state.setProperty("synthHeight", synthHeight, nullptr);
    state.setProperty("bottomTabsHeight", bottomTabsHeight, nullptr);
    state.setProperty("activeTab", activeTab, nullptr);

    // Transport
    state.setProperty("bpm", bpm, nullptr);
    state.setProperty("masterVolume", masterVolume, nullptr);
    state.setProperty("loopLength", loopLength, nullptr);

    // FX
    state.setProperty("swing", swing, nullptr);
    state.setProperty("reverb", reverb, nullptr);

    // AudioRoutingGraph state (plugin chains with their states)
    state.setProperty("audioRoutingState", audioRoutingState, nullptr);

    return state;
}

void WorkspacePreset::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType() != juce::Identifier("WorkspacePreset"))
        return;

    // Metadata
    name = state.getProperty("name", "Untitled");
    description = state.getProperty("description", "");
    savedTime = juce::Time(state.getProperty("savedTime", juce::Time::getCurrentTime().toMilliseconds()));

    // Theme
    themeType = static_cast<ThemeManager::ThemeType>(static_cast<int>(state.getProperty("themeType", 1)));
    colorScheme = static_cast<ProfessionalTheme::ColorScheme>(static_cast<int>(state.getProperty("colorScheme", 0)));

    // Panel visibility
    showWavetable = state.getProperty("showWavetable", true);
    showTimeline = state.getProperty("showTimeline", true);
    showRhythmExplorer = state.getProperty("showRhythmExplorer", false);
    showMelodyPanel = state.getProperty("showMelodyPanel", false);
    showPluginBrowser = state.getProperty("showPluginBrowser", false);

    // Layout
    synthHeight = state.getProperty("synthHeight", 350);
    bottomTabsHeight = state.getProperty("bottomTabsHeight", 400);
    activeTab = state.getProperty("activeTab", 0);

    // Transport
    bpm = state.getProperty("bpm", 120.0);
    masterVolume = state.getProperty("masterVolume", 0.8f);
    loopLength = state.getProperty("loopLength", 16);

    // FX
    swing = state.getProperty("swing", 0.0f);
    reverb = state.getProperty("reverb", 0.3f);

    // AudioRoutingGraph state (plugin chains with their states)
    audioRoutingState = state.getProperty("audioRoutingState", "").toString();
}

WorkspacePreset WorkspacePreset::createDefault()
{
    WorkspacePreset preset;
    preset.name = "Default";
    preset.description = "Standard workspace layout";
    preset.savedTime = juce::Time::getCurrentTime();
    preset.themeType = ThemeManager::ThemeType::Professional;
    preset.colorScheme = ProfessionalTheme::ColorScheme::DarkOrange;
    preset.showWavetable = true;
    preset.showTimeline = true;
    preset.showRhythmExplorer = false;
    preset.showMelodyPanel = false;
    preset.showPluginBrowser = false;
    preset.synthHeight = 350;
    preset.bottomTabsHeight = 400;
    preset.activeTab = 0;
    preset.bpm = 120.0;
    preset.masterVolume = 0.8f;
    preset.loopLength = 16;
    preset.swing = 0.0f;
    preset.reverb = 0.3f;
    return preset;
}

WorkspacePreset WorkspacePreset::createProduction()
{
    WorkspacePreset preset;
    preset.name = "Production";
    preset.description = "Optimized for music production";
    preset.savedTime = juce::Time::getCurrentTime();
    preset.themeType = ThemeManager::ThemeType::Professional;
    preset.colorScheme = ProfessionalTheme::ColorScheme::DarkBlue;
    preset.showWavetable = true;
    preset.showTimeline = true;
    preset.showRhythmExplorer = true;
    preset.showMelodyPanel = true;
    preset.showPluginBrowser = false;
    preset.synthHeight = 300;
    preset.bottomTabsHeight = 350;
    preset.activeTab = 0;
    preset.bpm = 128.0;
    preset.masterVolume = 0.75f;
    preset.loopLength = 32;
    preset.swing = 0.0f;
    preset.reverb = 0.2f;
    return preset;
}

WorkspacePreset WorkspacePreset::createLivePerformance()
{
    WorkspacePreset preset;
    preset.name = "Live Performance";
    preset.description = "Clean layout for live use";
    preset.savedTime = juce::Time::getCurrentTime();
    preset.themeType = ThemeManager::ThemeType::Professional;
    preset.colorScheme = ProfessionalTheme::ColorScheme::DarkGray;
    preset.showWavetable = true;
    preset.showTimeline = false;
    preset.showRhythmExplorer = false;
    preset.showMelodyPanel = false;
    preset.showPluginBrowser = false;
    preset.synthHeight = 500;
    preset.bottomTabsHeight = 300;
    preset.activeTab = 1;
    preset.bpm = 130.0;
    preset.masterVolume = 0.85f;
    preset.loopLength = 16;
    preset.swing = 0.0f;
    preset.reverb = 0.15f;
    return preset;
}

WorkspacePreset WorkspacePreset::createComposition()
{
    WorkspacePreset preset;
    preset.name = "Composition";
    preset.description = "Optimized for composing and arranging";
    preset.savedTime = juce::Time::getCurrentTime();
    preset.themeType = ThemeManager::ThemeType::Professional;
    preset.colorScheme = ProfessionalTheme::ColorScheme::DarkOrange;
    preset.showWavetable = true;
    preset.showTimeline = true;
    preset.showRhythmExplorer = true;
    preset.showMelodyPanel = true;
    preset.showPluginBrowser = true;
    preset.synthHeight = 250;
    preset.bottomTabsHeight = 450;
    preset.activeTab = 0;
    preset.bpm = 100.0;
    preset.masterVolume = 0.7f;
    preset.loopLength = 64;
    preset.swing = 0.1f;
    preset.reverb = 0.35f;
    return preset;
}
