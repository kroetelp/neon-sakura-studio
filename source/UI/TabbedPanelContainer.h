#pragma once

/**
 * TabbedPanelContainer - Container für Multiple Panels mit Tabs
 *
 * Features:
 * - Multi-Panel Tabs (nicht nur Center)
 * - TabPosition: Top, Bottom, Left, Right
 * - Compact Tabs Mode
 * - TabBarHeight konfigurierbar
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include "../DockablePanel.h"

// Forward declarations
class ZoneSnapManager;

/**
 * TabbedPanelContainer - Container für mehrere Panels mit Tabs
 */
class TabbedPanelContainer : public juce::Component
{
public:
    TabbedPanelContainer();
    ~TabbedPanelContainer() override;

    // ============================================================
    // Tab-Position
    // ============================================================

    enum class TabPosition
    {
        Top,
        Bottom,
        Left,
        Right
    };

    void setTabPosition(TabPosition pos);
    TabPosition getTabPosition() const { return tabPosition; }

    // ============================================================
    // Tabs Management
    // ============================================================

    void addPanel(PanelType type, const juce::String& tabName);
    void removePanel(PanelType type);
    void setActivePanel(PanelType type);

    PanelType getActivePanel() const { return activePanel; }
    int getNumPanels() const { return tabs.size(); }

    // ============================================================
    // Tab-Style
    // ============================================================

    void setCompactTabs(bool compact);
    bool isCompactTabs() const { return compactTabs; }

    void setTabBarHeight(int height);
    int getTabBarHeight() const { return tabBarHeight; }

    // ============================================================
    // ZoneSnapManager Integration
    // ============================================================

    void setZoneSnapManager(ZoneSnapManager* manager) { zoneSnapManager = manager; }

    // ============================================================
    // Rendering
    // ============================================================

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ============================================================
    // Konstanten
    // ============================================================

    static constexpr int defaultTabBarHeight = 32;
    static constexpr int compactTabBarHeight = 24;
    static constexpr int minTabWidth = 80;
    static constexpr int maxTabWidth = 200;

private:
    // ============================================================
    // Tab Data Structure
    // ============================================================

    struct Tab
    {
        PanelType type;
        juce::String name;
        std::unique_ptr<DockablePanel> panel;
        juce::Rectangle<int> bounds;
        bool isHovered = false;
        bool isActive = false;
    };

    // ============================================================
    // State
    // ============================================================

    TabPosition tabPosition = TabPosition::Top;
    bool compactTabs = false;
    int tabBarHeight = defaultTabBarHeight;

    PanelType activePanel = PanelType::Unknown;
    std::vector<Tab> tabs;

    ZoneSnapManager* zoneSnapManager = nullptr;

    // ============================================================
    // Helpers
    // ============================================================

    void updateTabBounds();
    juce::Rectangle<int> getContentBounds() const;

    juce::Rectangle<int> getTabBounds(int tabIndex) const;
    void drawTab(juce::Graphics& g, const Tab& tab, juce::Rectangle<int> bounds);

    juce::Colour getTabBackgroundColor(const Tab& tab) const;
    juce::Colour getTabTextColor(const Tab& tab) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TabbedPanelContainer)
};
