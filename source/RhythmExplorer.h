#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <melatonin_blur.h>
#include <functional>
#include <vector>
#include <random>
#include <array>

class TrackModel;

// Euclidean Rhythm Generator - creates musically interesting patterns
// Based on Bjorklund's algorithm used in TidalCycles
class EuclideanRhythm
{
public:
    // Generate euclidean rhythm with k pulses in n steps
    static std::vector<bool> generate(int pulses, int steps, int offset = 0);

    // Get pattern density (0.0 to 1.0)
    static float getDensity(int pulses, int steps) { return static_cast<float>(pulses) / steps; }
};

// Pattern Preset definition
struct PatternPreset
{
    juce::String name;
    std::vector<int> activeSteps;  // 0-indexed step positions
    char defaultModifier = ' ';     // Optional default modifier
    int modifierValue = 1;
    bool isFill = false;           // Is this a fill pattern?
};

class RhythmExplorer : public juce::Component
{
public:
    RhythmExplorer();
    ~RhythmExplorer() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // Set the target track for pattern operations
    void setTargetTrack(int trackIndex);
    int getTargetTrack() const { return targetTrack; }

    // Callback when pattern should be applied to a track
    // Parameters: trackIndex, activeSteps, shouldClearFirst
    std::function<void(int, const std::vector<int>&, bool)> onApplyPattern;

    // Callback for fill generation
    std::function<void(int, const std::vector<int>&)> onApplyFill;

    // Euclidean rhythm controls
    void setEuclideanPulses(int pulses);
    void setEuclideanSteps(int steps);
    void setEuclideanOffset(int offset);

    // Generate and preview
    void generateEuclidean();
    void mutatePattern();  // Slight random variation
    void applyVariation(); // Humanize with small offsets

    // Presets
    void loadPreset(int presetIndex);
    void applyFillPreset(int fillIndex);

    // Preview visualization
    void updatePreview(const std::vector<bool>& pattern);

private:
    int targetTrack = 0;

    // Euclidean controls
    juce::Slider pulseSlider;
    juce::Slider stepSlider;
    juce::Slider offsetSlider;
    juce::Label pulseLabel;
    juce::Label stepLabel;
    juce::Label offsetLabel;

    // Action buttons
    juce::TextButton generateButton;
    juce::TextButton mutateButton;
    juce::TextButton humanizeButton;
    juce::TextButton clearButton;

    // Preset buttons
    juce::TextButton kickPresetBtn;
    juce::TextButton snarePresetBtn;
    juce::TextButton hihatPresetBtn;
    juce::TextButton percussionPresetBtn;

    // Fill buttons
    juce::TextButton fill8Btn;
    juce::TextButton fill16Btn;
    juce::TextButton fillRollBtn;

    // Preview display (16-step visualization)
    class PreviewStep : public juce::Component
    {
    public:
        PreviewStep() = default;

        void setActive(bool active)
        {
            if (isActive != active)
            {
                isActive = active;
                repaint();
            }
        }

        void setStepIndex(int index) { stepIndex = index; }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            juce::Path p;
            p.addRoundedRectangle(bounds, 3);

            juce::Colour neonPink = juce::Colour(255, 20, 147);
            juce::Colour stepInactive = juce::Colour(30, 30, 45);

            if (isActive)
            {
                // Glow effect
                melatonin::DropShadow glow(neonPink, 4, {0, 0});
                glow.render(g, p);
                g.setColour(neonPink);
            }
            else
            {
                g.setColour(stepInactive);
            }

            g.fillPath(p);

            // Step number
            g.setColour(isActive ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f));
            g.setFont(8.0f);
            g.drawText(juce::String(stepIndex + 1), bounds, juce::Justification::centred, false);
        }

    private:
        bool isActive = false;
        int stepIndex = 0;
    };

    std::array<std::unique_ptr<PreviewStep>, 16> previewSteps;

    // Current pattern state
    std::vector<bool> currentPattern;
    int currentPulses = 4;
    int currentSteps = 16;
    int currentOffset = 0;

    // Presets library
    std::array<PatternPreset, 4> drumPresets;
    std::array<PatternPreset, 3> fillPresets;

    // Colors
    juce::Colour getNeonPink() const { return juce::Colour(255, 20, 147); }
    juce::Colour getNeonCyan() const { return juce::Colour(0, 255, 255); }
    juce::Colour getNeonPurple() const { return juce::Colour(180, 0, 255); }
    juce::Colour getNeonGreen() const { return juce::Colour(0, 255, 128); }
    juce::Colour getDarkBackground() const { return juce::Colour(15, 15, 25); }
    juce::Colour getStepInactive() const { return juce::Colour(30, 30, 45); }

    // Initialization helpers
    void initializePresets();
    void setupSlider(juce::Slider& slider, int min, int max, int initial);
    void setupButton(juce::TextButton& btn, const juce::String& text, juce::Colour color);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RhythmExplorer)
};

// Standalone Pattern Tools for quick access
class PatternTools
{
public:
    // Common drum patterns
    static PatternPreset getFourOnFloor();      // Kick on 1, 5, 9, 13
    static PatternPreset getTwoStep();          // Kick on 1, 5, Snare on 3, 7
    static PatternPreset getBreakbeat();        // Classic breakbeat pattern
    static PatternPreset getTrapHiHat();        // Trap-style hi-hat pattern
    static PatternPreset getBoomBap();          // Boom bap drum pattern
    static PatternPreset getTechnoKick();       // Driving techno kick
    static PatternPreset getHouseClap();        // House clap pattern
    static PatternPreset getDnBBreak();         // D&B Amen break style

    // Fill patterns
    static PatternPreset getSnareFill8();       // 8-step snare fill
    static PatternPreset getSnareFill16();      // 16-step snare roll
    static PatternPreset getKickFill();         // Kick drum fill
    static PatternPreset getTomRoll();          // Tom roll pattern

    // Generate fill at end of pattern
    static std::vector<int> generateAutoFill(int steps, int intensity, std::mt19937& rng);

    // Humanize pattern with slight timing variations
    static std::vector<int> humanizePattern(const std::vector<int>& steps, int variation, std::mt19937& rng);

    // Mutate pattern (add/remove/move notes)
    static std::vector<int> mutatePattern(const std::vector<int>& steps, int mutationRate, std::mt19937& rng);

    // Density-based random pattern
    static std::vector<int> generateByDensity(float density, int steps, std::mt19937& rng);
};
