#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"
#include "../DockingManager.h"
#include "../DockablePanel.h"

/**
 * PanelTogglesBar - Panel Visibility Toggles
 *
 * Verwaltet Toggle-Buttons für alle Panels.
 * Wenn ein DockingManager gesetzt ist, werden die Buttons
 * automatisch synchronisiert mit dem Panel-Status.
 *
 * Usage:
 *   1. PanelTogglesBar erstellen
 *   2. setDockingManager() aufrufen
 *   3. Buttons automatisch syncen
 */
class PanelTogglesBar : public juce::Component, public juce::Timer
{
public:
    PanelTogglesBar();
    ~PanelTogglesBar() override;

    void resized() override;

    // === DockingManager Integration ===

    // DockingManager setzen für automatische Sync
    void setDockingManager(DockingManager* manager);

    // Panel Visibility syncen mit DockingManager
    void syncButtonStates();

    // === Legacy Callbacks (für Plugin Browser der noch nicht im DockingManager ist) ===
    std::function<void(bool)> onPluginBrowserToggled;

    // === Components - getters only (created internally) ===
    juce::TextButton* getRhythmExplorerButton() const { return rhythmExplorerButton.get(); }
    juce::TextButton* getMelodyButton() const { return melodyButton.get(); }
    juce::TextButton* getWavetableButton() const { return wavetableButton.get(); }
    juce::TextButton* getTimelineButton() const { return timelineButton.get(); }
    juce::TextButton* getPluginBrowserButton() const { return pluginBrowserButton.get(); }

private:
    std::unique_ptr<juce::TextButton> rhythmExplorerButton;
    std::unique_ptr<juce::TextButton> melodyButton;
    std::unique_ptr<juce::TextButton> wavetableButton;
    std::unique_ptr<juce::TextButton> timelineButton;
    std::unique_ptr<juce::TextButton> pluginBrowserButton;

    // DockingManager Reference (non-owning)
    DockingManager* dockingManager = nullptr;

    // === Button Panel Type Mapping ===
    PanelType getPanelTypeForButton(juce::TextButton* button) const;
    juce::TextButton* getButtonForPanelType(PanelType type) const;

    // === Timer Override ===
    void timerCallback() override;  // Für periodische Sync mit DockingManager

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelTogglesBar)
};
