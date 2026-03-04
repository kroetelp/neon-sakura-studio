#include "SidechainRoutingPanel.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../VSTHost/PluginInstance.h"
#include <melatonin_blur.h>

//==============================================================================
// SidechainRouteRow Implementation
//==============================================================================

SidechainRouteRow::SidechainRouteRow(int routeId, const SidechainRoute& route, int maxTracks)
    : routeId(routeId), maxTracks(maxTracks)
{
    initializeComponents();
    setupListeners();

    // Set initial values
    sourceTrackCombo->setSelectedId(route.sourceTrackIndex + 1, juce::dontSendNotification);
    targetTrackCombo->setSelectedId(route.targetTrackIndex + 1, juce::dontSendNotification);
    gainSlider->setValue(route.gain, juce::dontSendNotification);
    enableButton->setToggleState(route.enabled, juce::dontSendNotification);
}

void SidechainRouteRow::initializeComponents()
{
    // Source track dropdown
    sourceTrackCombo = std::make_unique<juce::ComboBox>();
    sourceTrackCombo->addItem("None", 1);
    for (int i = 0; i < maxTracks; ++i)
    {
        sourceTrackCombo->addItem("Track " + juce::String(i + 1), i + 2);
    }

    // Target track dropdown
    targetTrackCombo = std::make_unique<juce::ComboBox>();
    targetTrackCombo->addItem("None", 1);
    for (int i = 0; i < maxTracks; ++i)
    {
        targetTrackCombo->addItem("Track " + juce::String(i + 1), i + 2);
    }

    // Target plugin dropdown (optional)
    targetPluginCombo = std::make_unique<juce::ComboBox>();
    targetPluginCombo->addItem("All Plugins", 1);
    targetPluginCombo->setEnabled(false);

    // Gain slider
    gainSlider = std::make_unique<juce::Slider>(juce::Slider::LinearBar, juce::Slider::NoTextBox);
    gainSlider->setRange(0.0, 2.0, 0.01);
    gainSlider->setPopupMenuEnabled(true);

    // Enable toggle
    enableButton = std::make_unique<juce::TextButton>();
    enableButton->setButtonText("ON");
    enableButton->setClickingTogglesState(true);

    // Remove button
    removeButton = std::make_unique<juce::TextButton>();
    removeButton->setButtonText("X");
    removeButton->setTooltip("Remove route");

    // Labels
    sourceLabel = std::make_unique<juce::Label>();
    sourceLabel->setText("Source", juce::dontSendNotification);

    targetLabel = std::make_unique<juce::Label>();
    targetLabel->setText("Target", juce::dontSendNotification);

    pluginLabel = std::make_unique<juce::Label>();
    pluginLabel->setText("Plugin", juce::dontSendNotification);

    gainLabel = std::make_unique<juce::Label>();
    gainLabel->setText("Gain", juce::dontSendNotification);

    // Add child components
    addAndMakeVisible(*sourceLabel);
    addAndMakeVisible(*sourceTrackCombo);
    addAndMakeVisible(*targetLabel);
    addAndMakeVisible(*targetTrackCombo);
    addAndMakeVisible(*pluginLabel);
    addAndMakeVisible(*targetPluginCombo);
    addAndMakeVisible(*gainLabel);
    addAndMakeVisible(*gainSlider);
    addAndMakeVisible(*enableButton);
    addAndMakeVisible(*removeButton);
}

void SidechainRouteRow::setupListeners()
{
    sourceTrackCombo->onChange = [this]
    {
        if (onRouteChanged)
            onRouteChanged(routeId, getCurrentRoute());
    };

    targetTrackCombo->onChange = [this]
    {
        if (onRouteChanged)
            onRouteChanged(routeId, getCurrentRoute());
    };

    targetPluginCombo->onChange = [this]
    {
        if (onRouteChanged)
            onRouteChanged(routeId, getCurrentRoute());
    };

    gainSlider->onValueChange = [this]
    {
        if (onRouteChanged)
            onRouteChanged(routeId, getCurrentRoute());
    };

    enableButton->onClick = [this]
    {
        if (onRouteChanged)
            onRouteChanged(routeId, getCurrentRoute());
    };

    removeButton->onClick = [this]
    {
        if (onRouteRemoved)
            onRouteRemoved(routeId);
    };

    // Selection on click - handled in mouseDown override
}

void SidechainRouteRow::mouseDown(const juce::MouseEvent& e)
{
    if (onRouteSelected)
        onRouteSelected(routeId);
}

