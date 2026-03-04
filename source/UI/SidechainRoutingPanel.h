#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../Theme/ThemeManager.h"
#include "../AudioRouting/SidechainManager.h"

// Forward declarations
class AudioRoutingGraph;

/**
 * SidechainRouteRow - Visualisiert eine einzelne Sidechain-Route
 *
 * Zeigt:
 *   - Source Track (Dropdown)
 *   - Target Track (Dropdown)
 *   - Target Plugin (Dropdown, optional)
 *   - Gain Slider
 *   - Enable Toggle
 *   - Remove Button
 */
class SidechainRouteRow : public juce::Component
{
public:
    SidechainRouteRow(int routeId, const SidechainRoute& route, int maxTracks);
    ~SidechainRouteRow() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    int getRouteId() const { return routeId; }

    void updateMaxTracks(int maxTracks);
    void updatePluginsForTargetTrack(int targetTrackIndex,
                                  const std::vector<juce::String>& pluginNames,
                                  const std::vector<uint32_t>& pluginIds);

    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    // Callbacks
    std::function<void(int, const SidechainRoute&)> onRouteChanged;
    std::function<void(int)> onRouteRemoved;
    std::function<void(int)> onRouteSelected;

private:
    int routeId;
    int maxTracks;
    bool selected = false;

    // UI Components
    std::unique_ptr<juce::ComboBox> sourceTrackCombo;
    std::unique_ptr<juce::ComboBox> targetTrackCombo;
    std::unique_ptr<juce::ComboBox> targetPluginCombo;
    std::unique_ptr<juce::Slider> gainSlider;
    std::unique_ptr<juce::TextButton> enableButton;
    std::unique_ptr<juce::TextButton> removeButton;
    std::unique_ptr<juce::Label> sourceLabel;
    std::unique_ptr<juce::Label> targetLabel;
    std::unique_ptr<juce::Label> pluginLabel;
    std::unique_ptr<juce::Label> gainLabel;

    // Track plugin data for target plugin dropdown
    std::vector<uint32_t> targetPluginIds;

    void initializeComponents();
    void layoutComponents();
    void setupListeners();

public:
    // Get route data from current UI state
    SidechainRoute getCurrentRoute() const;

    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainRouteRow)
};

/**
 * SidechainRoutingPanel - Haupt-Panel für Sidechain-Konfiguration
 *
 * Features:
 *   - Liste aller aktiven Sidechain-Routings
 *   - Neue Routings hinzufügen
 *   - Routings bearbeiten/löschen
 *   - Visualisierung von Track-zu-Track Verbindungen
 *   - Plugin-basierte Sidechain (specifices Plugin als Target)
 */
class SidechainRoutingPanel : public juce::Component
{
public:
    SidechainRoutingPanel(AudioRoutingGraph& graph);
    ~SidechainRoutingPanel() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void refreshRoutes();
    void updateTrackCount(int trackCount);

    // Callbacks
    std::function<void()> onClose;

private:
    AudioRoutingGraph& graph;
    int trackCount = 16;

    // UI Components
    std::unique_ptr<juce::Viewport> routeListViewport;
    std::unique_ptr<juce::Component> routeListContainer;
    std::unique_ptr<juce::TextButton> addRouteButton;
    std::unique_ptr<juce::TextButton> closeButton;
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> subtitleLabel;

    // Route rows
    std::vector<std::unique_ptr<SidechainRouteRow>> routeRows;
    std::vector<int> routeIds;

    int selectedRouteId = -1;

    void initializeComponents();
    void layoutComponents();
    void setupListeners();

    void addRouteRow(int routeId, const SidechainRoute& route);
    void removeRouteRow(int routeId);
    void updateRouteRow(int routeId);
    void clearRouteRows();

    void updatePluginDropdowns();
    void getPluginsForTrack(int trackIndex,
                          std::vector<juce::String>& pluginNames,
                          std::vector<uint32_t>& pluginIds);

    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainRoutingPanel)
};
