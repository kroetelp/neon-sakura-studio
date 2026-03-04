#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableVoice.h"
#include "../Theme/ThemeManager.h"
#include <memory>

class WavetableParams;

/**
 * EnvelopeSection - ADSR envelope controls with visual display
 */
class EnvelopeSection : public juce::Component
{
public:
    EnvelopeSection();
    ~EnvelopeSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void connectToParams(WavetableVoice::VoiceParams& params);
    void connectToSharedParams(std::shared_ptr<WavetableParams> params);

    // Update UI from parameter values
    void updateFromParams(float attack, float decay, float sustain, float release);

private:
    // Controls
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;

    // Labels
    juce::Label titleLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;

    // Envelope display
    juce::Path envelopePath;
    float attackValue = 0.01f;
    float decayValue = 0.1f;
    float sustainValue = 0.7f;
    float releaseValue = 0.3f;

    void updateEnvelopePath();

    // Colors (delegated to ThemeManager)
    static juce::Colour getNeonGreen() { return ThemeManager::getInstance().getSuccessColor(); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }
};