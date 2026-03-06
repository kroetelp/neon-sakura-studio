#include "TransportBar.h"
#include "../Theme/ThemeManager.h"

TransportBar::TransportBar()
{
    // Create buttons
    playButton = std::make_unique<juce::TextButton>("Play");
    stopButton = std::make_unique<juce::TextButton>("Stop");

    // Style buttons
    auto& theme = ThemeManager::getInstance();
    playButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    playButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());
    stopButton->setColour(juce::TextButton::buttonColourId, theme.getButtonColor());
    stopButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());

    addAndMakeVisible(*playButton);
    addAndMakeVisible(*stopButton);

    // Connect callbacks
    playButton->onClick = [this]() {
        if (onPlay)
            onPlay();
    };

    stopButton->onClick = [this]() {
        if (onStop)
            onStop();
    };
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds();
    auto height = bounds.getHeight();
    auto buttonWidth = 70;
    auto gap = 5;

    // Play button
    playButton->setBounds(0, 0, buttonWidth, height);

    // Stop button
    stopButton->setBounds(buttonWidth + gap, 0, buttonWidth, height);
}

