// ============================================================================
// WavetablePanel.h - DockablePanel Wrapper für WavetableSynthEditor
// ============================================================================

#pragma once

#include "../DockablePanel.h"
#include <memory>

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
 */
class WavetablePanel : public DockablePanel
{
public:
    WavetablePanel();
    ~WavetablePanel() override;

    // === Dependencies setzen ===

    // Engine-Modus (für standalone Synth)
    void setEngine(WavetableEngine* engine);

    // Shared-Params-Modus (für per-track Synth)
    void setSharedParams(std::shared_ptr<WavetableParams> params,
                          std::shared_ptr<WavetableData> wavetableData = nullptr);

    // === DockablePanel Interface ===
    void prepareForUndock() override;
    void prepareForDock() override;
    void onDockStateChanged(DockState newState) override;

    // === State Persistence ===
    juce::ValueTree saveState() const override;
    void restoreState(const juce::ValueTree& state) override;

    // === Preferred Sizes ===
    juce::Rectangle<int> getPreferredDockedBounds() const override
    {
        return { 0, 0, 800, 400 };
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        return { 100, 100, 1000, 600 };
    }

    int getMinimumWidth() const override { return 300; }
    int getMinimumHeight() const override { return 200; }

    // === Zugriff auf interne Component ===
    WavetableSynthEditor* getSynthEditor() const { return synthEditor.get(); }
    bool hasSynthEditor() const { return synthEditor != nullptr; }

private:
    std::unique_ptr<WavetableSynthEditor> synthEditor;

    // Dependencies (non-owning)
    WavetableEngine* wavetableEngine = nullptr;
    std::shared_ptr<WavetableParams> sharedParams;
    std::shared_ptr<WavetableData> wavetableData;

    void createSynthEditor();
    void updateSynthBounds();

    void resized() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetablePanel)
};
