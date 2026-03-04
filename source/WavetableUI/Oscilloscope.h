#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Theme/ThemeManager.h"
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

    // Colors (delegated to ThemeManager)
    static juce::Colour getNeonPink() { return ThemeManager::getInstance().getAccentColor(); }
    static juce::Colour getNeonCyan() { return ThemeManager::getInstance().getInfoColor(); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    juce::Path createWaveformPath(const std::vector<float>& buffer,
                                   const juce::Rectangle<float>& bounds);
};
