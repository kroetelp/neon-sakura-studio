#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../WavetableUI/WavetableSynthEditor.h"
#include <memory>

class InternalSynthProcessor;

/**
 * InternalSynthEditor - UI Panel for the Internal Wavetable Synthesizer
 *
 * This is NOT a plugin editor - it's a UI panel that can be embedded
 * in the DAW's interface (as a dockable panel, modal window, etc.)
 *
 * Reuses the existing WavetableSynthEditor component.
 */
class InternalSynthEditor : public juce::AudioProcessorEditor,
                            public juce::Timer
{
public:
    explicit InternalSynthEditor(InternalSynthProcessor& p);
    ~InternalSynthEditor() override;

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Timer override for UI updates
    void timerCallback() override;

private:
    // Reference to processor for parameter access
    InternalSynthProcessor& processor;

    // Look and feel (custom neon style)
    std::unique_ptr<juce::LookAndFeel> lookAndFeel;

    // Main synth editor (reused from standalone)
    std::unique_ptr<WavetableSynthEditor> synthEditor;

    // Size constraints
    static constexpr int minEditorWidth = 1000;
    static constexpr int minEditorHeight = 750;
    static constexpr int defaultWidth = 1050;
    static constexpr int defaultHeight = 800;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalSynthEditor)
};
