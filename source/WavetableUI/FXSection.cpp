#include "FXSection.h"
#include <cmath>

FXSection::FXSection()
{
    // Title
    titleLabel.setText("MASTER FX", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, getNeonPurple());
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // === CHORUS ===
    chorusLabel.setText("CHORUS", juce::dontSendNotification);
    chorusLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    chorusLabel.setColour(juce::Label::textColourId, getNeonCyan());
    addAndMakeVisible(chorusLabel);

    setupSlider(chorusMixSlider, chorusMixLabel, "Mix", 0.0, 1.0, 0.0);
    setupSlider(chorusRateSlider, chorusRateLabel, "Rate", 0.1, 5.0, 1.0, " Hz");
    setupSlider(chorusDepthSlider, chorusDepthLabel, "Depth", 0.0, 1.0, 0.25);

    chorusMixSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().chorusMix.store((float)chorusMixSlider.getValue());
    };
    chorusRateSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().chorusRate.store((float)chorusRateSlider.getValue());
    };
    chorusDepthSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().chorusDepth.store((float)chorusDepthSlider.getValue());
    };

    // === DELAY ===
    delayLabel.setText("DELAY", juce::dontSendNotification);
    delayLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    delayLabel.setColour(juce::Label::textColourId, getNeonPink());
    addAndMakeVisible(delayLabel);

    setupSlider(delayMixSlider, delayMixLabel, "Mix", 0.0, 1.0, 0.0);
    setupSlider(delayTimeSlider, delayTimeLabel, "Time", 0.05, 1.0, 0.33, " s");
    setupSlider(delayFeedbackSlider, delayFeedbackLabel, "Fdbk", 0.0, 0.9, 0.4);

    delayMixSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().delayMix.store((float)delayMixSlider.getValue());
    };
    delayTimeSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().delayTime.store((float)delayTimeSlider.getValue());
    };
    delayFeedbackSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().delayFeedback.store((float)delayFeedbackSlider.getValue());
    };

    // === REVERB ===
    reverbLabel.setText("REVERB", juce::dontSendNotification);
    reverbLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    reverbLabel.setColour(juce::Label::textColourId, getNeonGreen());
    addAndMakeVisible(reverbLabel);

    setupSlider(reverbMixSlider, reverbMixLabel, "Mix", 0.0, 1.0, 0.0);
    setupSlider(reverbSizeSlider, reverbSizeLabel, "Size", 0.0, 1.0, 0.5);
    setupSlider(reverbDampingSlider, reverbDampingLabel, "Damp", 0.0, 1.0, 0.5);

    reverbMixSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().reverbMix.store((float)reverbMixSlider.getValue());
    };
    reverbSizeSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().reverbSize.store((float)reverbSizeSlider.getValue());
    };
    reverbDampingSlider.onValueChange = [this] {
        if (engine) engine->getFXParams().reverbDamping.store((float)reverbDampingSlider.getValue());
    };
}

void FXSection::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name,
                             double minVal, double maxVal, double defaultVal, const juce::String& suffix)
{
    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(9.0f));
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);

    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 36, 14);
    slider.setRange(minVal, maxVal);
    slider.setValue(defaultVal, juce::dontSendNotification);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::rotarySliderFillColourId, getNeonPurple());
    slider.setColour(juce::Slider::thumbColourId, getNeonPurple());
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, getPanelBackground().darker(0.3f));
    addAndMakeVisible(slider);
}

void FXSection::connectToEngine(WavetableEngine* eng)
{
    engine = eng;
    if (engine)
        updateFromParams(engine->getFXParams());
}

