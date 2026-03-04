#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"

/**
 * GlobalControlsBar - BPM, Master Volume, Folder Controls
 *
 * Professional design with consistent styling
 */
class GlobalControlsBar : public juce::Component
{
public:
    GlobalControlsBar();
    ~GlobalControlsBar() override = default;

    void resized() override;

    // Callbacks
    std::function<void(double)> onBpmChanged;
    std::function<void(float)> onMasterVolumeChanged;
    std::function<void()> onFolderButtonClicked;
    std::function<void(const juce::String&)> onFolderChanged;

    // Components - getters only (created internally)
    juce::Slider* getBpmSlider() const { return bpmSlider.get(); }
    juce::Slider* getMasterVolumeSlider() const { return masterVolumeSlider.get(); }
    juce::TextButton* getFolderButton() const { return folderButton.get(); }
    juce::Label* getFolderLabel() const { return folderLabel.get(); }

private:
    std::unique_ptr<juce::Slider> bpmSlider;
    std::unique_ptr<juce::Slider> masterVolumeSlider;
    std::unique_ptr<juce::TextButton> folderButton;
    std::unique_ptr<juce::Label> folderLabel;

    void layoutComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalControlsBar)
};
