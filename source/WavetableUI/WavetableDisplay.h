#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableVoice.h"
#include "../WavetableSynth/WavetableData.h"
#include <memory>

/**
 * WavetableDisplay - Visual wavetable editor with morphing animation
 *
 * Shows:
 * - Current waveform frame
 * - Morph position indicator
 * - Wavetable name
 */
class WavetableDisplay : public juce::Component,
                          public juce::Timer
{
public:
    WavetableDisplay();
    ~WavetableDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Connect to parameters
    void connectToParams(WavetableVoice::VoiceParams& params);

    // Set wavetable data
    void setWavetable(std::shared_ptr<WavetableData> data);

    // Force refresh of the display (call when wavetable or params change externally)
    void refresh();

    // Mouse interaction for morphing
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    std::shared_ptr<WavetableData> wavetableData;
    WavetableVoice::VoiceParams* params = nullptr;

    int selectedOscillator = 0;
    float currentMorphPosition = 0.0f;
    std::vector<float> waveformBuffer;

    // Animation
    float animationPhase = 0.0f;

    // Colors
    static juce::Colour getNeonPink() { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getNeonPurple() { return juce::Colour(180, 0, 255); }
    static juce::Colour getDarkBackground() { return juce::Colour(15, 15, 25); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

    void updateWaveform();
    juce::Path createWaveformPath(const juce::Rectangle<float>& bounds);
};