void FXSection::updateFromParams(const WavetableEngine::FXParams& params)
{
    chorusMixSlider.setValue(params.chorusMix.load(), juce::dontSendNotification);
    chorusRateSlider.setValue(params.chorusRate.load(), juce::dontSendNotification);
    chorusDepthSlider.setValue(params.chorusDepth.load(), juce::dontSendNotification);

    delayMixSlider.setValue(params.delayMix.load(), juce::dontSendNotification);
    delayTimeSlider.setValue(params.delayTime.load(), juce::dontSendNotification);
    delayFeedbackSlider.setValue(params.delayFeedback.load(), juce::dontSendNotification);

    reverbMixSlider.setValue(params.reverbMix.load(), juce::dontSendNotification);
    reverbSizeSlider.setValue(params.reverbSize.load(), juce::dontSendNotification);
    reverbDampingSlider.setValue(params.reverbDamping.load(), juce::dontSendNotification);
}

void FXSection::paint(juce::Graphics& g)
{
    g.fillAll(getPanelBackground());

    // Border
    g.setColour(juce::Colour(60, 40, 80));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), 3, 1);

    // Decorative lines
    g.setColour(getNeonPurple().withAlpha(0.3f));
    auto titleBounds = titleLabel.getBounds();
    g.drawHorizontalLine(titleBounds.getBottom() + 3, 5, getWidth() - 5);
}

void FXSection::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    auto top = bounds.removeFromTop(20);

    // Title
    titleLabel.setBounds(top);

    bounds.removeFromTop(5);

    // Three columns for Chorus, Delay, Reverb
    int colWidth = bounds.getWidth() / 3;

    // === CHORUS COLUMN ===
    auto chorusCol = bounds.removeFromLeft(colWidth);
    chorusLabel.setBounds(chorusCol.removeFromTop(14));

    auto chorusKnobs = chorusCol.removeFromTop(85);
    int knobHeight = 28;

    auto row1 = chorusKnobs.removeFromTop(knobHeight);
    chorusMixLabel.setBounds(row1.removeFromLeft(35));
    chorusMixSlider.setBounds(row1);

    auto row2 = chorusKnobs.removeFromTop(knobHeight);
    chorusRateLabel.setBounds(row2.removeFromLeft(35));
    chorusRateSlider.setBounds(row2);

    auto row3 = chorusKnobs.removeFromTop(knobHeight);
    chorusDepthLabel.setBounds(row3.removeFromLeft(35));
    chorusDepthSlider.setBounds(row3);

    // === DELAY COLUMN ===
    bounds.removeFromLeft(2);  // small gap
    auto delayCol = bounds.removeFromLeft(colWidth);
    delayLabel.setBounds(delayCol.removeFromTop(14));

    auto delayKnobs = delayCol.removeFromTop(85);

    auto dRow1 = delayKnobs.removeFromTop(knobHeight);
    delayMixLabel.setBounds(dRow1.removeFromLeft(35));
    delayMixSlider.setBounds(dRow1);

    auto dRow2 = delayKnobs.removeFromTop(knobHeight);
    delayTimeLabel.setBounds(dRow2.removeFromLeft(35));
    delayTimeSlider.setBounds(dRow2);

    auto dRow3 = delayKnobs.removeFromTop(knobHeight);
    delayFeedbackLabel.setBounds(dRow3.removeFromLeft(35));
    delayFeedbackSlider.setBounds(dRow3);

    // === REVERB COLUMN ===
    bounds.removeFromLeft(2);
    auto reverbCol = bounds;
    reverbLabel.setBounds(reverbCol.removeFromTop(14));

    auto reverbKnobs = reverbCol.removeFromTop(85);

    auto rRow1 = reverbKnobs.removeFromTop(knobHeight);
    reverbMixLabel.setBounds(rRow1.removeFromLeft(35));
    reverbMixSlider.setBounds(rRow1);

    auto rRow2 = reverbKnobs.removeFromTop(knobHeight);
    reverbSizeLabel.setBounds(rRow2.removeFromLeft(35));
    reverbSizeSlider.setBounds(rRow2);

    auto rRow3 = reverbKnobs.removeFromTop(knobHeight);
    reverbDampingLabel.setBounds(rRow3.removeFromLeft(35));
    reverbDampingSlider.setBounds(rRow3);
}
