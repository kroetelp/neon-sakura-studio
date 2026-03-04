#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"

/**
 * TrackToolsBar - Loop Length, Genre, Generate, FX Controls, Workspace Presets
 *
 * Professional design with grouped controls
 */
class TrackToolsBar : public juce::Component
{
public:
    TrackToolsBar();
    ~TrackToolsBar() override = default;

    void resized() override;

    // Callbacks
    std::function<void(int)> onLoopLengthChanged;
    std::function<void(int)> onGenreChanged;
    std::function<void(int)> onTargetTrackChanged;
    std::function<void()> onGenerateClicked;
    std::function<void(float)> onSwingChanged;
    std::function<void(float)> onReverbChanged;
    std::function<void(const juce::String&)> onWorkspacePresetChanged;

    // Components - getters only (created internally)
    juce::ComboBox* getLoopLengthCombo() const { return loopLengthCombo.get(); }
    juce::ComboBox* getGenreCombo() const { return genreCombo.get(); }
    juce::ComboBox* getTargetTrackCombo() const { return targetTrackCombo.get(); }
    juce::TextButton* getGenerateButton() const { return generateButton.get(); }
    juce::Slider* getSwingSlider() const { return swingSlider.get(); }
    juce::Slider* getReverbSlider() const { return reverbSlider.get(); }
    juce::ComboBox* getWorkspacePresetCombo() const { return workspacePresetCombo.get(); }

private:
    std::unique_ptr<juce::ComboBox> loopLengthCombo;
    std::unique_ptr<juce::ComboBox> genreCombo;
    std::unique_ptr<juce::ComboBox> targetTrackCombo;
    std::unique_ptr<juce::TextButton> generateButton;
    std::unique_ptr<juce::Slider> swingSlider;
    std::unique_ptr<juce::Slider> reverbSlider;
    std::unique_ptr<juce::ComboBox> workspacePresetCombo;

    void layoutComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackToolsBar)
};
