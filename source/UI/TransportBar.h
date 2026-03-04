#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"

/**
 * TransportBar - Transport Controls (Play, Stop)
 *
 * Professionelles Design ohne Neon-Glow.
 */
class TransportBar : public juce::Component
{
public:
    TransportBar();
    ~TransportBar() override = default;

    void resized() override;

    // Callbacks
    std::function<void()> onPlay;
    std::function<void()> onStop;

    // Components - getters only (created internally)
    juce::TextButton* getPlayButton() const { return playButton.get(); }
    juce::TextButton* getStopButton() const { return stopButton.get(); }

private:
    std::unique_ptr<juce::TextButton> playButton;
    std::unique_ptr<juce::TextButton> stopButton;

    void layoutComponents();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
