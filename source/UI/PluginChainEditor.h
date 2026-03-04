#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <memory>
#include "../Theme/ThemeManager.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../VSTHost/PluginInstance.h"
#include "../VSTHost/PluginWindow.h"

/**
 * PluginChainItem - Visuelle Darstellung eines Plugins in der Kette
 *
 * Features:
 *   - Plugin-Name
 *   - Bypass-Button
 *   - Quick-Access Parameter (optional)
 *   - Drag & Drop Reordering
 *   - Remove-Button
 */
class PluginChainItem : public juce::Component
{
public:
    PluginChainItem(NodeID nodeId, const juce::String& name, int position);
    ~PluginChainItem() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    NodeID getNodeId() const { return nodeId; }
    int getPosition() const { return position; }
    juce::String getName() const { return pluginName; }

    void setBypassed(bool bypassed);
    bool isBypassed() const { return bypassed; }

    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    void setActive(bool active);
    bool isActive() const { return isActive_; }

    // Callbacks
    std::function<void(NodeID)> onBypassChanged;
    std::function<void(NodeID)> onRemoveClicked;
    std::function<void(NodeID)> onMouseDown;

private:
    NodeID nodeId;
    int position;
    juce::String pluginName;
    bool bypassed = false;
    bool selected = false;
    bool isActive_ = false;

    // UI Components
    std::unique_ptr<juce::TextButton> bypassButton;
    std::unique_ptr<juce::TextButton> removeButton;
    std::unique_ptr<juce::Label> nameLabel;

    void initializeComponents();
    void layoutComponents();
    void setupListeners();

    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainItem)
};

/**
 * PluginChainEditor - Visuelle Darstellung der Plugin-Kette
 *
 * Features:
 *   - Drag & Drop Plugin-Reihenfolge
 *   - Bypass-Buttons pro Plugin
 *   - Quick-Access Parameter
 *   - Plugin-Name-Anzeige
 *   - Add/Remove Plugin Buttons
 */
class PluginChainEditor : public juce::Component
{
public:
    PluginChainEditor(AudioRoutingGraph& graph, int trackIndex,
                   PluginWindowManager& windowManager);
    ~PluginChainEditor() override;

    void resized() override;
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    void refresh();
    int getTrackIndex() const { return trackIndex; }

    // Callbacks
    std::function<void()> onClose;

private:
    AudioRoutingGraph& graph;
    int trackIndex;
    PluginWindowManager& windowManager;

    // UI Components
    std::unique_ptr<juce::Viewport> chainViewport;
    std::unique_ptr<juce::Component> chainContainer;
    std::unique_ptr<juce::TextButton> addPluginButton;
    std::unique_ptr<juce::TextButton> closeButton;
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> emptyLabel;

    // Chain items
    std::vector<std::unique_ptr<PluginChainItem>> chainItems;

    NodeID selectedNodeId = 0;
    NodeID draggedNodeId = 0;
    int draggedPosition = -1;

    void initializeComponents();
    void layoutComponents();
    void setupListeners();

    void rebuildChainItems();
    void clearChainItems();

    // Drag and drop handling
    void handleDragStart(PluginChainItem* item, const juce::MouseEvent& e);
    void handleDragMove(const juce::MouseEvent& e);
    void handleDragEnd(const juce::MouseEvent& e);

    // Find chain item under mouse
    PluginChainItem* findItemAtPosition(const juce::Point<int>& pos);

    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChainEditor)
};
