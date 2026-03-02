#pragma once

#include "WavetablePreset.h"
#include <juce_core/juce_core.h>

class WavetableSynth;
class WavetableEngine;

/**
 * WavetablePresetManager - Manages loading, saving, and organizing presets
 *
 * Supports:
 * - Factory presets (bundled with application)
 * - User presets (saved in user documents)
 * - Preset browsing and loading
 * - Saving current synth state to preset
 */
class WavetablePresetManager
{
public:
    WavetablePresetManager();
    ~WavetablePresetManager() = default;

    // Directory management
    juce::File getPresetDirectory() const;
    juce::File getUserPresetDirectory() const;
    juce::File getFactoryPresetDirectory() const;

    // Ensure directories exist
    void ensureDirectoriesExist();

    // Single preset file operations
    bool savePresetToFile(const WavetablePreset& preset, const juce::File& file);
    WavetablePreset loadPresetFromFile(const juce::File& file);

    // Save to user directory with name
    bool saveUserPreset(const WavetablePreset& preset);

    // Get all available presets
    juce::Array<juce::File> getAllPresetFiles() const;
    juce::Array<juce::File> getFactoryPresetFiles() const;
    juce::Array<juce::File> getUserPresetFiles() const;

    // Load all presets as data
    juce::Array<WavetablePreset> loadAllPresets();

    // Create factory presets (call once on first run)
    void createFactoryPresets();

    // Apply preset to synth
    void applyPresetToSynth(const WavetablePreset& preset, WavetableSynth& synth);

    // Apply FX preset to engine
    void applyPresetToEngine(const WavetablePreset& preset, WavetableEngine& engine);

    // Extract preset from synth
    WavetablePreset extractPresetFromSynth(WavetableSynth& synth, const juce::String& name);

    // Extract FX preset from engine
    void extractFXFromEngine(WavetablePreset& preset, WavetableEngine& engine);

    // Get preset name from file
    juce::String getPresetNameFromFile(const juce::File& file) const;

    // Delete a preset file
    bool deletePreset(const juce::File& file);

    // Rename a preset file
    bool renamePreset(const juce::File& file, const juce::String& newName);

private:
    // Create individual factory presets
    WavetablePreset createBassPreset();
    WavetablePreset createLeadPreset();
    WavetablePreset createPadPreset();
    WavetablePreset createPluckPreset();
    WavetablePreset createWobblePreset();
    WavetablePreset createEvolvingPadPreset();
    WavetablePreset createAggressiveLeadPreset();
    WavetablePreset createSubBassPreset();
    WavetablePreset createModernFMBassPreset();     // FM + Waveshaper
    WavetablePreset createGritLeadPreset();         // AM + Foldback
    WavetablePreset createPressureResponsivePreset(); // For aftertouch demo

    juce::File applicationDirectory;
    juce::File userPresetDirectory;

    static constexpr const char* PRESET_EXTENSION = ".wtpreset";
    static constexpr const char* FACTORY_PRESET_DIR = "WavetablePresets";
    static constexpr const char* USER_PRESET_DIR = "NeonSakuraStudio/WavetablePresets";
};
