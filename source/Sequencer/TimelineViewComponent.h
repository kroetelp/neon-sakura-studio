// ============================================================================
// TimelineViewComponent.h - Timeline-basierte View für UnifiedSequencerPanel
// ============================================================================
//
// Diese View zeigt eine DAW-artige Timeline mit Multi-Track-Audio.

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>
#include "../Theme/ThemeManager.h"

// Forward declarations
class UnifiedSequencerModel;
class TimelineData;
class TrackManager;

/**
 * TimelineViewComponent - Timeline-basierte Darstellung
 *
 * Diese View zeigt:
 * - Multi-Track Timeline wie in einer DAW
 * - Audio Clips mit Wellenform-Visualisierung
 * - Automation-Lanes
 * - Playhead und Time-Ruler
 */
class TimelineViewComponent : public juce::Component
{
public:
    TimelineViewComponent();
    ~TimelineViewComponent() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Dependencies ===
    void setModel(UnifiedSequencerModel* model);
    void setTimelineData(TimelineData* data);
    void setTrackManager(TrackManager* manager);

    // === Timeline Navigation ===
    void setScrollPosition(double seconds);
    double getScrollPosition() const { return scrollPosition; }
    void setZoomLevel(double pixelsPerSecond);
    double getZoomLevel() const { return pixelsPerSecond; }

    // === Playback ===
    void setPlayheadPosition(double seconds);
    double getPlayheadPosition() const { return playheadPosition; }
    void setLoopEnabled(bool enabled);
    void setLoopRange(double start, double end);

    // === Callbacks ===
    std::function<void(double position)> onPlayheadMoved;
    std::function<void(int trackIndex)> onTrackClicked;
    std::function<void()> onSelectionChanged;

private:
    // === Dependencies ===
    UnifiedSequencerModel* model = nullptr;
    TimelineData* timelineData = nullptr;
    TrackManager* trackManager = nullptr;

    // === Timeline State ===
    double scrollPosition = 0.0;        // Scroll-Position in Sekunden
    double playheadPosition = 0.0;     // Playhead in Sekunden
    double pixelsPerSecond = 100.0;     // Zoom-Level
    double totalTime = 60.0;            // Gesamte Timeline-Dauer in Sekunden
    double loopStart = 0.0;
    double loopEnd = 16.0;
    bool loopEnabled = false;
    bool isPlaying = false;

    // === Layout Constants ===
    static constexpr int headerWidth = 150;      // Track Header Breite
    static constexpr int rulerHeight = 30;       // Time-Ruler Höhe
    static constexpr int trackHeight = 80;       // Standard Track-Höhe

    // === UI Components ===
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton loopButton;
    juce::Slider zoomSlider;

    juce::Label positionLabel;
    juce::Label bpmLabel;

    // === Mouse State ===
    bool isDraggingPlayhead = false;
    bool isScrolling = false;
    int lastMouseX = 0;

    // === Colors ===
    juce::Colour getNeonPink() const { return ThemeManager::getInstance().getAccentColor(); }
    juce::Colour getNeonCyan() const { return ThemeManager::getInstance().getInfoColor(); }
    juce::Colour getDarkBackground() const { return ThemeManager::getInstance().getBackgroundColor(); }
    juce::Colour getGridLineColour() const { return juce::Colours::darkgrey.withAlpha(0.5f); }

    // === Setup ===
    void createControls();
    void setupSlider(juce::Slider& slider, double min, double max, double initial);

    // === Mouse Handling ===
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // === Drawing ===
    void drawTimeRuler(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawTrackHeaders(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawTracks(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& bounds);
    void drawLoopMarkers(juce::Graphics& g, const juce::Rectangle<int>& bounds);

    // === Helper ===
    int timeToPixels(double seconds) const;
    double pixelsToTime(int pixels) const;
    juce::String formatTime(double seconds) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineViewComponent)
};
