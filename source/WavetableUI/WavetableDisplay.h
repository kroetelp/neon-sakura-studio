#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../WavetableSynth/WavetableVoice.h"
#include "../WavetableSynth/WavetableData.h"
#include "../WavetableSynth/WavetableParams.h"
#include "../Theme/ThemeManager.h"
#include "melatonin_blur.h"
#include <memory>

/**
 * WavetableDisplay - Visual wavetable editor with morphing animation and drawing
 *
 * Features:
 * - View mode: Display waveform, drag to change morph position
 * - Draw mode: Draw waveforms directly with mouse
 * - Generate all frames from drawing using spectral morphing
 */
class WavetableDisplay : public juce::Component,
                          public juce::Timer
{
public:
    enum class EditMode { View, Draw };

    WavetableDisplay();
    ~WavetableDisplay() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Connect to parameters
    void connectToParams(WavetableVoice::VoiceParams& params);
    void connectToSharedParams(std::shared_ptr<WavetableParams> params);

    // Set wavetable data
    void setWavetable(std::shared_ptr<WavetableData> data);

    // Force refresh
    void refresh();

    // Edit mode
    void setEditMode(EditMode mode);
    EditMode getEditMode() const { return editMode; }

    // Callback when wavetable is modified
    std::function<void()> onWavetableModified;

private:
    std::shared_ptr<WavetableData> wavetableData;
    WavetableVoice::VoiceParams* params = nullptr;
    std::shared_ptr<WavetableParams> sharedParams;

    int selectedOscillator = 0;
    float currentMorphPosition = 0.0f;
    std::vector<float> waveformBuffer;

    // Animation
    float animationPhase = 0.0f;

    // Edit mode
    EditMode editMode = EditMode::View;
    std::vector<float> drawnWaveform;
    bool isDrawing = false;
    int lastDrawnIndex = -1;

    // Cached shadow objects (avoid recreation every paint call)
    melatonin::DropShadow glowShadow { juce::Colour(0, 255, 255), 6, {0, 0} };

    // UI Elements
    juce::TextButton editButton;
    juce::TextButton clearButton;
    juce::TextButton generateButton;
    juce::ComboBox basicWaveCombo;

    // Mouse interaction
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Drawing helpers
    void drawAtPosition(int x, int y, const juce::Rectangle<int>& drawArea);
    void updateWavetableFromDrawing();
    void loadBasicWaveform(int waveformType);

    // Visualization
    void updateWaveform();
    juce::Path createWaveformPath(const juce::Rectangle<float>& bounds);
    void drawGrid(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    // Colors (delegated to ThemeManager)
    static juce::Colour getNeonPink() { return ThemeManager::getInstance().getAccentColor(); }
    static juce::Colour getNeonCyan() { return ThemeManager::getInstance().getInfoColor(); }
    static juce::Colour getNeonPurple() { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    static juce::Colour getNeonGreen() { return ThemeManager::getInstance().getSuccessColor(); }
    static juce::Colour getNeonOrange() { return ThemeManager::getInstance().getWarningColor(); }
    static juce::Colour getDarkBackground() { return ThemeManager::getInstance().getBackgroundColor(); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }
};
