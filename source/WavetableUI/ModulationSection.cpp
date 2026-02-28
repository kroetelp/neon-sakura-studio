#include "ModulationSection.h"
#include "../WavetableSynth/WavetableParams.h"

ModulationSection::ModulationSection()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("FM/AM MOD", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, getNeonViolet());
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    // FM Label
    addAndMakeVisible(fmLabel);
    fmLabel.setText("FM (st)", juce::dontSendNotification);
    fmLabel.setColour(juce::Label::textColourId, getNeonViolet().brighter(0.3f));
    fmLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    fmLabel.setJustificationType(juce::Justification::centred);

    // FM Sliders - smaller rotary style
    auto setupFMSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        addAndMakeVisible(slider);
        slider.setRange(0.0, 24.0, 0.1);  // 0-24 semitones
        slider.setValue(0.0);
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 12);
        slider.setColour(juce::Slider::thumbColourId, getNeonViolet());
        slider.setColour(juce::Slider::rotarySliderFillColourId, getNeonViolet().withAlpha(0.3f));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label.setFont(juce::Font(9.0f));
        label.setJustificationType(juce::Justification::centred);
    };

    setupFMSlider(fm12Slider, fm12Label, "1>2");
    setupFMSlider(fm13Slider, fm13Label, "1>3");
    setupFMSlider(fm23Slider, fm23Label, "2>3");

    // AM Label
    addAndMakeVisible(amLabel);
    amLabel.setText("AM", juce::dontSendNotification);
    amLabel.setColour(juce::Label::textColourId, getNeonPurple().brighter(0.3f));
    amLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    amLabel.setJustificationType(juce::Justification::centred);

    // AM Sliders
    auto setupAMSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        addAndMakeVisible(slider);
        slider.setRange(0.0, 1.0, 0.01);  // 0-100%
        slider.setValue(0.0);
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 12);
        slider.setColour(juce::Slider::thumbColourId, getNeonPurple());
        slider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPurple().withAlpha(0.3f));
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);

        addAndMakeVisible(label);
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label.setFont(juce::Font(9.0f));
        label.setJustificationType(juce::Justification::centred);
    };

    setupAMSlider(am12Slider, am12Label, "1>2");
    setupAMSlider(am13Slider, am13Label, "1>3");
    setupAMSlider(am23Slider, am23Label, "2>3");

    // Routing info
    addAndMakeVisible(routingLabel);
    routingLabel.setText("1>2 = OSC1 moduliert OSC2", juce::dontSendNotification);
    routingLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    routingLabel.setFont(juce::Font(8.0f));
    routingLabel.setJustificationType(juce::Justification::centred);
}

void ModulationSection::connectToSharedParams(std::shared_ptr<WavetableParams> params)
{
    if (!params)
        return;

    fm12Slider.onValueChange = [params, this] {
        params->setFMAmount12(static_cast<float>(fm12Slider.getValue()));
    };

    fm13Slider.onValueChange = [params, this] {
        params->setFMAmount13(static_cast<float>(fm13Slider.getValue()));
    };

    fm23Slider.onValueChange = [params, this] {
        params->setFMAmount23(static_cast<float>(fm23Slider.getValue()));
    };

    am12Slider.onValueChange = [params, this] {
        params->setAMAmount12(static_cast<float>(am12Slider.getValue()));
    };

    am13Slider.onValueChange = [params, this] {
        params->setAMAmount13(static_cast<float>(am13Slider.getValue()));
    };

    am23Slider.onValueChange = [params, this] {
        params->setAMAmount23(static_cast<float>(am23Slider.getValue()));
    };
}

void ModulationSection::updateFromParams(float fm12, float fm13, float fm23,
                                          float am12, float am13, float am23)
{
    fm12Slider.setValue(fm12, juce::dontSendNotification);
    fm13Slider.setValue(fm13, juce::dontSendNotification);
    fm23Slider.setValue(fm23, juce::dontSendNotification);
    am12Slider.setValue(am12, juce::dontSendNotification);
    am13Slider.setValue(am13, juce::dontSendNotification);
    am23Slider.setValue(am23, juce::dontSendNotification);
}

void ModulationSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Border with violet glow
    g.setColour(getNeonViolet().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);

    // Draw routing visualization
    auto routingBounds = bounds.reduced(10).removeFromBottom(35);
    g.setColour(juce::Colours::white.withAlpha(0.1f));

    // Simple routing diagram
    int centerX = routingBounds.getCentreX();
    int y = routingBounds.getY() + 10;

    // OSC boxes
    g.setColour(getNeonViolet().withAlpha(0.3f));
    g.fillRect(centerX - 80, y, 30, 16);
    g.fillRect(centerX - 15, y, 30, 16);
    g.fillRect(centerX + 50, y, 30, 16);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.drawText("OSC1", centerX - 80, y, 30, 16, juce::Justification::centred, false);
    g.drawText("OSC2", centerX - 15, y, 30, 16, juce::Justification::centred, false);
    g.drawText("OSC3", centerX + 50, y, 30, 16, juce::Justification::centred, false);

    // Arrows (simplified)
    g.setColour(getNeonViolet().withAlpha(0.5f));
    g.drawLine(centerX - 50, y + 8, centerX - 17, y + 8, 1.5f);  // 1->2
    g.drawLine(centerX - 65, y + 20, centerX + 35, y + 20, 1.5f); // 1->3
    g.drawLine(centerX + 15, y + 8, centerX + 48, y + 8, 1.5f);   // 2->3
}

void ModulationSection::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(18));

    // FM Section
    fmLabel.setBounds(bounds.removeFromTop(14));

    auto fmRow = bounds.removeFromTop(65);
    int knobWidth = 50;
    int spacing = (fmRow.getWidth() - 3 * knobWidth) / 4;

    fm12Label.setBounds(fmRow.getX() + spacing, fmRow.getY(), knobWidth, 12);
    fm12Slider.setBounds(fmRow.getX() + spacing, fmRow.getY() + 14, knobWidth, 48);

    fm13Label.setBounds(fmRow.getX() + spacing * 2 + knobWidth, fmRow.getY(), knobWidth, 12);
    fm13Slider.setBounds(fmRow.getX() + spacing * 2 + knobWidth, fmRow.getY() + 14, knobWidth, 48);

    fm23Label.setBounds(fmRow.getX() + spacing * 3 + knobWidth * 2, fmRow.getY(), knobWidth, 12);
    fm23Slider.setBounds(fmRow.getX() + spacing * 3 + knobWidth * 2, fmRow.getY() + 14, knobWidth, 48);

    // AM Section
    amLabel.setBounds(bounds.removeFromTop(14));

    auto amRow = bounds.removeFromTop(65);

    am12Label.setBounds(amRow.getX() + spacing, amRow.getY(), knobWidth, 12);
    am12Slider.setBounds(amRow.getX() + spacing, amRow.getY() + 14, knobWidth, 48);

    am13Label.setBounds(amRow.getX() + spacing * 2 + knobWidth, amRow.getY(), knobWidth, 12);
    am13Slider.setBounds(amRow.getX() + spacing * 2 + knobWidth, amRow.getY() + 14, knobWidth, 48);

    am23Label.setBounds(amRow.getX() + spacing * 3 + knobWidth * 2, amRow.getY(), knobWidth, 12);
    am23Slider.setBounds(amRow.getX() + spacing * 3 + knobWidth * 2, amRow.getY() + 14, knobWidth, 48);

    // Routing label
    routingLabel.setBounds(bounds.removeFromBottom(12));
}
