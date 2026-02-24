#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <functional>
#include <atomic>

// Step state struct supporting TidalCycles modifiers
struct StepModifierState
{
    bool active = false;
    char modifierType = ' ';  // ' ' = Normal, '*' = Speed, '/' = Slow, '@' = Elongate, '!' = Replicate
    int modifierValue = 1;      // Modifier value (1-4 typically)

    // Helper to check if step is effectively active
    bool isActive() const { return active; }

    // Helper to get display text
    juce::String getDisplayText() const
    {
        if (!active)
            return "";
        if (modifierType == ' ')
            return "";
        return juce::String(modifierType) + juce::String(modifierValue);
    }
};

// Custom step button with right-click support for modifiers
class StepButton : public juce::TextButton
{
public:
    StepButton() = default;
    ~StepButton() override = default;

    std::function<void()> onRightClick;

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
};

class TrackComponent : public juce::Component
{
public:
    TrackComponent(int trackIndex, juce::AudioFormatManager& formatManager);
    ~TrackComponent() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

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

    // Pattern bank methods
    void setBank(int bank);
    int getBank() const { return currentBank; }
    int getTotalSteps() const { return numBanks * stepsPerBank; }

    juce::Synthesiser& getSynthesiser() { return synth; }
    juce::ComboBox& getComboBox() { return categoryComboBox; }
    void loadSampleForCategory(const juce::String& category, const juce::File& sampleDirectory);
    void updatePlayhead(int currentStep, bool isPlaying);

    // Control value getters
    float getVolume() const { return volume; }
    int getPitch() const { return pitch; }
    float getDecay() const { return decay; }
    float getAttack() const { return attack; }
    float getCutoff() const { return cutoff; }

    // Sample selection helpers
    void changeSampleIndex(int delta);
    void loadSampleAtIndex(int index);
    void updateSampleIndexLabel();

    // Update decay envelope when knob changes
    void updateDecayEnvelope();

    // Audio processing
    void prepareAudio(double sampleRate, int samplesPerBlock);
    void processAudioBlock(juce::AudioBuffer<float>& buffer);

    // Bank and step button UI helpers
    void refreshStepButtons();
    void updateBankButtonStyles();
    void updateStepButtonState(int bankStep, const StepModifierState& state);

private:
    static constexpr int numBanks = 4;
    static constexpr int stepsPerBank = 16;
    int trackIndex;

    // GUI Components
    juce::Label trackLabel;
    juce::ComboBox categoryComboBox;

    // Sample selection controls
    juce::TextButton prevSampleButton;
    juce::TextButton nextSampleButton;
    juce::Label sampleIndexLabel;

    // Bank selector buttons
    std::array<std::unique_ptr<juce::TextButton>, numBanks> bankButtons;
    juce::Label bankLabel;

    // Control knobs (Volume, Pitch, Attack, Decay, Cutoff)
    juce::Slider volumeSlider;
    juce::Slider pitchSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider cutoffSlider;
    juce::Label volumeLabel;
    juce::Label pitchLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label cutoffLabel;

    // Step buttons for display (16 visible at a time)
    std::array<std::unique_ptr<StepButton>, stepsPerBank> stepButtons;

    // Pattern bank storage (4 banks x 16 steps) - now stores StepModifierState
    std::array<std::array<StepModifierState, stepsPerBank>, numBanks> bankSteps;
    int currentBank = 0;

    // Audio
    juce::Synthesiser synth;
    juce::AudioFormatManager& formatManager;
    juce::CriticalSection synthLock;

    // DSP Filter (Low-Pass)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowPassFilter;
    double currentSampleRate = 44100.0;

    // Control values (thread-safe for audio access)
    std::atomic<float> volume{0.8f};
    std::atomic<int> pitch{0};
    std::atomic<float> attack{0.0f};
    std::atomic<float> decay{0.5f};
    std::atomic<float> cutoff{20000.0f};

    // Sample selection
    juce::Array<juce::File> currentSampleFiles;
    int currentSampleIndex = 0;

    // Colors
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackComponent)
};
