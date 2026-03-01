// ============================================================================
// TimelinePanel.h - DockablePanel Wrapper für TimelineComponent
// ============================================================================
//
// Diese Klasse wrapt die TimelineComponent in das neue Docking-System.
// Die TimelineComponent-Instanz bleibt während des gesamten Lebenszyklus
// erhalten - sie wird NICHT neu erstellt beim Docking/Undocking.

#pragma once

#include "../DockablePanel.h"
#include "TimelineComponent.h"
#include <memory>

class SampleManager;

/**
 * TimelinePanel - DockablePanel Wrapper für TimelineComponent
 *
 * Usage:
 *   1. TimelinePanel erstellen
 *   2. setDependencies() aufrufen mit TimelineData und RecordingManager
 *   3. Beim DockingManager registrieren
 *
 * WICHTIG: Die TimelineComponent wird erst erstellt, wenn setDependencies()
 * aufgerufen wird. Das erlaubt es, das Panel zu erstellen bevor die
 * Dependencies verfügbar sind.
 */
class TimelinePanel : public DockablePanel
{
public:
    TimelinePanel();
    ~TimelinePanel() override;

    // === Dependencies setzen ===
    // Muss aufgerufen werden bevor das Panel angezeigt wird
    void setDependencies(TimelineData* data, RecordingManager* recorder);

    // Optional: SampleManager für Sample-Import
    void setSampleManager(SampleManager* manager);

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
        return { 0, 0, 800, 500 };
    }

    juce::Rectangle<int> getPreferredFloatingBounds() const override
    {
        return { 100, 100, 1200, 700 };
    }

    int getMinimumWidth() const override { return 400; }
    int getMinimumHeight() const override { return 300; }

    // === Zugriff auf interne TimelineComponent ===
    TimelineComponent* getTimelineComponent() const { return timelineComponent.get(); }
    bool hasTimelineComponent() const { return timelineComponent != nullptr; }

private:
    // Die eigentliche TimelineComponent
    std::unique_ptr<TimelineComponent> timelineComponent;

    // Dependencies (non-owning references)
    TimelineData* timelineData = nullptr;
    RecordingManager* recordingManager = nullptr;
    SampleManager* sampleManager = nullptr;

    // === Internal ===
    void createTimelineComponent();
    void updateTimelineBounds();

    // === Component Override ===
    void resized() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelinePanel)
};
