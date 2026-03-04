#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include "ThemeManager.h"

/**
 * WorkspacePreset - Speichert einen kompletten Workspace-Zustand
 *
 * Enthält:
 * - Theme-Einstellungen
 * - Panel-Sichtbarkeit
 * - Layout-Positionen
 * - Transport-Einstellungen
 */
struct WorkspacePreset
{
    juce::String name;
    juce::String description;
    juce::Time savedTime;

    // Theme
    ThemeManager::ThemeType themeType = ThemeManager::ThemeType::Professional;
    ProfessionalTheme::ColorScheme colorScheme = ProfessionalTheme::ColorScheme::DarkOrange;

    // Panel visibility
    bool showWavetable = true;
    bool showTimeline = true;
    bool showRhythmExplorer = false;
    bool showMelodyPanel = false;
    bool showPluginBrowser = false;

    // Layout sizes
    int synthHeight = 350;
    int bottomTabsHeight = 400;
    int activeTab = 0;  // 0 = Timeline, 1 = Step Sequencer

    // Transport settings
    double bpm = 120.0;
    float masterVolume = 0.8f;
    int loopLength = 16;

    // FX settings
    float swing = 0.0f;
    float reverb = 0.3f;

    // AudioRoutingGraph state (plugin chains with their states)
    juce::String audioRoutingState;  // Base64-encoded XML of AudioRoutingGraph state

    // Serialization
    juce::ValueTree saveState() const;
    void restoreState(const juce::ValueTree& state);

    // Default presets
    static WorkspacePreset createDefault();
    static WorkspacePreset createProduction();
    static WorkspacePreset createLivePerformance();
    static WorkspacePreset createComposition();
};
