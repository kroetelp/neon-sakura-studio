#include "PluginChainEditor.h"
#include "../VSTHost/PluginWindow.h"
#include <melatonin_blur.h>

//==============================================================================
// PluginChainItem Implementation
//==============================================================================

PluginChainItem::PluginChainItem(NodeID nodeId, const juce::String& name, int position)
    : nodeId(nodeId), pluginName(name), position(position)
{
    initializeComponents();
    setupListeners();
}

PluginChainItem::~PluginChainItem() = default;

void PluginChainItem::initializeComponents()
{
    // Bypass button
    bypassButton = std::make_unique<juce::TextButton>();
    bypassButton->setButtonText("⏻");
    bypassButton->setClickingTogglesState(true);
    bypassButton->setTooltip("Bypass Plugin");
    bypassButton->setSize(20, 20);

    // Remove button
    removeButton = std::make_unique<juce::TextButton>();
    removeButton->setButtonText("×");
    removeButton->setTooltip("Remove Plugin");
    removeButton->setSize(20, 20);

    // Name label
    nameLabel = std::make_unique<juce::Label>();
    nameLabel->setText(pluginName, juce::dontSendNotification);
    nameLabel->setFont(juce::Font(11));
    nameLabel->setJustificationType(juce::Justification::centredLeft);

    // Add child components
    addAndMakeVisible(*bypassButton);
    addAndMakeVisible(*removeButton);
    addAndMakeVisible(*nameLabel);
}

void PluginChainItem::setupListeners()
{
    bypassButton->onClick = [this]
    {
        bypassed = bypassButton->getToggleState();
        if (onBypassChanged)
            onBypassChanged(nodeId);
    };

    removeButton->onClick = [this]
    {
        if (onRemoveClicked)
            onRemoveClicked(nodeId);
    };

    // Mouse down for selection and drag - override directly
}

void PluginChainEditor::mouseDown(const juce::MouseEvent& e)
{
    juce::Component::mouseDown(e);

    // Find item under mouse
    juce::Point<int> localPos = e.getMouseDownPosition();
    auto* targetItem = findItemAtPosition(localPos);

    if (targetItem)
    {
        draggedNodeId = targetItem->getNodeId();
        draggedPosition = targetItem->getPosition();

        // Update selection visual state
        for (auto& chainItem : chainItems)
        {
            chainItem->setSelected(chainItem->getNodeId() == draggedNodeId);
        }
    }
}

void PluginChainItem::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    const int buttonWidth = 20;
    const int gap = 4;

    int x = bounds.getX();
    int y = bounds.getY() + (bounds.getHeight() - 20) / 2;

    // Bypass button
    bypassButton->setBounds(x, y, buttonWidth, 20);
    x += buttonWidth + gap;

    // Name label (takes remaining space)
    nameLabel->setBounds(x, y, bounds.getWidth() - (buttonWidth * 2 + gap * 3), 20);

    // Remove button (right aligned)
    removeButton->setBounds(bounds.getRight() - buttonWidth - gap, y, buttonWidth, 20);
}

void PluginChainItem::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (bypassed)
    {
        // Dimmed for bypassed plugins
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRect(bounds);
        g.setColour(juce::Colours::grey.withAlpha(0.5f));
        g.drawRect(bounds, 1);
    }
    else if (selected)
    {
        // Highlight for selected item
        g.setColour(getNeonPink().withAlpha(0.2f));
        g.fillRect(bounds);
        g.setColour(getNeonPink());
        g.drawRect(bounds, 2);
    }
    else if (isActive_)
    {
        // Subtle highlight for active item
        g.setColour(getNeonCyan().withAlpha(0.1f));
        g.fillRect(bounds);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRect(bounds, 1);
    }
    else
    {
        // Normal state
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.fillRect(bounds);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRect(bounds, 1);
    }
}

void PluginChainItem::setBypassed(bool bypass)
{
    bypassed = bypass;
    bypassButton->setToggleState(bypass, juce::dontSendNotification);
    repaint();
}

void PluginChainItem::setSelected(bool sel)
{
    selected = sel;
    repaint();
}

void PluginChainItem::setActive(bool active)
{
    isActive_ = active;
    repaint();
}

//==============================================================================
// PluginChainEditor Implementation
//==============================================================================

PluginChainEditor::PluginChainEditor(AudioRoutingGraph& graph, int trackIndex,
                               PluginWindowManager& windowManager)
    : graph(graph), trackIndex(trackIndex), windowManager(windowManager)
{
    initializeComponents();
    setupListeners();
    refresh();
}

PluginChainEditor::~PluginChainEditor()
{
    clearChainItems();
}

void PluginChainEditor::initializeComponents()
{
    // Chain viewport
    chainContainer = std::make_unique<juce::Component>();
    chainViewport = std::make_unique<juce::Viewport>();
    chainViewport->setViewedComponent(chainContainer.get(), false);

    // Add plugin button
    addPluginButton = std::make_unique<juce::TextButton>();
    addPluginButton->setButtonText("+ Add Plugin");

    // Close button
    closeButton = std::make_unique<juce::TextButton>();
    closeButton->setButtonText("Close");

    // Title labels
    titleLabel = std::make_unique<juce::Label>();
    titleLabel->setText("Plugin Chain", juce::dontSendNotification);
    titleLabel->setFont(juce::Font(18, juce::Font::bold));

    // Empty state label
    emptyLabel = std::make_unique<juce::Label>();
    emptyLabel->setText("No plugins loaded.\nClick \"+ Add Plugin\" to load a VST3/AU plugin.",
                       juce::dontSendNotification);
    emptyLabel->setFont(juce::Font(12));
    emptyLabel->setJustificationType(juce::Justification::centred);
    emptyLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Add components
    addAndMakeVisible(*titleLabel);
    addAndMakeVisible(*chainViewport);
    addAndMakeVisible(*addPluginButton);
    addAndMakeVisible(*closeButton);
}

