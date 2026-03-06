#include "TrackToolsBar.h"
#include "../Theme/ThemeManager.h"

TrackToolsBar::TrackToolsBar()
{
    auto& theme = ThemeManager::getInstance();

    // Loop Length Combo
    loopLengthCombo = std::make_unique<juce::ComboBox>();
    loopLengthCombo->addItem("16", 1);
    loopLengthCombo->addItem("32", 2);
    loopLengthCombo->addItem("48", 3);
    loopLengthCombo->addItem("64", 4);
    loopLengthCombo->setSelectedId(1);
    loopLengthCombo->setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    loopLengthCombo->setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    loopLengthCombo->setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());

    // Genre Combo
    genreCombo = std::make_unique<juce::ComboBox>();
    genreCombo->addItem("Techno", 1);
    genreCombo->addItem("House", 2);
    genreCombo->addItem("Trap", 3);
    genreCombo->addItem("DnB", 4);
    genreCombo->addItem("Ambient", 5);
    genreCombo->addItem("Garage", 6);
    genreCombo->setSelectedId(1);
    genreCombo->setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    genreCombo->setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    genreCombo->setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());

    // Target Track Combo
    targetTrackCombo = std::make_unique<juce::ComboBox>();
    targetTrackCombo->addItem("All Tracks", 1);
    for (int i = 1; i <= 8; ++i)
        targetTrackCombo->addItem("Track " + juce::String(i), i + 1);
    targetTrackCombo->setSelectedId(1);
    targetTrackCombo->setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    targetTrackCombo->setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    targetTrackCombo->setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());

    // Generate Button
    generateButton = std::make_unique<juce::TextButton>("Generate");
    generateButton->setColour(juce::TextButton::buttonColourId, theme.getAccentColor());
    generateButton->setColour(juce::TextButton::textColourOnId, theme.getBackgroundColor());

    // Clear All Button
    clearAllButton = std::make_unique<juce::TextButton>("Clear All");
    clearAllButton->setColour(juce::TextButton::buttonColourId, theme.getErrorColor().darker(0.3f));
    clearAllButton->setColour(juce::TextButton::textColourOnId, theme.getTextPrimaryColor());

    // Swing Slider
    swingSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    swingSlider->setRange(0, 0.75, 0.01);
    swingSlider->setValue(0);
    swingSlider->setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    swingSlider->setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());

    // Reverb Slider
    reverbSlider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
    reverbSlider->setRange(0, 1, 0.01);
    reverbSlider->setValue(0);
    reverbSlider->setColour(juce::Slider::trackColourId, theme.getSliderTrackColor());
    reverbSlider->setColour(juce::Slider::thumbColourId, theme.getSliderThumbColor());

    // Workspace Preset Combo
    workspacePresetCombo = std::make_unique<juce::ComboBox>();
    workspacePresetCombo->addItem("Default", 1);
    workspacePresetCombo->addItem("Production", 2);
    workspacePresetCombo->addItem("Live Performance", 3);
    workspacePresetCombo->setSelectedId(1);
    workspacePresetCombo->setColour(juce::ComboBox::backgroundColourId, theme.getButtonColor());
    workspacePresetCombo->setColour(juce::ComboBox::textColourId, theme.getTextPrimaryColor());
    workspacePresetCombo->setColour(juce::ComboBox::outlineColourId, theme.getPanelBorderColor());

    addAndMakeVisible(*loopLengthCombo);
    addAndMakeVisible(*genreCombo);
    addAndMakeVisible(*targetTrackCombo);
    addAndMakeVisible(*generateButton);
    addAndMakeVisible(*clearAllButton);
    addAndMakeVisible(*swingSlider);
    addAndMakeVisible(*reverbSlider);
    addAndMakeVisible(*workspacePresetCombo);

    // Connect callbacks
    loopLengthCombo->onChange = [this]() {
        if (onLoopLengthChanged && loopLengthCombo->getSelectedId() > 0)
            onLoopLengthChanged(loopLengthCombo->getSelectedId());
    };

    genreCombo->onChange = [this]() {
        if (onGenreChanged && genreCombo->getSelectedId() > 0)
            onGenreChanged(genreCombo->getSelectedId());
    };

    targetTrackCombo->onChange = [this]() {
        if (onTargetTrackChanged && targetTrackCombo->getSelectedId() > 0)
            onTargetTrackChanged(targetTrackCombo->getSelectedId());
    };

    generateButton->onClick = [this]() {
        if (onGenerateClicked)
            onGenerateClicked();
    };

    clearAllButton->onClick = [this]() {
        if (onClearAllClicked)
            onClearAllClicked();
    };

    swingSlider->onValueChange = [this]() {
        if (onSwingChanged)
            onSwingChanged(static_cast<float>(swingSlider->getValue()));
    };

    reverbSlider->onValueChange = [this]() {
        if (onReverbChanged)
            onReverbChanged(static_cast<float>(reverbSlider->getValue()));
    };

    workspacePresetCombo->onChange = [this]() {
        if (onWorkspacePresetChanged && workspacePresetCombo->getSelectedId() > 0)
            onWorkspacePresetChanged(workspacePresetCombo->getItemText(workspacePresetCombo->getSelectedId() - 1));
    };
}

void TrackToolsBar::resized()
{
    auto bounds = getLocalBounds();
    auto height = bounds.getHeight();
    auto x = 0;
    auto controlWidth = 100;
    auto gap = 5;

    // Loop Length Combo
    loopLengthCombo->setBounds(x, 0, controlWidth, height);
    x += controlWidth + gap;

    // Genre Combo
    genreCombo->setBounds(x, 0, controlWidth - 20, height);
    x += controlWidth - 20 + gap;

    // Target Track Combo
    targetTrackCombo->setBounds(x, 0, controlWidth - 30, height);
    x += controlWidth - 30 + gap;

    // Generate Button
    generateButton->setBounds(x, 0, controlWidth, height);
    x += controlWidth + gap;

    // Clear All Button
    clearAllButton->setBounds(x, 0, controlWidth, height);
    x += controlWidth + gap;

    // Swing Slider
    swingSlider->setBounds(x, 0, controlWidth, height);
    x += controlWidth + gap;

    // Reverb Slider
    reverbSlider->setBounds(x, 0, controlWidth, height);
    x += controlWidth + gap;

    // Workspace Preset Combo (remaining space)
    workspacePresetCombo->setBounds(x, 0, 150, height);
}
