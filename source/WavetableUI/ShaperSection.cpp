#include "ShaperSection.h"
#include "../WavetableSynth/WavetableParams.h"

ShaperSection::ShaperSection()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("SHAPER", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, getNeonPink());
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    // Mode combo
    addAndMakeVisible(modeCombo);
    modeCombo.addItem("Off", static_cast<int>(Waveshaper::Mode::Off) + 1);
    modeCombo.addItem("Soft", static_cast<int>(Waveshaper::Mode::SoftClip) + 1);
    modeCombo.addItem("Hard", static_cast<int>(Waveshaper::Mode::HardClip) + 1);
    modeCombo.addItem("Fold", static_cast<int>(Waveshaper::Mode::Foldback) + 1);
    modeCombo.addItem("Crush", static_cast<int>(Waveshaper::Mode::Bitcrush) + 1);
    modeCombo.addItem("Rect", static_cast<int>(Waveshaper::Mode::Rectify) + 1);
    modeCombo.addItem("Sat", static_cast<int>(Waveshaper::Mode::Saturate) + 1);
    modeCombo.setSelectedId(1);
    modeCombo.setColour(juce::ComboBox::backgroundColourId, getPanelBackground());
    modeCombo.setColour(juce::ComboBox::textColourId, getNeonPink());
    modeCombo.setColour(juce::ComboBox::outlineColourId, getNeonPink().withAlpha(0.5f));

    addAndMakeVisible(modeLabel);
    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    modeLabel.setFont(juce::Font(10.0f));
    modeLabel.setJustificationType(juce::Justification::centred);

    // Amount slider
    addAndMakeVisible(amountSlider);
    amountSlider.setRange(0.0, 1.0, 0.01);
    amountSlider.setValue(0.5);
    amountSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    amountSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    amountSlider.setColour(juce::Slider::thumbColourId, getNeonMagenta());
    amountSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonMagenta().withAlpha(0.3f));
    amountSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

    addAndMakeVisible(amountLabel);
    amountLabel.setText("AMOUNT", juce::dontSendNotification);
    amountLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    amountLabel.setFont(juce::Font(10.0f));
    amountLabel.setJustificationType(juce::Justification::centred);

    // Mix slider
    addAndMakeVisible(mixSlider);
    mixSlider.setRange(0.0, 1.0, 0.01);
    mixSlider.setValue(1.0);
    mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    mixSlider.setColour(juce::Slider::thumbColourId, getNeonPink());
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPink().withAlpha(0.3f));
    mixSlider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

    addAndMakeVisible(mixLabel);
    mixLabel.setText("MIX", juce::dontSendNotification);
    mixLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    mixLabel.setFont(juce::Font(10.0f));
    mixLabel.setJustificationType(juce::Justification::centred);
}

void ShaperSection::connectToSharedParams(std::shared_ptr<WavetableParams> params)
{
    if (!params)
        return;

    modeCombo.onChange = [params, this] {
        params->setShaperMode(modeCombo.getSelectedId() - 1);
    };

    amountSlider.onValueChange = [params, this] {
        params->setShaperAmount(static_cast<float>(amountSlider.getValue()));
    };

    mixSlider.onValueChange = [params, this] {
        params->setShaperMix(static_cast<float>(mixSlider.getValue()));
    };
}

void ShaperSection::updateFromParams(int mode, float amount, float mix)
{
    modeCombo.setSelectedId(mode + 1, juce::dontSendNotification);
    amountSlider.setValue(amount, juce::dontSendNotification);
    mixSlider.setValue(mix, juce::dontSendNotification);
}

void ShaperSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Border with pink glow effect
    g.setColour(getNeonPink().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);
}

void ShaperSection::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(20));

    // Mode combo
    modeLabel.setBounds(10, bounds.getY(), 50, 15);
    modeCombo.setBounds(10, bounds.getY() + 15, 70, 22);

    // Knobs row
    auto knobRow = bounds.reduced(0, 45);
    int knobWidth = 60;
    int x = 10;

    amountLabel.setBounds(x, knobRow.getY(), knobWidth, 15);
    amountSlider.setBounds(x, knobRow.getY() + 15, knobWidth, 55);
    x += knobWidth + 10;

    mixLabel.setBounds(x, knobRow.getY(), knobWidth, 15);
    mixSlider.setBounds(x, knobRow.getY() + 15, knobWidth, 55);
}
