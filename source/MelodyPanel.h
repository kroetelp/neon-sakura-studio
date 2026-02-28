#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur.h>
#include <functional>
#include <vector>
#include "MusicTheory.h"
#include "MelodyGenerator.h"

// MelodyPanel - Full Workstation UI for melody creation
class MelodyPanel : public juce::Component
{
public:
    MelodyPanel();
    ~MelodyPanel() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Callback when melody should be applied
    // Parameters: trackIndex, stepPitches (step, pitchOffset pairs)
    std::function<void(int, const std::vector<std::pair<int, int>>&)> onApplyMelody;

    // Set target track
    void setTargetTrack(int track) {
        targetTrack = track;
        targetTrackCombo.setSelectedId(track + 1, juce::dontSendNotification);
    }
    int getTargetTrack() const { return targetTrack; }

    // Get current parameters
    MelodyParams getCurrentParams() const;

private:
    int targetTrack = 0;
    MelodyGenerator generator;

    // Key/Scale selection
    juce::ComboBox rootNoteCombo;
    juce::ComboBox scaleCombo;
    juce::Label keyLabel;
    juce::Label scaleLabel;

    // Octave controls
    juce::Slider octaveSlider;
    juce::Slider octaveRangeSlider;
    juce::Label octaveLabel;
    juce::Label octaveRangeLabel;

    // Melody type
    juce::ComboBox melodyTypeCombo;
    juce::Label melodyTypeLabel;

    // Chord progression
    juce::ComboBox progressionCombo;
    juce::Label progressionLabel;

    // Pattern settings
    juce::Slider densitySlider;
    juce::Slider variationSlider;
    juce::Label densityLabel;
    juce::Label variationLabel;

    // Rhythm settings
    juce::ToggleButton syncopateToggle;
    juce::Slider maxLeapSlider;
    juce::Label maxLeapLabel;

    // Chord settings
    juce::ComboBox chordTypeCombo;
    juce::ToggleButton arpUpToggle;
    juce::ToggleButton arpDownToggle;

    // Step count
    juce::Slider stepCountSlider;
    juce::Label stepCountLabel;

    // Target track selection
    juce::ComboBox targetTrackCombo;
    juce::Label targetTrackLabel;

    // Action buttons
    juce::TextButton generateMelodyBtn;
    juce::TextButton generateArpeggioBtn;
    juce::TextButton generateBassBtn;
    juce::TextButton generateLeadBtn;
    juce::TextButton generateProgressionBtn;
    juce::TextButton clearBtn;

    // Preview keyboard (mini)
    class MiniKeyboard : public juce::Component
    {
    public:
        void setNotes(const std::vector<int>& notes) { activeNotes = notes; repaint(); }
        void paint(juce::Graphics& g) override;
    private:
        std::vector<int> activeNotes;
    };
    std::unique_ptr<MiniKeyboard> keyboard;

    // Piano roll preview (16 steps)
    class PianoRoll : public juce::Component
    {
    public:
        void setMelody(const std::vector<MelodyNote>& notes) { melody = notes; repaint(); }
        void setScale(int root, int scaleIdx) { rootNote = root; scaleIndex = scaleIdx; repaint(); }
        void paint(juce::Graphics& g) override;
    private:
        std::vector<MelodyNote> melody;
        int rootNote = 0;
        int scaleIndex = 0;
    };
    std::unique_ptr<PianoRoll> pianoRoll;

    // Current melody state
    std::vector<MelodyNote> currentMelody;

    // Colors
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getNeonGreen() const { return juce::Colour(0, 255, 128); }
    juce::Colour getNeonOrange() const { return juce::Colour(255, 165, 0); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }

    // Setup helpers
    void setupComboBox(juce::ComboBox& combo, const juce::StringArray& items, int defaultIndex = 0);
    void setupSlider(juce::Slider& slider, double min, double max, double initial, double step = 1.0);
    void setupButton(juce::TextButton& btn, const juce::String& text, juce::Colour color);

    // Generate actions
    void generateMelody();
    void generateArpeggio();
    void generateBassLine();
    void generateLeadLine();
    void generateProgression();
    void clearMelody();
    void applyMelody();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MelodyPanel)
};
