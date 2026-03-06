// ============================================================================
// WavetablePanel.h - DockablePanel Wrapper für WavetableSynthEditor
// ============================================================================

#pragma once

#include "../DockablePanel.h"
#include "../UI/FloatingPanelEnums.h" // Für SynthMode
#include <memory>
#include <functional>

class WavetableEngine;
class WavetableSynthEditor;
class WavetableParams;
class WavetableData;

/**
 * WavetablePanel - DockablePanel Wrapper für WavetableSynthEditor
 *
 * Unterstützt zwei Modi:
 * 1. Engine-Modus: Direkte Referenz auf eine WavetableEngine
 * 2. Shared-Params-Modus: Shared Pointer auf WavetableParams
 *
 * Erweitert mit SynthMode Integration für SynthWorkspacePanel:
 * - PERFORM: Großes WavetableDisplay, optimiert für Live-Performance
 * - DESIGN: Vollständiger Editor-Access mit allen Controls
 * - PATCH: Minimiert, Preset-Browser hat Fokus
 */
class WavetablePanel : public DockablePanel
{
public:
    WavetablePanel();
    ~WavetablePanel() override;

    // ============================================================
    // Dependencies setzen
    // ============================================================

    // Engine-Modus (für standalone Synth)
    void setEngine(WavetableEngine* engine);

    // Shared-Params-Modus (für per-track Synth)
    void setSharedParams(std::shared_ptr<WavetableParams> params,
                          std::shared_ptr<WavetableData> wavetableData = nullptr);

    // ============================================================
    // SynthMode Integration
    // ============================================================

    /**
     * SynthMode setzen für Layout-Optimierung
     * Diese Methode wird von SynthWorkspacePanel aufgerufen
     * wenn der Mode gewechselt wird.
     */
    void setSynthMode(SynthMode mode);
    SynthMode getSynthMode() const { return currentMode; }

    // ============================================================
    // SynthMode-spezifische Features
    // ============================================================

    // PERFORM Mode: Large Display
    void setLargeDisplayMode(bool enabled);
    bool isLargeDisplayMode() const { return largeDisplayMode; }

    // DESIGN Mode: Full Editor Access
    void setFullEditorMode(bool enabled);
    bool isFullEditorMode() const { return fullEditorMode; }

    // PATCH Mode: Minimized
    void setMinimizedMode(bool enabled);
    bool isMinimizedMode() const { return minimizedMode; }

    // ============================================================
    // Perform Mode Features
    // ============================================================

    /**
     * Quick-Oscillator Switch (für Perform Mode)
     * Schnell zwischen Oscillators wechseln für Live-Performance
     */
    struct QuickOscillator
    {
        int oscillatorIndex = 0;
        bool enabled = true;
        float detune = 0.0f;
        float level = 0.8f;
    };

    void setQuickOscillator(const QuickOscillator& osc);
    QuickOscillator getQuickOscillator() const { return quickOscillator; }

    /**
     * Callback für Quick-Oscillator Änderungen
     */
    using QuickOscillatorCallback = std::function<void(const QuickOscillator&)>;
    void setQuickOscillatorCallback(QuickOscillatorCallback callback) { quickOscillatorCallback = std::move(callback); }

    // ============================================================
    // Design Mode Features
    // ============================================================

    /**
     * Wavetable-Editor Show/Hide
     */
    void setWavetableEditorVisible(bool show);
    bool isWavetableEditorVisible() const { return wavetableEditorVisible; }

    /**
     * Envelope-Editor Show/Hide
     */
    void setEnvelopeEditorVisible(bool show);
    bool isEnvelopeEditorVisible() const { return envelopeEditorVisible; }

    /**
     * Filter-Section Show/Hide
     */
    void setFilterSectionVisible(bool show);
    bool isFilterSectionVisible() const { return filterSectionVisible; }

    // ============================================================
    // Display Customization
    // ============================================================

    /**
     * Display-Mode für Wavetable Anzeige
     */
    enum class DisplayMode
    {
        Standard,      // Normales Display
        Large,         // Groß für Perform Mode
        Compact        // Klein für Patch Mode
    };

    void setDisplayMode(DisplayMode mode);
    DisplayMode getDisplayMode() const { return displayMode; }

    /**
     * Show/Hide von Labels und Values
     */
    void setShowLabels(bool show) { showLabels = show; }
    bool shouldShowLabels() const { return showLabels; }

    void setShowValues(bool show) { showValues = show; }
    bool shouldShowValues() const { return showValues; }

    // ============================================================
    // DockablePanel Interface
    // ============================================================

    void prepareForUndock() override;
    void prepareForDock() override;
    void onDockStateChanged(DockState newState) override;

    // ============================================================
    // State Persistence
    // ============================================================

    juce::ValueTree saveState() const override;
    void restoreState(const juce::ValueTree& state) override;

    // ============================================================
    // Preferred Sizes (abhängig von SynthMode)
    // ============================================================

    juce::Rectangle<int> getPreferredDockedBounds() const override
    {
        switch (currentMode)
        {
            case SynthMode::Perform:
                return { 0, 0, 900, 500 };
            case SynthMode::Design:
                return { 0, 0, 1000, 600 };
            case SynthMode::Patch:
                return { 0, 0, 700, 400 };
            default:
                return { 0, 0, 800, 400 };
        }
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        switch (currentMode)
        {
            case SynthMode::Perform:
                return { 100, 100, 1100, 650 };
            case SynthMode::Design:
                return { 100, 100, 1200, 750 };
            case SynthMode::Patch:
                return { 100, 100, 850, 500 };
            default:
                return { 100, 100, 1000, 600 };
        }
    }

    int getMinimumWidth() const override
    {
        return (currentMode == SynthMode::Patch) ? 300 : 400;
    }

    int getMinimumHeight() const override
    {
        return (currentMode == SynthMode::Patch) ? 200 : 300;
    }

    // ============================================================
    // Zugriff auf interne Component
    // ============================================================

    WavetableSynthEditor* getSynthEditor() const { return synthEditor.get(); }
    bool hasSynthEditor() const { return synthEditor != nullptr; }

private:
    std::unique_ptr<WavetableSynthEditor> synthEditor;

    // Dependencies (non-owning)
    WavetableEngine* wavetableEngine = nullptr;
    std::shared_ptr<WavetableParams> sharedParams;
    std::shared_ptr<WavetableData> wavetableData;

    // ============================================================
    // SynthMode State
    // ============================================================

    SynthMode currentMode = SynthMode::Perform;
    DisplayMode displayMode = DisplayMode::Standard;

    // SynthMode-spezifische Flags
    bool largeDisplayMode = false;
    bool fullEditorMode = false;
    bool minimizedMode = false;

    // Editor Visibility Flags
    bool wavetableEditorVisible = true;
    bool envelopeEditorVisible = true;
    bool filterSectionVisible = true;

    // Display Flags
    bool showLabels = true;
    bool showValues = true;

    // ============================================================
    // Perform Mode State
    // ============================================================

    QuickOscillator quickOscillator;
    QuickOscillatorCallback quickOscillatorCallback;

    // ============================================================
    // Helper Methods
    // ============================================================

    void createSynthEditor();
    void updateSynthBounds();
    void updateSynthModeLayout();

    void resized() override;
    void paint(juce::Graphics& g) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetablePanel)
};
