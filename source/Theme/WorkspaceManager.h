#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <vector>
#include <functional>
#include "WorkspacePreset.h"
#include "ThemeManager.h"

class MainComponent;
class AudioRoutingGraph;

/**
 * WorkspaceManager - Verwaltet Workspace-Presets
 *
 * Features:
 * - Speichern/Laden von Workspace-Konfigurationen
 * - Persistenz in AppData
 * - Auto-Save beim Beenden
 * - Callbacks für UI-Updates
 */
class WorkspaceManager
{
public:
    WorkspaceManager();
    ~WorkspaceManager();

    // Singleton
    static WorkspaceManager& getInstance();

    // ============================================================
    // Preset Management
    // ============================================================

    /** Get all available presets. */
    const std::vector<WorkspacePreset>& getPresets() const { return presets; }

    /** Get preset names for ComboBox. */
    juce::StringArray getPresetNames() const;

    /** Get preset by name. */
    const WorkspacePreset* getPresetByName(const juce::String& name) const;

    /** Get preset by index. */
    const WorkspacePreset* getPresetByIndex(int index) const;

    /** Get currently active preset. */
    const WorkspacePreset* getCurrentPreset() const { return currentPresetIndex >= 0 ? &presets[currentPresetIndex] : nullptr; }

    /** Get current preset index. */
    int getCurrentPresetIndex() const { return currentPresetIndex; }

    // ============================================================
    // Save/Load Operations
    // ============================================================

    /** Load a preset by index. Returns true on success. */
    bool loadPreset(int index);

    /** Load a preset by name. Returns true on success. */
    bool loadPreset(const juce::String& name);

    /** Save current state as a new preset. */
    void saveCurrentAsPreset(const juce::String& name, const juce::String& description = "");

    /** Update existing preset with current state. */
    void updatePreset(int index);

    /** Delete a preset. */
    void deletePreset(int index);

    /** Rename a preset. */
    void renamePreset(int index, const juce::String& newName);

    // ============================================================
    // State Capture/Apply
    // ============================================================

    /** Capture current workspace state from MainComponent. */
    WorkspacePreset captureCurrentState() const;

    /** Apply a preset to the application. */
    void applyPreset(const WorkspacePreset& preset);

    /** Set the MainComponent reference for state capture/apply. */
    void setMainComponent(MainComponent* mainComp) { mainComponent = mainComp; }

    /** Set AudioRoutingGraph reference for plugin state persistence. */
    void setAudioRoutingGraph(AudioRoutingGraph* graph) { audioRoutingGraph = graph; }

    // ============================================================
    // Persistence
    // ============================================================

    /** Save all presets to file. */
    void saveToFile();

    /** Load presets from file. */
    void loadFromFile();

    /** Get the presets file path. */
    juce::File getPresetsFile() const;

    /** Reset to default presets. */
    void resetToDefaults();

    // ============================================================
    // Callbacks
    // ============================================================

    /** Called when a preset is loaded. */
    std::function<void(const WorkspacePreset&)> onPresetLoaded;

    /** Called when a preset is saved. */
    std::function<void(const WorkspacePreset&)> onPresetSaved;

    /** Called when the preset list changes. */
    std::function<void()> onPresetListChanged;

private:
    std::vector<WorkspacePreset> presets;
    int currentPresetIndex = 0;
    MainComponent* mainComponent = nullptr;
    AudioRoutingGraph* audioRoutingGraph = nullptr;

    void initializeDefaultPresets();
    void ensureDefaultPresetsExist();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WorkspaceManager)
};
