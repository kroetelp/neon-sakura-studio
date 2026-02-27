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
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 45, 18);
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

    // Background with Cyberpunk gradient
    auto gradient = juce::ColourGradient::vertical(
        getPanelBackground().brighter(0.05f), 0,
        getPanelBackground().darker(0.2f), (float)bounds.getHeight());
    g.setGradientFill(gradient);
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
    bounds.removeFromTop(20);  // Platz für den Titel oben lassen

    juce::FlexBox mainBox;
    mainBox.flexDirection = juce::FlexBox::Direction::column;

    juce::FlexBox row1, row2;
    row1.flexDirection = juce::FlexBox::Direction::row;
    row2.flexDirection = juce::FlexBox::Direction::row;

    // WICHTIG: Alle FlexBoxen müssen hier deklariert werden, damit sie bis zum performLayout() existieren!
    juce::FlexBox kb1, kb2, kb3, kb4, kb5, kb6, kb7;

    // Sicheres Hilfs-Lambda, das bereits existierende FlexBoxen befüllt
    auto setupKnobBox = [](juce::FlexBox& kb, juce::Component& slider, juce::Component& label) {
        kb.flexDirection = juce::FlexBox::Direction::column;
        kb.items.add(juce::FlexItem(label).withHeight(15.0f));
        kb.items.add(juce::FlexItem(slider).withFlex(1.0f));
    };

    // --- Reihe 1 befüllen ---
    setupKnobBox(kb1, levelSlider, levelLabel);
    setupKnobBox(kb2, morphSlider, morphLabel);
    setupKnobBox(kb3, detuneSlider, detuneLabel);
    
    row1.items.add(juce::FlexItem(kb1).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    row1.items.add(juce::FlexItem(kb2).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    row1.items.add(juce::FlexItem(kb3).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));

    // --- Reihe 2 befüllen ---
    setupKnobBox(kb4, panSpreadSlider, panSpreadLabel);
    setupKnobBox(kb5, panSlider, panLabel);
    setupKnobBox(kb6, pitchSlider, pitchLabel);

    // Unison Combo Box (kb7)
    kb7.flexDirection = juce::FlexBox::Direction::column;
    kb7.items.add(juce::FlexItem(unisonLabel).withHeight(15.0f));
    kb7.items.add(juce::FlexItem(unisonCombo).withHeight(20.0f).withMargin(juce::FlexItem::Margin(5, 0, 0, 0)));

    row2.items.add(juce::FlexItem(kb4).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    row2.items.add(juce::FlexItem(kb5).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    row2.items.add(juce::FlexItem(kb6).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));
    row2.items.add(juce::FlexItem(kb7).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 5, 0, 5)));

    // Füge beide Reihen zur Hauptbox hinzu
    mainBox.items.add(juce::FlexItem(row1).withFlex(1.0f).withMargin(juce::FlexItem::Margin(0, 0, 10, 0)));
    mainBox.items.add(juce::FlexItem(row2).withFlex(1.0f));

    // Layout sicher berechnen
    mainBox.performLayout(bounds);
}