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

    addAndMakeVisible(*bpmSlider);
    addAndMakeVisible(*masterVolumeSlider);
    addAndMakeVisible(*folderButton);
    addAndMakeVisible(*folderLabel);
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
    folderLabel->setBounds(bounds);
}
