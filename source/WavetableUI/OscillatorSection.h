#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableVoice.h"
#include <memory>

class WavetableParams;

/**
 * OscillatorSection - Controls for a single wavetable oscillator
 *
 * Controls:
 * - Level
 * - Morph position (wavetable frame)
 * - Detune (cents)
 * - Unison count
 * - Pan spread
 * - Pan position
 * - Pitch offset (semitones)
 */
class OscillatorSection : public juce::Component
{
public:
    OscillatorSection(int oscillatorIndex, const juce::String& title);
    ~OscillatorSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Connect to voice parameters (legacy)
    void connectToParams(WavetableVoice::VoiceParams& params, int index);

    // Connect to shared params (for track integration)
    void connectToSharedParams(std::shared_ptr<WavetableParams> params, int index);

    // Update UI from parameter values
    void updateFromParams(float level, float morph, float detune, int unison, float panSpread, float pan, float pitch);

private:
    int oscIndex;
    juce::String sectionTitle;

    // Controls
    juce::Slider levelSlider;
    juce::Slider morphSlider;
    juce::Slider detuneSlider;
    juce::Slider panSpreadSlider;
    juce::Slider panSlider;
    juce::Slider pitchSlider;

    juce::ComboBox unisonCombo;

    // Labels
    juce::Label levelLabel;
    juce::Label morphLabel;
    juce::Label detuneLabel;
    juce::Label panSpreadLabel;
    juce::Label panLabel;
    juce::Label pitchLabel;
    juce::Label unisonLabel;

    // Colors
    static juce::Colour getNeonPink() { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

    void setupSlider(juce::Slider& slider, const juce::String& name);
    void setupLabel(juce::Label& label, const juce::String& text);
};