void SidechainRouteRow::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    const int labelWidth = 50;
    const int comboWidth = 80;
    const int gainWidth = 100;
    const int buttonWidth = 30;
    const int gap = 6;

    int x = bounds.getX();
    int y = bounds.getY();
    int height = bounds.getHeight();

    // Source track
    sourceLabel->setBounds(x, y, labelWidth, 20);
    x += labelWidth;
    sourceTrackCombo->setBounds(x, y, comboWidth, 20);
    x += comboWidth + gap;

    // Target track
    targetLabel->setBounds(x, y, labelWidth, 20);
    x += labelWidth;
    targetTrackCombo->setBounds(x, y, comboWidth, 20);
    x += comboWidth + gap;

    // Plugin (smaller)
    pluginLabel->setBounds(x, y, labelWidth - 10, 20);
    x += labelWidth - 10;
    targetPluginCombo->setBounds(x, y, 80, 20);
    x += 80 + gap;

    // Gain
    gainLabel->setBounds(x, y, labelWidth - 20, 20);
    x += labelWidth - 20;
    gainSlider->setBounds(x, y, gainWidth, 20);
    x += gainWidth + gap;

    // Enable button
    enableButton->setBounds(x, y, 45, 20);
    x += 45 + gap;

    // Remove button
    removeButton->setBounds(x, y, buttonWidth, 20);
}

void SidechainRouteRow::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    if (selected)
    {
        // Highlight selected row with border
        g.setColour(getNeonPink().withAlpha(0.3f));
        g.drawRect(bounds, 2);

        g.setColour(getNeonPink().withAlpha(0.1f));
        g.fillRect(bounds);
    }
    else
    {
        // Subtle border for non-selected rows
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRect(bounds, 1);
    }
}

void SidechainRouteRow::updateMaxTracks(int newMaxTracks)
{
    maxTracks = newMaxTracks;

    // Update source track combo
    int currentSourceId = sourceTrackCombo->getSelectedId();
    sourceTrackCombo->clear(juce::dontSendNotification);
    sourceTrackCombo->addItem("None", 1);
    for (int i = 0; i < maxTracks; ++i)
    {
        sourceTrackCombo->addItem("Track " + juce::String(i + 1), i + 2);
    }
    sourceTrackCombo->setSelectedId(currentSourceId, juce::dontSendNotification);

    // Update target track combo
    int currentTargetId = targetTrackCombo->getSelectedId();
    targetTrackCombo->clear(juce::dontSendNotification);
    targetTrackCombo->addItem("None", 1);
    for (int i = 0; i < maxTracks; ++i)
    {
        targetTrackCombo->addItem("Track " + juce::String(i + 1), i + 2);
    }
    targetTrackCombo->setSelectedId(currentTargetId, juce::dontSendNotification);
}

void SidechainRouteRow::updatePluginsForTargetTrack(int targetTrackIndex,
                                                const std::vector<juce::String>& pluginNames,
                                                const std::vector<uint32_t>& pluginIds)
{
    int currentPluginId = targetPluginCombo->getSelectedId();

    targetPluginCombo->clear(juce::dontSendNotification);
    targetPluginCombo->addItem("All Plugins", 1);
    targetPluginIds.clear();

    for (size_t i = 0; i < pluginNames.size(); ++i)
    {
        targetPluginCombo->addItem(pluginNames[i], static_cast<int>(i + 2));
        targetPluginIds.push_back(pluginIds[i]);
    }

    // Only enable if target track is selected
    bool hasTarget = (targetTrackCombo->getSelectedId() > 1);
    targetPluginCombo->setEnabled(hasTarget && !pluginNames.empty());

    if (hasTarget)
    {
        targetPluginCombo->setSelectedId(currentPluginId, juce::dontSendNotification);
    }
    else
    {
        targetPluginCombo->setSelectedId(1, juce::dontSendNotification);
    }
}

SidechainRoute SidechainRouteRow::getCurrentRoute() const
{
    SidechainRoute route;
    route.sourceTrackIndex = sourceTrackCombo->getSelectedId() - 2;  // -2 because None=1
    route.targetTrackIndex = targetTrackCombo->getSelectedId() - 2;
    route.targetPluginNodeId = 0;  // TODO: Handle plugin selection
    route.gain = static_cast<float>(gainSlider->getValue());
    route.enabled = enableButton->getToggleState();

    return route;
}

void SidechainRouteRow::setSelected(bool sel)
{
    selected = sel;
    enableButton->setButtonText(selected ? "ON" : "ON");
    enableButton->setToggleState(selected, juce::dontSendNotification);
    repaint();
}

