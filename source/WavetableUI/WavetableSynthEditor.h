#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <memory>
#include "../WavetableSynth/WavetableEngine.h"
#include "../WavetableSynth/WavetableData.h"
#include "../WavetableSynth/WavetableParams.h"
#include "../WavetableSynth/WavetablePresetManager.h"
#include "NeonSakuraLookAndFeel.h"
#include "../Theme/ThemeManager.h"

class OscillatorSection;
class FilterSection;
class EnvelopeSection;
class ShaperSection;
class ModulationSection;
class FXSection;
class WavetableDisplay;
class Oscilloscope;
class ModulationGrid;

class WavetableSynthEditor : public juce::Component,
                              public juce::Timer,
                              public juce::MidiKeyboardState::Listener,
                              public WavetableParams::Listener
{
public:
    WavetableSynthEditor(WavetableEngine& externalEngine);
    WavetableSynthEditor(std::shared_ptr<WavetableParams> sharedParams);
    ~WavetableSynthEditor() override;

    WavetableEngine* getEngine() { return engine; }
    const WavetableEngine* getEngine() const { return engine; }
    std::shared_ptr<WavetableParams> getSharedParams() const { return sharedParams; }
    void setSharedParams(std::shared_ptr<WavetableParams> newParams);
    void setWavetableData(std::shared_ptr<WavetableData> newWavetable);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void onParamsChanged() override;
    void onOscParamChanged(int oscIndex, int paramId) override;
    void onFilterParamChanged() override;
    void onEnvelopeParamChanged() override;

    // Farben (delegated to ThemeManager)
    static juce::Colour getNeonPink() { return ThemeManager::getInstance().getAccentColor(); }
    static juce::Colour getNeonCyan() { return ThemeManager::getInstance().getInfoColor(); }
    static juce::Colour getNeonPurple() { return ThemeManager::getInstance().getAccentColor().withHue(0.8f); }
    static juce::Colour getNeonGreen() { return ThemeManager::getInstance().getSuccessColor(); }
    static juce::Colour getNeonOrange() { return ThemeManager::getInstance().getWarningColor(); }
    static juce::Colour getDarkBackground() { return ThemeManager::getInstance().getBackgroundColor(); }
    static juce::Colour getPanelBackground() { return ThemeManager::getInstance().getPanelBackgroundColor(); }

private:
    NeonSakuraLookAndFeel customLookAndFeel; // <--- NEU HINZUGEFÜGT

    WavetableEngine* engine;
    std::shared_ptr<WavetableParams> sharedParams;
    bool isEngineMode;

    std::unique_ptr<WavetableDisplay> wavetableDisplay;
    std::unique_ptr<OscillatorSection> osc1Section;
    std::unique_ptr<OscillatorSection> osc2Section;
    std::unique_ptr<OscillatorSection> osc3Section;
    std::unique_ptr<FilterSection> filterSection;
    std::unique_ptr<ShaperSection> shaperSection;
    std::unique_ptr<ModulationSection> modulationSection;
    std::unique_ptr<EnvelopeSection> envelopeSection;
    std::unique_ptr<FXSection> fxSection;
    std::unique_ptr<Oscilloscope> oscilloscope;
    std::unique_ptr<ModulationGrid> modulationGrid;

    juce::Slider masterVolumeSlider;
    juce::Label masterVolumeLabel;
    juce::ToggleButton standaloneModeButton;
    juce::ToggleButton modulatorModeButton;

    juce::TextButton savePresetButton;
    juce::TextButton loadPresetButton;
    juce::TextButton saveAsPresetButton;
    juce::TextButton prevPresetButton;
    juce::TextButton nextPresetButton;
    juce::TextButton deletePresetButton;
    juce::ComboBox presetComboBox;
    juce::Label presetLabel;

    juce::TextButton loadWavetableButton;
    juce::ComboBox wavetableComboBox;
    juce::File currentWavetableDir;
    juce::Array<juce::File> wavetableFiles;
    std::shared_ptr<WavetableData> loadedWavetable;

    std::unique_ptr<WavetablePresetManager> presetManager;
    juce::Array<juce::File> presetFiles;
    WavetablePreset currentPreset;

    std::unique_ptr<juce::MidiKeyboardComponent> keyboard;
    juce::MidiKeyboardState keyboardState;

    void setupMasterControls();
    void setupKeyboard();
    void connectUIToEngine();
    void connectUIToSharedParams();
    void updateUIFromParams();
    void triggerAsyncUpdate();

    void setupPresetUI();
    void scanPresets();
    void loadSelectedPreset();
    void saveCurrentPreset();
    void savePresetAs();
    void updatePresetComboBox();
    void applyPresetToUI();
    void navigatePreset(int direction);  // -1 = previous, +1 = next
    void deleteCurrentPreset();
    bool isFactoryPreset(const juce::File& file) const;

    void setupWavetableUI();
    void openWavetableFolder();
    void scanWavetableFolder(const juce::File& dir);
    void loadSelectedWavetable();
    void applyLoadedWavetable();

    void handleNoteOn(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState* source, int midiChannel, int midiNoteNumber, float velocity) override;
};