#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include "../WavetableSynth/WavetableEngine.h"
#include "../WavetableSynth/WavetableData.h"
#include "../WavetableSynth/WavetableParams.h"
#include "../WavetableSynth/WavetablePresetManager.h"

class OscillatorSection;
class FilterSection;
class EnvelopeSection;
class WavetableDisplay;
class Oscilloscope;
class ModulationGrid;

/**
 * WavetableSynthEditor - Main editor component for the wavetable synthesiser
 *
 * Serum-inspired layout:
 * - Top: Wavetable display with morphing visualization
 * - Middle: 3 oscillator sections + sub oscillator
 * - Below: Filter section
 * - Bottom: Modulation matrix + Envelope section
 * - Side: Oscilloscope visualization
 *
 * Supports two modes:
 * 1. Engine mode: Connected to a WavetableEngine (standalone synth)
 * 2. Params mode: Connected to shared WavetableParams (track integration)
 */
class WavetableSynthEditor : public juce::Component,
                              public juce::Timer,
                              public juce::MidiKeyboardState::Listener,
                              public WavetableParams::Listener
{
public:
    // Constructor with external engine reference (for audio integration)
    WavetableSynthEditor(WavetableEngine& externalEngine);

    // Constructor with shared params (for track integration)
    WavetableSynthEditor(std::shared_ptr<WavetableParams> sharedParams);

    ~WavetableSynthEditor() override;

    // Access to the engine (returns nullptr in params mode)
    WavetableEngine* getEngine() { return engine; }
    const WavetableEngine* getEngine() const { return engine; }

    // Access to shared params
    std::shared_ptr<WavetableParams> getSharedParams() const { return sharedParams; }

    // Switch params source (for track switching)
    void setSharedParams(std::shared_ptr<WavetableParams> newParams);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer for UI updates
    void timerCallback() override;

    // WavetableParams::Listener
    void onParamsChanged() override;
    void onOscParamChanged(int oscIndex, int paramId) override;
    void onFilterParamChanged() override;
    void onEnvelopeParamChanged() override;

    // Colors
    static juce::Colour getNeonPink() { return juce::Colour(255, 20, 147); }
    static juce::Colour getNeonCyan() { return juce::Colour(0, 255, 255); }
    static juce::Colour getNeonPurple() { return juce::Colour(180, 0, 255); }
    static juce::Colour getNeonGreen() { return juce::Colour(0, 255, 128); }
    static juce::Colour getNeonOrange() { return juce::Colour(255, 165, 0); }
    static juce::Colour getDarkBackground() { return juce::Colour(15, 15, 25); }
    static juce::Colour getPanelBackground() { return juce::Colour(25, 25, 40); }

private:
    // The synth engine (nullptr in params mode, valid in engine mode)
    WavetableEngine* engine;

    // Shared params (for track integration)
    std::shared_ptr<WavetableParams> sharedParams;

    // Mode flag
    bool isEngineMode;

    // UI Sections
    std::unique_ptr<WavetableDisplay> wavetableDisplay;
    std::unique_ptr<OscillatorSection> osc1Section;
    std::unique_ptr<OscillatorSection> osc2Section;
    std::unique_ptr<OscillatorSection> osc3Section;
    std::unique_ptr<FilterSection> filterSection;
    std::unique_ptr<EnvelopeSection> envelopeSection;
    std::unique_ptr<Oscilloscope> oscilloscope;
    std::unique_ptr<ModulationGrid> modulationGrid;

    // Master controls
    juce::Slider masterVolumeSlider;
    juce::Label masterVolumeLabel;

    // Mode toggle
    juce::ToggleButton standaloneModeButton;
    juce::ToggleButton modulatorModeButton;

    // Preset UI
    juce::TextButton savePresetButton;
    juce::TextButton loadPresetButton;
    juce::TextButton saveAsPresetButton;
    juce::ComboBox presetComboBox;
    juce::Label presetLabel;

    // Wavetable loading
    juce::TextButton loadWavetableButton;
    juce::Label wavetableNameLabel;
    std::shared_ptr<WavetableData> loadedWavetable;

    // Preset manager
    std::unique_ptr<WavetablePresetManager> presetManager;
    juce::Array<juce::File> presetFiles;
    WavetablePreset currentPreset;

    // Keyboard for testing
    std::unique_ptr<juce::MidiKeyboardComponent> keyboard;
    juce::MidiKeyboardState keyboardState;

    void setupMasterControls();
    void setupKeyboard();
    void connectUIToEngine();
    void connectUIToSharedParams();

    // Update UI from params
    void updateUIFromParams();

    // Thread-safe UI update trigger
    void triggerAsyncUpdate();

    // Preset handling
    void setupPresetUI();
    void scanPresets();
    void loadSelectedPreset();
    void saveCurrentPreset();
    void savePresetAs();
    void updatePresetComboBox();
    void applyPresetToUI();

    // Wavetable loading
    void setupWavetableUI();
    void loadWavetableFromFile();
    void applyLoadedWavetable();

    // MidiKeyboardState::Listener
    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
};