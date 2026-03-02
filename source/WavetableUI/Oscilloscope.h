#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "melatonin_blur.h"
#include <vector>

/**
 * Oscilloscope - Audio visualization component
 *
 * Displays real-time audio output as a waveform
 */
class Oscilloscope : public juce::Component,
                      public juce::Timer
{
public:
    Oscilloscope();
    ~Oscilloscope() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Push audio samples for visualization
    void pushSamples(const float* leftSamples, const float* rightSamples, int numSamples);

private:
    static constexpr int bufferSize = 1024;

    std::vector<float> leftBuffer;
    std::vector<float> rightBuffer;
    int writePosition = 0;

    // Cached shadow objects (avoid recreation every paint call)
    melatonin::DropShadow leftGlow { juce::Colour(0, 255, 255), 3, {0, 0} };
    melatonin::DropShadow rightGlow { juce::Colour(255, 20, 147), 3, {0, 0} };

    // Colors
    static juce::Colour getNeonPink() { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

    juce::Path createWaveformPath(const std::vector<float>& buffer,
                                   const juce::Rectangle<float>& bounds);
};
