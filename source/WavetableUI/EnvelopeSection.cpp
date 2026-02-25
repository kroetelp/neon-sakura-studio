#include "EnvelopeSection.h"
#include "../WavetableSynth/WavetableParams.h"
#include "melatonin_blur.h"

EnvelopeSection::EnvelopeSection()
{
    // Title
    addAndMakeVisible(titleLabel);
    titleLabel.setText("AMP ENVELOPE", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, getNeonGreen());
    titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    // Attack slider
    addAndMakeVisible(attackSlider);
    attackSlider.setRange(0.001, 5.0, 0.001);
    attackSlider.setValue(0.01);
    attackSlider.setSliderStyle(juce::Slider::LinearVertical);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
    attackSlider.setColour(juce::Slider::thumbColourId, getNeonGreen());
    attackSlider.setColour(juce::Slider::trackColourId, getNeonGreen().withAlpha(0.3f));

    addAndMakeVisible(attackLabel);
    attackLabel.setText("A", juce::dontSendNotification);
    attackLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    attackLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    attackLabel.setJustificationType(juce::Justification::centred);

    // Decay slider
    addAndMakeVisible(decaySlider);
    decaySlider.setRange(0.001, 5.0, 0.001);
    decaySlider.setValue(0.1);
    decaySlider.setSliderStyle(juce::Slider::LinearVertical);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
    decaySlider.setColour(juce::Slider::thumbColourId, getNeonGreen());
    decaySlider.setColour(juce::Slider::trackColourId, getNeonGreen().withAlpha(0.3f));

    addAndMakeVisible(decayLabel);
    decayLabel.setText("D", juce::dontSendNotification);
    decayLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    decayLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    decayLabel.setJustificationType(juce::Justification::centred);

    // Sustain slider
    addAndMakeVisible(sustainSlider);
    sustainSlider.setRange(0.0, 1.0, 0.01);
    sustainSlider.setValue(0.7);
    sustainSlider.setSliderStyle(juce::Slider::LinearVertical);
    sustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
    sustainSlider.setColour(juce::Slider::thumbColourId, getNeonGreen());
    sustainSlider.setColour(juce::Slider::trackColourId, getNeonGreen().withAlpha(0.3f));

    addAndMakeVisible(sustainLabel);
    sustainLabel.setText("S", juce::dontSendNotification);
    sustainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    sustainLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    sustainLabel.setJustificationType(juce::Justification::centred);

    // Release slider
    addAndMakeVisible(releaseSlider);
    releaseSlider.setRange(0.001, 10.0, 0.001);
    releaseSlider.setValue(0.3);
    releaseSlider.setSliderStyle(juce::Slider::LinearVertical);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
    releaseSlider.setColour(juce::Slider::thumbColourId, getNeonGreen());
    releaseSlider.setColour(juce::Slider::trackColourId, getNeonGreen().withAlpha(0.3f));

    addAndMakeVisible(releaseLabel);
    releaseLabel.setText("R", juce::dontSendNotification);
    releaseLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    releaseLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    releaseLabel.setJustificationType(juce::Justification::centred);

    // Update path when values change (This is the fallback, but mostly the connect-methods take over)
    auto updatePath = [this] {
        attackValue = static_cast<float>(attackSlider.getValue());
        decayValue = static_cast<float>(decaySlider.getValue());
        sustainValue = static_cast<float>(sustainSlider.getValue());
        releaseValue = static_cast<float>(releaseSlider.getValue());
        updateEnvelopePath();
        repaint();
    };

    attackSlider.onValueChange = updatePath;
    decaySlider.onValueChange = updatePath;
    sustainSlider.onValueChange = updatePath;
    releaseSlider.onValueChange = updatePath;

    updateEnvelopePath();
}

void EnvelopeSection::updateEnvelopePath()
{
    envelopePath.clear();

    // Calculate total time for normalization
    float totalTime = attackValue + decayValue + releaseValue + 0.5f;  // 0.5s for sustain
    float scale = 1.0f / totalTime;

    // Normalize times to 0-1 range
    float a = attackValue * scale;
    float d = decayValue * scale;
    float s = 0.5f * scale;  // Fixed sustain display time
    float r = releaseValue * scale;

    // Create path
    envelopePath.startNewSubPath(0.0f, 1.0f);  // Start at bottom-left
    envelopePath.lineTo(a, 0.0f);              // Attack to top
    envelopePath.lineTo(a + d, 1.0f - sustainValue);  // Decay to sustain level
    envelopePath.lineTo(a + d + s, 1.0f - sustainValue);  // Sustain
    envelopePath.lineTo(a + d + s + r, 1.0f);  // Release to bottom
}

