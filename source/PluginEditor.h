#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "WavetableUI/WavetableSynthEditor.h"
#include <memory>

class NeonSakuraProcessor;

/**
 * NeonSakuraPluginEditor - VST3/AU Plugin Editor
 *
 * This editor wraps the existing WavetableSynthEditor for use as a plugin UI.
 * It connects to the processor's parameters and engine.
 */
class NeonSakuraPluginEditor : public juce::AudioProcessorEditor,
                                public juce::Timer
{
public:
    explicit NeonSakuraPluginEditor(NeonSakuraProcessor& p);
    ~NeonSakuraPluginEditor() override;

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override for UI updates
    void timerCallback() override;

private:
    // Reference to processor for parameter access
    NeonSakuraProcessor& processor;

    // Look and feel (custom neon style)
    std::unique_ptr<juce::LookAndFeel> lookAndFeel;

    // Main synth editor (reused from standalone)
    std::unique_ptr<WavetableSynthEditor> synthEditor;

    // Size constraints for different DAW hosts
    static constexpr int minEditorWidth = 1000;
    static constexpr int minEditorHeight = 750;
    static constexpr int defaultWidth = 1050;
    static constexpr int defaultHeight = 800;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeonSakuraPluginEditor)
};
