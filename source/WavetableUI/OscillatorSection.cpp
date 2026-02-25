#include "OscillatorSection.h"
#include "../WavetableSynth/WavetableParams.h"

OscillatorSection::OscillatorSection(int oscillatorIndex, const juce::String& title)
    : oscIndex(oscillatorIndex), sectionTitle(title)
{
    // Setup sliders
    setupSlider(levelSlider, "Level");
    levelSlider.setRange(0.0, 1.0, 0.01);
    levelSlider.setValue(1.0);

    setupSlider(morphSlider, "Morph");
    morphSlider.setRange(0.0, 1.0, 0.01);
    morphSlider.setValue(0.0);

    setupSlider(detuneSlider, "Detune");
    detuneSlider.setRange(0.0, 100.0, 1.0);
    detuneSlider.setValue(0.0);

    setupSlider(panSpreadSlider, "Spread");
    panSpreadSlider.setRange(0.0, 1.0, 0.01);
    panSpreadSlider.setValue(0.0);

    setupSlider(panSlider, "Pan");
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(0.0);

    setupSlider(pitchSlider, "Pitch");
    pitchSlider.setRange(-24.0, 24.0, 1.0);
    pitchSlider.setValue(0.0);

    // Setup unison combo
    addAndMakeVisible(unisonCombo);
    for (int i = 1; i <= 8; ++i)
        unisonCombo.addItem(juce::String(i), i);
    unisonCombo.setSelectedId(1);
    unisonCombo.setColour(juce::ComboBox::backgroundColourId, getPanelBackground());
    unisonCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white);

    // Setup labels
    setupLabel(levelLabel, "LVL");
    setupLabel(morphLabel, "MORPH");
    setupLabel(detuneLabel, "DETUNE");
    setupLabel(panSpreadLabel, "SPREAD");
    setupLabel(panLabel, "PAN");
    setupLabel(pitchLabel, "PITCH");
    setupLabel(unisonLabel, "UNISON");
    addAndMakeVisible(unisonLabel);
}

void OscillatorSection::setupSlider(juce::Slider& slider, const juce::String& name)
{
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
    slider.setColour(juce::Slider::thumbColourId, getNeonPink());
    slider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPink().withAlpha(0.3f));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
    slider.setName(name);
}

void OscillatorSection::setupLabel(juce::Label& label, const juce::String& text)
{
    addAndMakeVisible(label);
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont(juce::Font(10.0f));
    label.setJustificationType(juce::Justification::centred);
}

void OscillatorSection::connectToParams(WavetableVoice::VoiceParams& params, int index)
{
    levelSlider.onValueChange = [&params, index, this] {
        params.oscLevels[index].store(static_cast<float>(levelSlider.getValue()));
    };

    morphSlider.onValueChange = [&params, index, this] {
        params.oscMorphs[index].store(static_cast<float>(morphSlider.getValue()));
    };

    detuneSlider.onValueChange = [&params, index, this] {
        params.oscDetunes[index].store(static_cast<float>(detuneSlider.getValue()));
    };

    panSpreadSlider.onValueChange = [&params, index, this] {
        params.oscPanSpreads[index].store(static_cast<float>(panSpreadSlider.getValue()));
    };

    panSlider.onValueChange = [&params, index, this] {
        params.oscPans[index].store(static_cast<float>(panSlider.getValue()));
    };

    pitchSlider.onValueChange = [&params, index, this] {
        params.oscPitchOffsets[index].store(static_cast<float>(pitchSlider.getValue()));
    };

    unisonCombo.onChange = [&params, index, this] {
        params.oscUnisonCounts[index].store(unisonCombo.getSelectedId());
    };
}