void EnvelopeSection::connectToParams(WavetableVoice::VoiceParams& params)
{
    attackSlider.onValueChange = [&params, this] {
        params.envAttack.store(static_cast<float>(attackSlider.getValue()));
        attackValue = static_cast<float>(attackSlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };

    decaySlider.onValueChange = [&params, this] {
        params.envDecay.store(static_cast<float>(decaySlider.getValue()));
        decayValue = static_cast<float>(decaySlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };

    sustainSlider.onValueChange = [&params, this] {
        params.envSustain.store(static_cast<float>(sustainSlider.getValue()));
        sustainValue = static_cast<float>(sustainSlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };

    releaseSlider.onValueChange = [&params, this] {
        params.envRelease.store(static_cast<float>(releaseSlider.getValue()));
        releaseValue = static_cast<float>(releaseSlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };
}

void EnvelopeSection::connectToSharedParams(std::shared_ptr<WavetableParams> params)
{
    if (!params)
        return;

    attackSlider.onValueChange = [params, this] {
        params->setEnvAttack(static_cast<float>(attackSlider.getValue()));
        attackValue = static_cast<float>(attackSlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };

    decaySlider.onValueChange = [params, this] {
        params->setEnvDecay(static_cast<float>(decaySlider.getValue()));
        decayValue = static_cast<float>(decaySlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };

    sustainSlider.onValueChange = [params, this] {
        params->setEnvSustain(static_cast<float>(sustainSlider.getValue()));
        sustainValue = static_cast<float>(sustainSlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };

    releaseSlider.onValueChange = [params, this] {
        params->setEnvRelease(static_cast<float>(releaseSlider.getValue()));
        releaseValue = static_cast<float>(releaseSlider.getValue());
        updateEnvelopePath();
        repaint(); // <--- Hier fehlte das repaint()!
    };
}

void EnvelopeSection::updateFromParams(float attack, float decay, float sustain, float release)
{
    attackSlider.setValue(attack, juce::dontSendNotification);
    attackValue = attack;

    decaySlider.setValue(decay, juce::dontSendNotification);
    decayValue = decay;

    sustainSlider.setValue(sustain, juce::dontSendNotification);
    sustainValue = sustain;

    releaseSlider.setValue(release, juce::dontSendNotification);
    releaseValue = release;

    updateEnvelopePath();
    repaint();
}

void EnvelopeSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(getPanelBackground());
    g.fillRoundedRectangle(bounds.toFloat(), 5);

    // Border
    g.setColour(getNeonGreen().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(1), 5, 1);

    // Draw envelope display
    auto displayBounds = bounds.reduced(10).removeFromTop(40).toFloat();
    displayBounds.removeFromLeft(100);  // Space for sliders

    if (displayBounds.getWidth() > 50)
    {
        // Scale path to display bounds
        auto scaledPath = envelopePath;
        auto transform = juce::AffineTransform::scale(displayBounds.getWidth(), displayBounds.getHeight())
                                           .translated(displayBounds.getX(), displayBounds.getY());
        scaledPath.applyTransform(transform);

        // Draw glow
        melatonin::DropShadow glow(getNeonGreen(), 4, {0, 0});
        glow.render(g, scaledPath);

        // Draw envelope
        g.setColour(getNeonGreen());
        g.strokePath(scaledPath, juce::PathStrokeType(2.0f));
    }
}

void EnvelopeSection::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(20));

    // Slider area
    auto sliderArea = bounds.removeFromTop(90);
    int sliderWidth = 40;
    int spacing = 10;
    int x = 5;

    attackLabel.setBounds(x, sliderArea.getY(), sliderWidth, 15);
    attackSlider.setBounds(x, sliderArea.getY() + 15, sliderWidth, sliderArea.getHeight() - 25);
    x += sliderWidth + spacing;

    decayLabel.setBounds(x, sliderArea.getY(), sliderWidth, 15);
    decaySlider.setBounds(x, sliderArea.getY() + 15, sliderWidth, sliderArea.getHeight() - 25);
    x += sliderWidth + spacing;

    sustainLabel.setBounds(x, sliderArea.getY(), sliderWidth, 15);
    sustainSlider.setBounds(x, sliderArea.getY() + 15, sliderWidth, sliderArea.getHeight() - 25);
    x += sliderWidth + spacing;

    releaseLabel.setBounds(x, sliderArea.getY(), sliderWidth, 15);
    releaseSlider.setBounds(x, sliderArea.getY() + 15, sliderWidth, sliderArea.getHeight() - 25);
}