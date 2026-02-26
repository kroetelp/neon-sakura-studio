#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <melatonin_blur.h>
#include <functional>
#include <memory>
#include "TrackModel.h"
#include "TrackAudioProcessor.h"
#include "TrackType.h"

class WavetableSynth;
class WavetableParams;
class WavetableData;

// Custom step button with right-click support for modifiers and neon glow
class StepButton : public juce::TextButton
{
public:
    StepButton() = default;
    ~StepButton() override = default;

    std::function<void()> onRightClick;
    bool isActiveStep = false;  // For neon glow rendering

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (onRightClick)
                onRightClick();
        }
        else
        {
            juce::TextButton::mouseDown(e);
        }
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class TrackComponent : public juce::Component
{
public:
    TrackComponent(int trackIndex, juce::AudioFormatManager& formatManager);
    ~TrackComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // UI -> Model delegation
    void setSampleCategories(const juce::StringArray& categories);
    void setSelectedCategory(const juce::String& category);
    juce::String getSelectedCategory() const;

    void setStepActive(int step, bool active);
    bool isStepActive(int step) const;
    StepModifierState getStepState(int step) const;
    void setStepState(int step, const StepModifierState& state);
    void setStepModifier(int step, char modifierType, int modifierValue);
    int getModifierValue(int step) const;
    char getModifierType(int step) const;

    // Bank methods
    void setBank(int bank);
    int getBank() const { return model.getBank(); }
    int getTotalSteps() const { return TrackModel::totalSteps; }

    // UI -> AudioProcessor delegation
    juce::Synthesiser& getSynthesiser() { return audioProcessor.getSynthesiser(); }
    WavetableSynth* getWavetableSynth() { return audioProcessor.getWavetableSynth(); }
    juce::ComboBox& getComboBox() { return categoryComboBox; }
    void loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory);
    void updatePlayhead(int currentStep, bool isPlaying);

    // Control value getters (delegated to AudioProcessor)
    float getVolume() const { return audioProcessor.getVolume(); }
    int getPitch() const { return audioProcessor.getPitch(); }
    float getDecay() const { return audioProcessor.getDecay(); }
    float getAttack() const { return audioProcessor.getAttack(); }
    float getCutoff() const { return audioProcessor.getCutoff(); }
    int getTrackLoopLength() const { return model.getTrackLoopLength(); }

    // Mute/Solo getters
    bool getMuted() const { return audioProcessor.getMuted(); }
    bool getSolo() const { return audioProcessor.getSolo(); }
    bool getIsExpanded() const { return model.getIsExpanded(); }
    void setMuted(bool muted);
    void setSolo(bool solo);
    void clearAllSteps();

    // Wavetable modulator control
    bool getWavetableModulationEnabled() const { return wavetableModulationEnabled; }
    void setWavetableModulationEnabled(bool enabled);

    // Track type control
    TrackType getTrackType() const { return model.getTrackType(); }
    void setTrackType(TrackType type);

    // Wavetable params access (for UI integration)
    std::shared_ptr<WavetableParams> getWavetableParams() const { return audioProcessor.getWavetableParams(); }

    // Callback for collapse state changes
    std::function<void()> onStateChange;

    // Callback to open wavetable editor (called when WT Edit button is clicked)
    std::function<void(int trackIndex, std::shared_ptr<WavetableParams> params, std::shared_ptr<WavetableData> wavetable)> onOpenWavetableEditor;

    // Sample selection helpers
    void changeSampleIndex(int delta);
    void loadSampleAtIndex(int index);
    void updateSampleIndexLabel();

    // Audio processing (delegated to AudioProcessor)
    void prepareAudio(double sampleRate, int samplesPerBlock);
    void processAudioBlock(juce::AudioBuffer<float>& buffer);

    // Bank and step button UI helpers
    void refreshStepButtons();
    void updateBankButtonStyles();
    void updateStepButtonState(int globalStep, const StepModifierState& state);

    // Track type handling
    void setupTrackTypeUI();
    void updateControlsForTrackType(TrackType type);

private:
    int trackIndex;

    // Model (data) and AudioProcessor
    TrackModel model;
    TrackAudioProcessor audioProcessor;

    // GUI Components
    juce::Label trackLabel;
    juce::ComboBox categoryComboBox;

    // Track type selector
    juce::ComboBox trackTypeComboBox;
    juce::Label trackTypeLabel;

    // Sample selection controls
    juce::TextButton prevSampleButton;
    juce::TextButton nextSampleButton;
    juce::Label sampleIndexLabel;

    // Wavetable-spezifische Controls (für Synth-Track)
    juce::Slider oscLevelSlider;
    juce::Slider oscMorphSlider;
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Label oscLevelLabel;
    juce::Label oscMorphLabel;
    juce::Label cutoffLabel;
    juce::Label resonanceLabel;

    // Track controls (Mute, Solo, Clear)
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::TextButton clearButton;

    // Wavetable modulator toggle
    juce::TextButton wavetableModulatorButton;
    bool wavetableModulationEnabled = false;

    // Wavetable editor button
    juce::TextButton wavetableEditorButton;

    // Bank selector buttons
    std::array<std::unique_ptr<juce::TextButton>, TrackModel::numBanks> bankButtons;
    juce::Label bankLabel;

    // Control knobs (Volume, Pitch, Attack, Decay, Cutoff)
    std::array<std::unique_ptr<juce::Slider>, 5> controlSliders;
    std::array<std::unique_ptr<juce::Label>, 5> controlLabels;

    // Step buttons for display (64 visible at once)
    std::array<std::unique_ptr<StepButton>, TrackModel::totalSteps> stepButtons;

    // Expand/Collapse button
    juce::TextButton expandButton;

    // Loop length slider
    juce::Slider loopLengthSlider;
    juce::Label loopLengthLabel;

    // Playhead caching
    int lastPlayedStep{-1};

    // Flag to prevent repaints during resize
    bool isResizing{false};

    // Colors
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComponent)
};