void OscillatorSection::connectToSharedParams(std::shared_ptr<WavetableParams> params, int index)
{
    if (!params)
        return;

    levelSlider.onValueChange = [params, index, this] {
        params->setOscLevel(index, static_cast<float>(levelSlider.getValue()));
    };

    morphSlider.onValueChange = [params, index, this] {
        params->setOscMorph(index, static_cast<float>(morphSlider.getValue()));
    };

    detuneSlider.onValueChange = [params, index, this] {
        params->setOscDetune(index, static_cast<float>(detuneSlider.getValue()));
    };

    panSpreadSlider.onValueChange = [params, index, this] {
        params->setOscPanSpread(index, static_cast<float>(panSpreadSlider.getValue()));
    };

    panSlider.onValueChange = [params, index, this] {
        params->setOscPan(index, static_cast<float>(panSlider.getValue()));
    };

    pitchSlider.onValueChange = [params, index, this] {
        params->setOscPitchOffset(index, static_cast<float>(pitchSlider.getValue()));
    };

    unisonCombo.onChange = [params, index, this] {
        params->setOscUnisonCount(index, unisonCombo.getSelectedId());
    };
}

void OscillatorSection::updateFromParams(float level, float morph, float detune, int unison, float panSpread, float pan, float pitch)
{
    // dontSendNotification verhindert eine Endlosschleife (UI ändert Slider -> Slider ruft onValueChange auf -> UI ändert Slider...)
    levelSlider.setValue(level, juce::dontSendNotification);
    morphSlider.setValue(morph, juce::dontSendNotification);
    detuneSlider.setValue(detune, juce::dontSendNotification);
    panSpreadSlider.setValue(panSpread, juce::dontSendNotification);
    panSlider.setValue(pan, juce::dontSendNotification);
    pitchSlider.setValue(pitch, juce::dontSendNotification);
    unisonCombo.setSelectedId(unison, juce::dontSendNotification);
}

void OscillatorSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Border
    g.setColour(getNeonPink().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);

    // Title
    g.setColour(getNeonPink());
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(sectionTitle, bounds.removeFromTop(20), juce::Justification::centred, false);
}

void OscillatorSection::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    bounds.removeFromTop(20);  // Title

    // Two rows of controls
    auto row1 = bounds.removeFromTop(bounds.getHeight() / 2);
    auto row2 = bounds;

    int knobWidth = 55;
    int spacing = 5;

    // Row 1: Level, Morph, Detune, Unison
    int x = 0;
    levelLabel.setBounds(row1.removeFromLeft(knobWidth).withTrimmedTop(0).withTrimmedBottom(20));
    row1.removeFromLeft(spacing);
    morphLabel.setBounds(row1.removeFromLeft(knobWidth).withTrimmedTop(0).withTrimmedBottom(20));
    row1.removeFromLeft(spacing);
    detuneLabel.setBounds(row1.removeFromLeft(knobWidth).withTrimmedTop(0).withTrimmedBottom(20));

    // Actual knobs in row 1
    auto knobRow1 = getLocalBounds().reduced(5);
    knobRow1.removeFromTop(35);
    knobRow1.removeFromTop(15);  // Label space

    x = 5;
    levelSlider.setBounds(x, knobRow1.getY(), 50, 55);
    x += 55;
    morphSlider.setBounds(x, knobRow1.getY(), 50, 55);
    x += 55;
    detuneSlider.setBounds(x, knobRow1.getY(), 50, 55);

    // Row 2: Spread, Pan, Pitch
    auto knobRow2 = knobRow1.translated(0, 60);
    x = 5;
    panSpreadSlider.setBounds(x, knobRow2.getY(), 50, 55);
    x += 55;
    panSlider.setBounds(x, knobRow2.getY(), 50, 55);
    x += 55;
    pitchSlider.setBounds(x, knobRow2.getY(), 50, 55);

    // Labels in row 2
    x = 5;
    panSpreadLabel.setBounds(x, knobRow2.getY() - 15, 50, 15);
    x += 55;
    panLabel.setBounds(x, knobRow2.getY() - 15, 50, 15);
    x += 55;
    pitchLabel.setBounds(x, knobRow2.getY() - 15, 50, 15);

    // Unison combo at bottom
    unisonLabel.setBounds(5, getHeight() - 35, 50, 15);
    unisonCombo.setBounds(5, getHeight() - 20, 50, 18);
}