// ============================================================================
// UnifiedSequencerPanel.h - Multi-Mode Sequencer mit drei Views
// ============================================================================
//
// UnifiedSequencerPanel kombiniert drei verschiedene Sequencer-Ansichten:
// - Track View: Track-basierte Darstellung mit Controls
// - Pattern View: Pattern-Grid mit Drag-to-Draw
// - Timeline View: Timeline-basierte Darstellung
//
// Alle Views teilen sich das gleiche Datenmodell (UnifiedSequencerModel).

#pragma once

#include "../DockablePanel.h"
#include "UnifiedSequencerModel.h"
#include "TrackViewComponent.h"
#include "PatternViewComponent.h"
#include "TimelineViewComponent.h"
#include <memory>

// Forward declarations
class TimelineData;
class RecordingManager;
class SampleManager;
class TrackManager;

/**
 * Mode Tab Component - Tabs oben für View-Wechsel
 */
class ModeTabComponent : public juce::Component
{
public:
    enum Mode
    {
        Track,
        Pattern,
        Timeline
    };

    ModeTabComponent();
    ~ModeTabComponent() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Mode ===
    void setMode(Mode newMode);
    Mode getMode() const { return currentMode; }

    // === Callbacks ===
    std::function<void(Mode mode)> onModeChanged;

private:
    Mode currentMode = Mode::Track;

    // Tab Buttons
    juce::TextButton trackTabButton;
    juce::TextButton patternTabButton;
    juce::TextButton timelineTabButton;

    // Colors
    juce::Colour getTabActiveColor() const { return juce::Colours::white; }
    juce::Colour getTabInactiveColor() const { return juce::Colours::white.withAlpha(0.5f); }
    juce::Colour getTabBackgroundColor() const { return juce::Colour(50, 50, 60); }

    void setupTabButton(juce::TextButton& btn, const juce::String& text);
    void updateTabStyles();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModeTabComponent)
};

/**
 * UnifiedSequencerPanel - Hauptkomponente für alle Sequencer-Views
 */
class UnifiedSequencerPanel : public DockablePanel
{
public:
    UnifiedSequencerPanel();
    ~UnifiedSequencerPanel() override;

    // Konstruktor für DockablePanel
    UnifiedSequencerPanel(PanelType type, const juce::String& panelName);

    // === Dependencies setzen ===
    void setTrackManager(TrackManager* manager);
    void setSampleManager(SampleManager* manager);
    void setTimelineDependencies(TimelineData* data, RecordingManager* recorder);

    // === Sequencer Mode ===
    void setSequencerMode(SequencerMode mode);
    SequencerMode getSequencerMode() const { return sequencerModel->getSequencerMode(); }

    // === UnifiedSequencerModel Access ===
    UnifiedSequencerModel* getModel() { return sequencerModel.get(); }

    // === DockablePanel Interface ===
    void prepareForUndock() override;
    void prepareForDock() override;
    void onDockStateChanged(DockState newState) override;

    // === State Persistence ===
    juce::ValueTree saveState() const override;
    void restoreState(const juce::ValueTree& state);

    // === Preferred Sizes ===
    juce::Rectangle<int> getPreferredDockedBounds() const override
    {
        return { 0, 0, 900, 550 };
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        return { 100, 100, 1100, 700 };
    }

    int getMinimumWidth() const override { return 600; }
    int getMinimumHeight() const override { return 400; }

private:
    // === Data Model ===
    std::unique_ptr<UnifiedSequencerModel> sequencerModel;

    // === Views ===
    std::unique_ptr<TrackViewComponent> trackView;
    std::unique_ptr<PatternViewComponent> patternView;
    std::unique_ptr<TimelineViewComponent> timelineView;

    // === Mode Tabs ===
    std::unique_ptr<ModeTabComponent> modeTabs;

    // === Dependencies (non-owning) ===
    TrackManager* trackManager = nullptr;
    SampleManager* sampleManager = nullptr;
    TimelineData* timelineData = nullptr;
    RecordingManager* recordingManager = nullptr;

    // === Layout ===
    static constexpr int modeTabsHeight = 40;
    juce::Rectangle<int> getContentBounds() const;  // Override für Mode-Tabs

    // === Initialization ===
    void createViews();
    void createModeTabs();
    void setupModelCallbacks();
    void setupViewCallbacks();

    // === View Management ===
    void showView(SequencerMode mode);
    void hideAllViews();

    void resized() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedSequencerPanel)
};
