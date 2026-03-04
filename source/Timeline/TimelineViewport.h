#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ThemeManager.h"
#include "TimelineData.h"
#include "TimelineTransport.h"

// Forward declarations
class TimelineRenderer;
class TimelinePlayHead;

/**
 * TimelineViewport - Professionelle Timeline-Navigation
 *
 * Features:
 *   - Mouse-Wheel Zoom (horizontal & vertical)
 *   - Drag-to-Scroll
 *   - Playhead-Scroll-Tracking
 *   - Minimap
 *   - Zoom-Level-Indikator
 */
class TimelineViewport : public juce::Component
{
public:
    TimelineViewport(TimelineRenderer& renderer, TimelineData& data, TimelineTransport& transport);
    ~TimelineViewport() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    void mouseWheelMove(const juce::MouseEvent& e,
                       const juce::MouseWheelDetails& wheel) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Zoom control
    void setHorizontalZoom(double zoom);
    double getHorizontalZoom() const { return horizontalZoom; }
    void setVerticalZoom(double zoom);
    double getVerticalZoom() const { return verticalZoom; }

    void zoomIn();
    void zoomOut();
    void resetZoom();

    // Scroll control
    void setScrollX(double x);
    void setScrollY(double y);
    double getScrollX() const { return scrollX; }
    double getScrollY() const { return scrollY; }

    // Playhead tracking
    void setPlayheadTracking(bool track);
    bool isPlayheadTracking() const { return playheadTracking; }

    // Minimap
    void showMinimap(bool show);
    bool isMinimapVisible() const { return minimapVisible; }

    // Timeline data
    void setTimelineData(TimelineData* data);
    void setTimelineTransport(TimelineTransport* transport);

    // Callbacks
    std::function<void(double)> onHorizontalZoomChanged;
    std::function<void(double)> onVerticalZoomChanged;
    std::function<void(double, double)> onScrollChanged;

private:
    TimelineRenderer& renderer;
    TimelineData* timelineData = nullptr;
    TimelineTransport* timelineTransport = nullptr;

    // Zoom state
    double horizontalZoom = 1.0;      // Pixels per beat
    double verticalZoom = 1.0;         // Pixels per track
    double minHorizontalZoom = 0.25;
    double maxHorizontalZoom = 8.0;
    double minVerticalZoom = 0.5;
    double maxVerticalZoom = 4.0;

    // Scroll state
    double scrollX = 0.0;
    double scrollY = 0.0;
    double maxScrollX = 0.0;
    double maxScrollY = 0.0;

    // Drag state
    bool isDragging = false;
    juce::Point<float> dragStartPos;
    double dragStartScrollX = 0.0;
    double dragStartScrollY = 0.0;

    // Playhead tracking
    bool playheadTracking = false;

    // Minimap
    bool minimapVisible = true;
    juce::Rectangle<float> minimapRect;
    juce::Rectangle<float> minimapViewportRect;

    // UI Components
    std::unique_ptr<juce::Component> zoomControlPanel;
    std::unique_ptr<juce::TextButton> zoomInButton;
    std::unique_ptr<juce::TextButton> zoomOutButton;
    std::unique_ptr<juce::TextButton> resetZoomButton;
    std::unique_ptr<juce::Label> zoomLevelLabel;

    void initializeComponents();
    void layoutComponents();
    void setupListeners();

    void updateMaxScroll();
    void clampScroll();
    void clampZoom();

    void drawMinimap(juce::Graphics& g);
    void drawZoomControls(juce::Graphics& g);

    juce::String formatZoomLevel(double zoom) const;
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getPanelBackgroundColor(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineViewport)
};
