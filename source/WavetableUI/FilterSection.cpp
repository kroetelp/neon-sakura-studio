#include "FilterSection.h"
#include "../WavetableSynth/WavetableParams.h"

FilterSection::FilterSection()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("FILTER", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, getNeonCyan());
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    // Cutoff slider
    addAndMakeVisible(cutoffSlider);
    cutoffSlider.setRange(20.0, 20000.0, 1.0);
    cutoffSlider.setValue(1000.0);
    cutoffSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    cutoffSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    cutoffSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonCyan().withAlpha(0.3f));
    cutoffSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

    addAndMakeVisible(cutoffLabel);
    cutoffLabel.setText("CUTOFF", juce::dontSendNotification);
    cutoffLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    cutoffLabel.setFont(juce::Font(10.0f));
    cutoffLabel.setJustificationType(juce::Justification::centred);

    // Resonance slider
    addAndMakeVisible(resonanceSlider);
    resonanceSlider.setRange(0.0, 1.0, 0.01);
    resonanceSlider.setValue(0.0);
    resonanceSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    resonanceSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    resonanceSlider.setColour(juce::Slider::thumbColourId, getNeonCyan());
    resonanceSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonCyan().withAlpha(0.3f));
    resonanceSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

    addAndMakeVisible(resonanceLabel);
    resonanceLabel.setText("RES", juce::dontSendNotification);
    resonanceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    resonanceLabel.setFont(juce::Font(10.0f));
    resonanceLabel.setJustificationType(juce::Justification::centred);

    // Drive slider
    addAndMakeVisible(driveSlider);
    driveSlider.setRange(0.0, 1.0, 0.01);
    driveSlider.setValue(0.0);
    driveSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    driveSlider.setColour(juce::Slider::thumbColourId, getNeonOrange());
    driveSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonOrange().withAlpha(0.3f));
    driveSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

    addAndMakeVisible(driveLabel);
    driveLabel.setText("DRIVE", juce::dontSendNotification);
    driveLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    driveLabel.setFont(juce::Font(10.0f));
    driveLabel.setJustificationType(juce::Justification::centred);

    // Mode combo
    addAndMakeVisible(modeCombo);
    modeCombo.addItem("LP", static_cast<int>(WavetableFilter::Mode::LowPass) + 1);
    modeCombo.addItem("HP", static_cast<int>(WavetableFilter::Mode::HighPass) + 1);
    modeCombo.addItem("BP", static_cast<int>(WavetableFilter::Mode::BandPass) + 1);
    modeCombo.addItem("Notch", static_cast<int>(WavetableFilter::Mode::Notch) + 1);
    modeCombo.setSelectedId(1);
    modeCombo.setColour(juce::ComboBox::backgroundColourId, getPanelBackground());
    modeCombo.setColour(juce::ComboBox::textColourId, getNeonCyan());

    addAndMakeVisible(modeLabel);
    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    modeLabel.setFont(juce::Font(10.0f));
}

void FilterSection::connectToParams(WavetableVoice::VoiceParams& params)
{
    cutoffSlider.onValueChange = [&params, this] {
        params.filterCutoff.store(static_cast<float>(cutoffSlider.getValue()));
    };

    resonanceSlider.onValueChange = [&params, this] {
        params.filterResonance.store(static_cast<float>(resonanceSlider.getValue()));
    };

    driveSlider.onValueChange = [&params, this] {
        params.filterDrive.store(static_cast<float>(driveSlider.getValue()));
    };

    modeCombo.onChange = [&params, this] {
        params.filterMode.store(modeCombo.getSelectedId() - 1);
    };
}

void FilterSection::connectToSharedParams(std::shared_ptr<WavetableParams> params)
{
    if (!params)
        return;

    cutoffSlider.onValueChange = [params, this] {
        params->setFilterCutoff(static_cast<float>(cutoffSlider.getValue()));
    };

    resonanceSlider.onValueChange = [params, this] {
        params->setFilterResonance(static_cast<float>(resonanceSlider.getValue()));
    };

    driveSlider.onValueChange = [params, this] {
        params->setFilterDrive(static_cast<float>(driveSlider.getValue()));
    };

    modeCombo.onChange = [params, this] {
        params->setFilterMode(modeCombo.getSelectedId() - 1);
    };
}

void FilterSection::updateFromParams(float cutoff, float resonance, float drive, int mode)
{
    cutoffSlider.setValue(cutoff, juce::dontSendNotification);
    resonanceSlider.setValue(resonance, juce::dontSendNotification);
    driveSlider.setValue(drive, juce::dontSendNotification);
    modeCombo.setSelectedId(mode + 1, juce::dontSendNotification);
}

void FilterSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Border
    g.setColour(getNeonCyan().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);
}

void FilterSection::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(20));

    // Knobs row
    auto knobRow = bounds.removeFromTop(80);
    int knobWidth = 60;
    int x = 10;

    cutoffLabel.setBounds(x, knobRow.getY(), knobWidth, 15);
    cutoffSlider.setBounds(x, knobRow.getY() + 15, knobWidth, 55);
    x += knobWidth + 10;

    resonanceLabel.setBounds(x, knobRow.getY(), knobWidth, 15);
    resonanceSlider.setBounds(x, knobRow.getY() + 15, knobWidth, 55);
    x += knobWidth + 10;

    driveLabel.setBounds(x, knobRow.getY(), knobWidth, 15);
    driveSlider.setBounds(x, knobRow.getY() + 15, knobWidth, 55);

    // Mode combo
    modeLabel.setBounds(10, bounds.getY(), 50, 15);
    modeCombo.setBounds(10, bounds.getY() + 15, 70, 22);
}