void PluginChainEditor::setupListeners()
{
    addPluginButton->onClick = [this]
    {
        // This will be connected to MainComponent's plugin browser
        // For now, just log
        DBG("Add plugin clicked for track " + juce::String(trackIndex));
    };

    closeButton->onClick = [this]
    {
        if (onClose)
            onClose();
    };
}

void PluginChainEditor::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    const int titleHeight = 24;
    const int buttonHeight = 32;
    const int itemHeight = 32;
    const int gap = 12;

    // Title area
    titleLabel->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), titleHeight);
    bounds.removeFromTop(titleHeight + 4);

    // Buttons at bottom
    int buttonY = bounds.getBottom() - buttonHeight - gap;
    addPluginButton->setBounds(bounds.getX(), buttonY, 120, buttonHeight);
    closeButton->setBounds(bounds.getRight() - 80, buttonY, 80, buttonHeight);
    bounds.removeFromBottom(buttonHeight + gap * 2);

    // Chain viewport
    chainViewport->setBounds(bounds);
    chainContainer->setBounds(0, 0, bounds.getWidth(),
                                  chainItems.size() * (itemHeight + 4));
}

void PluginChainEditor::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());

    // Header background
    auto bounds = getLocalBounds();
    auto headerBounds = bounds.removeFromTop(28);
    g.setColour(getNeonPink().withAlpha(0.1f));
    g.fillRect(headerBounds);
}

void PluginChainEditor::refresh()
{
    clearChainItems();

    const auto* chain = graph.getTrackPluginChain(trackIndex);
    if (!chain || chain->empty())
    {
        chainContainer->addAndMakeVisible(*emptyLabel);
        emptyLabel->setBounds(0, 0, chainContainer->getWidth(), 100);
        return;
    }

    rebuildChainItems();
}

void PluginChainEditor::rebuildChainItems()
{
    clearChainItems();

    const auto* chain = graph.getTrackPluginChain(trackIndex);
    if (!chain)
        return;

    int position = 0;
    for (NodeID nodeId : chain->pluginNodeIds)
    {
        auto pluginInstance = graph.getPluginInstance(nodeId);
        juce::String pluginName = "Unknown Plugin";

        if (pluginInstance && pluginInstance->hasEditor())
        {
            pluginName = pluginInstance->getName();
        }

        auto item = std::make_unique<PluginChainItem>(nodeId, pluginName, position);

        // Set up callbacks
        item->onBypassChanged = [this](NodeID id)
        {
            auto wrapper = graph.getPluginInstance(id);
            if (wrapper)
            {
                wrapper->setBypassed(wrapper->isBypassed());
                refresh();
            }
        };

        item->onRemoveClicked = [this](NodeID id)
        {
            graph.removePluginFromTrack(trackIndex, id);
            refresh();
        };

        item->onMouseDown = [this, itemPtr = item.get()](NodeID id)
        {
            selectedNodeId = id;

            // Update selection visual state
            for (auto& chainItem : chainItems)
            {
                chainItem->setSelected(chainItem->getNodeId() == id);
            }

            draggedNodeId = id;

            // Handle drag start in mouseDrag
        };

        chainContainer->addAndMakeVisible(*item);
        chainItems.push_back(std::move(item));

        position++;
    }

    // Update container size
    chainContainer->setBounds(0, 0, chainContainer->getWidth(),
                                  chainItems.size() * 36);
}

void PluginChainEditor::clearChainItems()
{
    chainItems.clear();
    chainContainer->removeAllChildren();
    chainContainer->setBounds(0, 0, chainContainer->getWidth(), 0);
}

void PluginChainEditor::handleDragStart(PluginChainItem* item, const juce::MouseEvent& e)
{
    if (item)
    {
        draggedNodeId = item->getNodeId();
        draggedPosition = item->getPosition();
    }
}

void PluginChainEditor::handleDragMove(const juce::MouseEvent& e)
{
    if (draggedNodeId == 0)
        return;

    // Find item under mouse position
    juce::Point<int> localPos = e.getMouseDownPosition();
    auto* targetItem = findItemAtPosition(localPos);

    if (targetItem && targetItem->getNodeId() != draggedNodeId)
    {
        // Swap positions
        int sourcePos = -1;
        int targetPos = targetItem->getPosition();

        for (size_t i = 0; i < chainItems.size(); ++i)
        {
            if (chainItems[i]->getNodeId() == draggedNodeId)
            {
                sourcePos = static_cast<int>(i);
                break;
            }
        }

        if (sourcePos >= 0 && sourcePos != targetPos)
        {
            // TODO: Implement actual reordering via AudioRoutingGraph
            // For now, just visual feedback
            DBG("Drag from position " + juce::String(sourcePos) +
                 " to position " + juce::String(targetPos));
        }
    }
}

void PluginChainEditor::handleDragEnd(const juce::MouseEvent& e)
{
    draggedNodeId = 0;
    draggedPosition = -1;
}

PluginChainItem* PluginChainEditor::findItemAtPosition(const juce::Point<int>& pos)
{
    for (auto& item : chainItems)
    {
        if (item->getBounds().contains(pos))
        {
            return item.get();
        }
    }
    return nullptr;
}
