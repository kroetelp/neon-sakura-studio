#include "GlobalControlsBar.h"
#include "../Theme/ThemeManager.h"

GlobalControlsBar::GlobalControlsBar()
{
    auto& theme = ThemeManager::getInstance();

    // BPM Slider
    bpmSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    bpmSlider->setRange(60, 200, 1);
    bpmSlider->setValue(120);
    bpmSlider->setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    bpmSlider->setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());
    bpmSlider->setColour(juce::Slider::textBoxTextColourId, theme.getTextSecondaryColor());

    // Master Volume Slider
    masterVolumeSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    masterVolumeSlider->setRange(0, 1, 0.01);
    masterVolumeSlider->setValue(0.8);
    masterVolumeSlider->setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    masterVolumeSlider->setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());
    masterVolumeSlider->setColour(juce::Slider::textBoxTextColourId, theme.getTextSecondaryColor());

    // Folder Button
    folderButton = std::make_unique<juce::TextButton>("Set Folder");
    folderButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    folderButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());

    // Folder Label
    folderLabel = std::make_unique<juce::Label>("folderLabel", "No folder selected");
    folderLabel->setColour(juce::Label::textColourId, theme.getTextSecondaryColor());

    // Audio Settings Button
    audioSettingsButton = std::make_unique<juce::TextButton>("Audio");
    audioSettingsButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    audioSettingsButton->setColour(juce::TextButton::textColourOnId, theme.getAccentColor());

    // Wooting Settings Button
    wootingSettingsButton = std::make_unique<juce::TextButton>("Wooting");
    wootingSettingsButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    wootingSettingsButton->setColour(juce::TextButton::textColourOnId, theme.getSuccessColor());

    addAndMakeVisible(*bpmSlider);
    addAndMakeVisible(*masterVolumeSlider);
    addAndMakeVisible(*folderButton);
    addAndMakeVisible(*folderLabel);
    addAndMakeVisible(*audioSettingsButton);
    addAndMakeVisible(*wootingSettingsButton);

    // Connect callbacks
    bpmSlider->onValueChange = [this]() {
        if (onBpmChanged)
            onBpmChanged(bpmSlider->getValue());
    };

    masterVolumeSlider->onValueChange = [this]() {
        if (onMasterVolumeChanged)
            onMasterVolumeChanged(static_cast<float>(masterVolumeSlider->getValue()));
    };

    folderButton->onClick = [this]() {
        if (onFolderButtonClicked)
            onFolderButtonClicked();
    };

    audioSettingsButton->onClick = [this]() {
        if (onAudioSettingsClicked)
            onAudioSettingsClicked();
    };

    wootingSettingsButton->onClick = [this]() {
        if (onWootingSettingsClicked)
            onWootingSettingsClicked();
    };
}

void GlobalControlsBar::resized()
{
    auto bounds = getLocalBounds();
    auto height = bounds.getHeight();

    // BPM Slider
    bpmSlider->setBounds(bounds.removeFromLeft(200));

    // Master Volume Slider
    masterVolumeSlider->setBounds(bounds.removeFromLeft(150));

    // Folder Button
    folderButton->setBounds(bounds.removeFromLeft(100));

    // Folder Label (remaining space)
    auto remainingWidth = bounds.getWidth();
    folderLabel->setBounds(bounds.removeFromLeft(remainingWidth * 0.6f));

    // Audio Settings Button
    audioSettingsButton->setBounds(bounds.removeFromLeft(60));

    // Wooting Settings Button (remaining)
    wootingSettingsButton->setBounds(bounds);
}