//==============================================================================
// SidechainRoutingPanel Implementation
//==============================================================================

SidechainRoutingPanel::SidechainRoutingPanel(AudioRoutingGraph& graph)
    : graph(graph)
{
    initializeComponents();
    setupListeners();
    refreshRoutes();
}

SidechainRoutingPanel::~SidechainRoutingPanel()
{
    clearRouteRows();
}

void SidechainRoutingPanel::initializeComponents()
{
    // Route list viewport
    routeListContainer = std::make_unique<juce::Component>();
    routeListViewport = std::make_unique<juce::Viewport>();
    routeListViewport->setViewedComponent(routeListContainer.get(), false);

    // Add route button
    addRouteButton = std::make_unique<juce::TextButton>();
    addRouteButton->setButtonText("+ Add Route");

    // Close button
    closeButton = std::make_unique<juce::TextButton>();
    closeButton->setButtonText("Close");

    // Title labels
    titleLabel = std::make_unique<juce::Label>();
    titleLabel->setText("Sidechain Routing", juce::dontSendNotification);
    titleLabel->setFont(juce::Font(18, juce::Font::bold));

    subtitleLabel = std::make_unique<juce::Label>();
    subtitleLabel->setText("Send audio from one track to plugins on another track", juce::dontSendNotification);
    subtitleLabel->setFont(juce::Font(12));
    subtitleLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Add components
    addAndMakeVisible(*titleLabel);
    addAndMakeVisible(*subtitleLabel);
    addAndMakeVisible(*routeListViewport);
    addAndMakeVisible(*addRouteButton);
    addAndMakeVisible(*closeButton);
}

void SidechainRoutingPanel::setupListeners()
{
    addRouteButton->onClick = [this]
    {
        // Create new route with default values
        SidechainRoute newRoute;
        newRoute.sourceTrackIndex = 0;
        newRoute.targetTrackIndex = 1;
        newRoute.gain = 1.0f;
        newRoute.enabled = true;

        int routeId = graph.addSidechainRoute(
            newRoute.sourceTrackIndex,
            newRoute.targetTrackIndex,
            newRoute.targetPluginNodeId,
            newRoute.busIndex,
            newRoute.gain
        );

        if (routeId >= 0)
        {
            refreshRoutes();
        }
    };

    closeButton->onClick = [this]
    {
        if (onClose)
            onClose();
    };
}

void SidechainRoutingPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    const int titleHeight = 24;
    const int subtitleHeight = 18;
    const int buttonHeight = 32;
    const int rowHeight = 32;
    const int gap = 12;

    // Title area
    titleLabel->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), titleHeight);
    bounds.removeFromTop(titleHeight + 4);

    subtitleLabel->setBounds(bounds.getX(), bounds.getY(), bounds.getWidth(), subtitleHeight);
    bounds.removeFromTop(subtitleHeight + gap);

    // Buttons at bottom
    int buttonY = bounds.getBottom() - buttonHeight - gap;
    addRouteButton->setBounds(bounds.getX(), buttonY, 120, buttonHeight);
    closeButton->setBounds(bounds.getRight() - 80, buttonY, 80, buttonHeight);
    bounds.removeFromBottom(buttonHeight + gap * 2);

    // Route list
    routeListViewport->setBounds(bounds);
    routeListContainer->setBounds(0, 0, bounds.getWidth(), routeRows.size() * (rowHeight + 4));
}

void SidechainRoutingPanel::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());

    // Header background
    auto bounds = getLocalBounds();
    auto headerBounds = bounds.removeFromTop(60);
    g.setColour(getNeonPink().withAlpha(0.1f));
    g.fillRect(headerBounds);
}

void SidechainRoutingPanel::refreshRoutes()
{
    clearRouteRows();

    const auto& routes = graph.getAllSidechainRoutes();

    for (const auto& route : routes)
    {
        if (route.isValid())
        {
            // Extract route ID from the hacky storage
            // In SidechainManager, routeId is stored in sourceTrackIndex
            int routeId = const_cast<SidechainRoute&>(route).sourceTrackIndex;

            // Restore the actual source track index
            int actualSource = 0;
            int actualTarget = 0;
            for (const auto& r : graph.getAllSidechainRoutes())
            {
                if (&r == &route)
                {
                    break;
                }
            }

            addRouteRow(routeId, route);
        }
    }
}

