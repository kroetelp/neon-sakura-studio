// ============================================================================
// TrackViewComponent.h - Track-basierte View für UnifiedSequencerPanel
// ============================================================================
//
// Diese View zeigt die Tracks vertikal mit allen Controls,
// ähnlich zum ursprünglichen TrackComponent Layout.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur.h>
#include <functional>
#include <memory>
#include "../TrackModel.h"
#include "../TrackType.h"
#include "../Theme/ThemeManager.h"

// Forward declarations
class UnifiedSequencerModel;
class SampleManager;

/**
 * TrackRowComponent - Eine einzelne Track-Row
 */
class TrackRowComponent : public juce::Component
{
public:
    TrackRowComponent(int trackIndex);
    ~TrackRowComponent() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Data Bindung ===
    void setModel(UnifiedSequencerModel* model);

    // === Step Buttons ===
    void setStepActive(int step, bool active);
    void setStepState(int step, const StepModifierState& state);

    // === Callbacks ===
    std::function<void(int step, bool active)> onStepClicked;
    std::function<void(int step)> onStepRightClicked;

    // === Track Controls ===
    juce::Slider& getVolumeSlider() { return volumeSlider; }
    juce::Slider& getPitchSlider() { return pitchSlider; }
    juce::Slider& getAttackSlider() { return attackSlider; }
    juce::Slider& getDecaySlider() { return decaySlider; }
    juce::Label& getTrackLabel() { return trackLabel; }

private:
    int trackIndex = 0;
    UnifiedSequencerModel* model = nullptr;

    // Track Label
    juce::Label trackLabel;

    // Track Controls (Volume, Pitch, etc.)
    juce::Slider volumeSlider;
    juce::Slider pitchSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;

    juce::Label volumeLabel;
    juce::Label pitchLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;

    // Step Buttons
    std::array<std::unique_ptr<juce::TextButton>, 64> stepButtons;

    // Colors
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getBackgroundColor(); }
    juce::Colour getStepInactive() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    void setupSlider(juce::Slider& slider, double min, double max, double initial, const juce::String& labelText);
    void createStepButtons();
    void updateStepButtonState(int step);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackRowComponent)
};

/**
 * TrackViewComponent - Container für alle Track Rows
 */
class TrackViewComponent : public juce::Component
{
public:
    TrackViewComponent();
    ~TrackViewComponent() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Model Bindung ===
    void setModel(UnifiedSequencerModel* model);
    void setSampleManager(SampleManager* manager);

    // === Callbacks ===
    std::function<void(int trackIndex)> onTrackSelected;

private:
    UnifiedSequencerModel* model = nullptr;
    SampleManager* sampleManager = nullptr;

    // Track Rows
    std::array<std::unique_ptr<TrackRowComponent>, 8> trackRows;

    // Header Labels
    juce::Label headerLabel;

    // Setup
    void createTrackRows();
    void setupHeader();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackViewComponent)
};