void SidechainRoutingPanel::updateTrackCount(int newTrackCount)
{
    trackCount = newTrackCount;

    for (auto& row : routeRows)
    {
        row->updateMaxTracks(trackCount);
    }

    routeListContainer->setBounds(0, 0, routeListContainer->getWidth(),
                                  routeRows.size() * 36);
}

void SidechainRoutingPanel::addRouteRow(int routeId, const SidechainRoute& route)
{
    auto row = std::make_unique<SidechainRouteRow>(routeId, route, trackCount);

    // Set up callbacks
    row->onRouteChanged = [this, routeId](int, const SidechainRoute& newRoute)
    {
        updateRouteRow(routeId);
    };

    row->onRouteRemoved = [this, routeId](int)
    {
        removeRouteRow(routeId);
    };

    row->onRouteSelected = [this, routeId](int)
    {
        selectedRouteId = routeId;
        for (auto& r : routeRows)
        {
            r->setSelected(r->getRouteId() == routeId);
        }
    };

    // Update plugin dropdown for target track
    std::vector<juce::String> pluginNames;
    std::vector<uint32_t> pluginIds;
    getPluginsForTrack(route.targetTrackIndex, pluginNames, pluginIds);
    row->updatePluginsForTargetTrack(route.targetTrackIndex, pluginNames, pluginIds);

    routeListContainer->addAndMakeVisible(*row);
    routeRows.push_back(std::move(row));
    routeIds.push_back(routeId);

    // Update container size
    updatePluginDropdowns();
}

void SidechainRoutingPanel::removeRouteRow(int routeId)
{
    graph.removeSidechainRoute(routeId);

    auto it = std::find(routeIds.begin(), routeIds.end(), routeId);
    if (it != routeIds.end())
    {
        int index = static_cast<int>(it - routeIds.begin());
        routeIds.erase(it);
        routeRows.erase(routeRows.begin() + index);
    }

    // Update container size
    routeListContainer->setBounds(0, 0, routeListContainer->getWidth(),
                                  routeRows.size() * 36);

    if (selectedRouteId == routeId)
    {
        selectedRouteId = -1;
    }
}

void SidechainRoutingPanel::updateRouteRow(int routeId)
{
    auto it = std::find(routeIds.begin(), routeIds.end(), routeId);
    if (it != routeIds.end())
    {
        int index = static_cast<int>(it - routeIds.begin());

        if (index < static_cast<int>(routeRows.size()))
        {
            auto& row = routeRows[index];
            SidechainRoute currentRoute = row->getCurrentRoute();

            graph.updateSidechainRoute(
                routeId,
                currentRoute.sourceTrackIndex,
                currentRoute.targetTrackIndex,
                currentRoute.targetPluginNodeId,
                currentRoute.busIndex,
                currentRoute.gain
            );

            graph.setSidechainRouteEnabled(routeId, currentRoute.enabled);

            // Update plugin dropdown if target track changed
            std::vector<juce::String> pluginNames;
            std::vector<uint32_t> pluginIds;
            getPluginsForTrack(currentRoute.targetTrackIndex, pluginNames, pluginIds);
            row->updatePluginsForTargetTrack(currentRoute.targetTrackIndex, pluginNames, pluginIds);
        }
    }
}

void SidechainRoutingPanel::clearRouteRows()
{
    routeRows.clear();
    routeIds.clear();
    routeListContainer->removeAllChildren();
}

void SidechainRoutingPanel::updatePluginDropdowns()
{
    // Update all route rows with current plugin list
    for (auto& row : routeRows)
    {
        SidechainRoute route = row->getCurrentRoute();

        std::vector<juce::String> pluginNames;
        std::vector<uint32_t> pluginIds;
        getPluginsForTrack(route.targetTrackIndex, pluginNames, pluginIds);
        row->updatePluginsForTargetTrack(route.targetTrackIndex, pluginNames, pluginIds);
    }
}

void SidechainRoutingPanel::getPluginsForTrack(int trackIndex,
                                             std::vector<juce::String>& pluginNames,
                                             std::vector<uint32_t>& pluginIds)
{
    pluginNames.clear();
    pluginIds.clear();

    if (trackIndex < 0 || trackIndex >= graph.maxTracks)
        return;

    // Get plugin chain for this track
    const auto* chain = graph.getTrackPluginChain(trackIndex);
    if (!chain)
        return;

    for (NodeID pluginId : chain->pluginNodeIds)
    {
        auto pluginInstance = graph.getPluginInstance(pluginId);
        if (pluginInstance && pluginInstance->hasEditor())
        {
            pluginNames.push_back(pluginInstance->getName());
            pluginIds.push_back(pluginId);
        }
    }
}
