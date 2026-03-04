// ============================================================================
// TimelineViewComponent.cpp - Timeline-basierte View Implementierung
// ============================================================================

#include "TimelineViewComponent.h"
#include "UnifiedSequencerModel.h"
#include "../Timeline/TimelineData.h"
#include "../TrackManager.h"

// ============================================================================
// TimelineViewComponent Implementierung
// ============================================================================

TimelineViewComponent::TimelineViewComponent()
{
    createControls();

    // Initialer Playback-Status
    playheadPosition = 0.0;
    scrollPosition = 0.0;
}

void TimelineViewComponent::createControls()
{
    // Playback Controls
    playButton.setButtonText("Play");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(20, 80, 20));
    playButton.onClick = [this]()
    {
        isPlaying = !isPlaying;
        playButton.setButtonText(isPlaying ? "Pause" : "Play");
        playButton.setColour(juce::TextButton::buttonColourId,
            isPlaying ? juce::Colour(80, 80, 20) : juce::Colour(20, 80, 20));
    };
    addAndMakeVisible(playButton);

    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 20, 20));
    stopButton.onClick = [this]()
    {
        isPlaying = false;
        playheadPosition = 0.0;
        playButton.setButtonText("Play");
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(20, 80, 20));
        repaint();
    };
    addAndMakeVisible(stopButton);

    loopButton.setButtonText("Loop: Off");
    loopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 60, 60));
    loopButton.onClick = [this]()
    {
        loopEnabled = !loopEnabled;
        loopButton.setButtonText(loopEnabled ? "Loop: On" : "Loop: Off");
        loopButton.setColour(juce::TextButton::buttonColourId,
            loopEnabled ? getNeonCyan() : juce::Colour(60, 60, 60));
        repaint();
    };
    addAndMakeVisible(loopButton);

    // Zoom Slider
    setupSlider(zoomSlider, 10.0, 500.0, 100.0);
    zoomSlider.onValueChange = [this]()
    {
        setZoomLevel(zoomSlider.getValue());
        repaint();
    };
    addAndMakeVisible(zoomSlider);

    // Info Labels
    positionLabel.setText("00:00:000", juce::dontSendNotification);
    positionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    positionLabel.setFont(juce::Font(14.0f));
    addAndMakeVisible(positionLabel);

    bpmLabel.setText("120 BPM", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    bpmLabel.setFont(juce::Font(14.0f));
    addAndMakeVisible(bpmLabel);
}

void TimelineViewComponent::setupSlider(juce::Slider& slider, double min, double max, double initial)
{
    slider.setRange(min, max);
    slider.setValue(initial);
    slider.setSliderStyle(juce::Slider::LinearBar);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    slider.setColour(juce::Slider::thumbColourId, getNeonPink());
}

// ============================================================================
// Dependencies
// ============================================================================

void TimelineViewComponent::setModel(UnifiedSequencerModel* model)
{
    this->model = model;
    repaint();
}

void TimelineViewComponent::setTimelineData(TimelineData* data)
{
    this->timelineData = data;

    if (data)
    {
        // Lade Timeline-Daten
        // TODO: Implementiere Daten-Auslesen
    }

    repaint();
}

void TimelineViewComponent::setTrackManager(TrackManager* manager)
{
    this->trackManager = manager;
    repaint();
}

// ============================================================================
// Timeline Navigation
// ============================================================================

void TimelineViewComponent::setScrollPosition(double seconds)
{
    scrollPosition = juce::jlimit(0.0, totalTime - 1.0, seconds);
    repaint();
}

void TimelineViewComponent::setZoomLevel(double pixelsPerSecond)
{
    this->pixelsPerSecond = juce::jlimit(10.0, 500.0, pixelsPerSecond);
    zoomSlider.setValue(this->pixelsPerSecond, juce::dontSendNotification);
}

// ============================================================================
// Playback
// ============================================================================

void TimelineViewComponent::setPlayheadPosition(double seconds)
{
    playheadPosition = juce::jlimit(0.0, totalTime, seconds);
    positionLabel.setText(formatTime(playheadPosition), juce::dontSendNotification);
    repaint();
}

void TimelineViewComponent::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    loopButton.setButtonText(loopEnabled ? "Loop: On" : "Loop: Off");
    loopButton.setColour(juce::TextButton::buttonColourId,
        loopEnabled ? getNeonCyan() : juce::Colour(60, 60, 60));
    repaint();
}

void TimelineViewComponent::setLoopRange(double start, double end)
{
    loopStart = juce::jlimit(0.0, totalTime, start);
    loopEnd = juce::jlimit(loopStart, totalTime, end);
    repaint();
}

// ============================================================================
// Mouse Handling
// ============================================================================

void TimelineViewComponent::mouseDown(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().withTrimmedTop(rulerHeight).withTrimmedLeft(headerWidth);

    if (e.mods.isLeftButtonDown() && bounds.contains(e.getPosition()))
    {
        // Klick auf Timeline - bewege Playhead
        double time = pixelsToTime(e.position.x - headerWidth);
        setPlayheadPosition(time);

        if (onPlayheadMoved)
            onPlayheadMoved(time);
    }
}

void TimelineViewComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingPlayhead)
    {
        double time = pixelsToTime(e.position.x - headerWidth);
        setPlayheadPosition(time);

        if (onPlayheadMoved)
            onPlayheadMoved(time);
    }
}

void TimelineViewComponent::mouseWheelMove(const juce::MouseEvent& e,
    const juce::MouseWheelDetails& wheel)
{
    // Scroll horizontal mit Mausrad
    if (std::abs(wheel.deltaX) > 0.0f)
    {
        double delta = wheel.deltaX * 0.5;
        setScrollPosition(scrollPosition + delta);
    }
    else if (std::abs(wheel.deltaY) > 0.0f)
    {
        // Zoom mit Mausrad (Alt gedrückt)
        if (e.mods.isAltDown())
        {
            double zoomFactor = wheel.deltaY > 0 ? 1.1 : 0.9;
            setZoomLevel(pixelsPerSecond * zoomFactor);
        }
        else
        {
            // Scroll
            double delta = wheel.deltaY * 0.5;
            setScrollPosition(scrollPosition + delta);
        }
    }
}

// ============================================================================
// Helper
// ============================================================================

int TimelineViewComponent::timeToPixels(double seconds) const
{
    return static_cast<int>((seconds - scrollPosition) * pixelsPerSecond);
}

double TimelineViewComponent::pixelsToTime(int pixels) const
{
    return scrollPosition + (pixels / pixelsPerSecond);
}

juce::String TimelineViewComponent::formatTime(double seconds) const
{
    int mins = static_cast<int>(seconds / 60.0);
    int secs = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);

    return juce::String::formatted("%02d:%02d:%03d", mins, secs, ms);
}

// ============================================================================
// Drawing
// ============================================================================

void TimelineViewComponent::resized()
{
    auto bounds = getLocalBounds();

    // Header Controls oben
    auto controlsBounds = bounds.removeFromTop(rulerHeight);

    int buttonWidth = 80;
    playButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(4));
    stopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(4));
    loopButton.setBounds(controlsBounds.removeFromLeft(buttonWidth).reduced(4));

    int sliderWidth = 150;
    zoomSlider.setBounds(controlsBounds.removeFromLeft(sliderWidth).reduced(4));

    bpmLabel.setBounds(controlsBounds.removeFromLeft(80).reduced(4));
    positionLabel.setBounds(controlsBounds.removeFromLeft(100).reduced(4));
}

void TimelineViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());

    auto bounds = getLocalBounds();

    // Time Ruler oben
    auto rulerBounds = bounds.removeFromTop(rulerHeight);
    drawTimeRuler(g, rulerBounds);

    // Track Header links
    auto headerBounds = bounds.removeFromLeft(headerWidth);
    drawTrackHeaders(g, headerBounds);

    // Timeline Tracks
    auto tracksBounds = bounds;
    drawTracks(g, tracksBounds);

    // Playhead
    drawPlayhead(g, bounds);

    // Loop Markers
    if (loopEnabled)
        drawLoopMarkers(g, bounds);
}

void TimelineViewComponent::drawTimeRuler(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.fillAll(juce::Colour(25, 25, 35));

    g.setColour(getGridLineColour());

    // Zeitskalen basierend auf Zoom-Level
    double majorInterval = 1.0;  // Sekunden
    if (pixelsPerSecond > 200)
        majorInterval = 0.5;
    else if (pixelsPerSecond > 50)
        majorInterval = 1.0;
    else if (pixelsPerSecond > 20)
        majorInterval = 2.0;
    else
        majorInterval = 5.0;

    // Zeichne Markierungen
    double start = scrollPosition;
    double end = scrollPosition + (bounds.getWidth() - headerWidth) / pixelsPerSecond;

    for (double t = std::floor(start / majorInterval) * majorInterval; t <= end; t += majorInterval)
    {
        int x = headerWidth + timeToPixels(t);

        if (x >= headerWidth && x <= bounds.getWidth())
        {
            // Major Tick
            g.drawLine((float)x, 0, (float)x, (float)bounds.getHeight(), 2.0f);

            // Time Label
            g.setColour(juce::Colours::lightgrey);
            g.setFont(juce::Font(10.0f));
            g.drawText(formatTime(t), x + 4, 8, 50, 14, juce::Justification::centredLeft);
        }
    }

    // Divider Line
    g.setColour(juce::Colours::darkgrey);
    g.drawLine((float)headerWidth, 0, (float)headerWidth, (float)bounds.getHeight(), 1.0f);
}

void TimelineViewComponent::drawTrackHeaders(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    g.fillAll(juce::Colour(35, 35, 45));

    int numTracks = 8;  // TODO: Aus TrackManager lesen
    int heightPerTrack = bounds.getHeight() / numTracks;

    auto workingBounds = bounds;

    for (int i = 0; i < numTracks; ++i)
    {
        auto trackBounds = workingBounds.removeFromTop(heightPerTrack);

        // Track Background
        g.setColour(juce::Colour(40, 40, 50));
        g.fillRect(trackBounds);

        // Track Label
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("Track " + juce::String(i + 1),
            trackBounds.reduced(8),
            juce::Justification::centredLeft);

        // Divider
        g.setColour(juce::Colours::darkgrey);
        g.drawLine((float)trackBounds.getX(), (float)trackBounds.getBottom(),
            (float)trackBounds.getRight(), (float)trackBounds.getBottom(), 1.0f);
    }
}

void TimelineViewComponent::drawTracks(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    // Track Backgrounds
    int numTracks = 8;
    int heightPerTrack = bounds.getHeight() / numTracks;

    auto workingBounds = bounds;

    for (int i = 0; i < numTracks; ++i)
    {
        auto trackBounds = workingBounds.removeFromTop(heightPerTrack);

        // Alternierende Track-Farben
        if (i % 2 == 0)
            g.setColour(juce::Colour(25, 25, 35));
        else
            g.setColour(juce::Colour(28, 28, 38));

        g.fillRect(trackBounds);

        // Divider
        g.setColour(juce::Colours::darkgrey);
        g.drawLine((float)trackBounds.getX(), (float)trackBounds.getBottom(),
            (float)trackBounds.getRight(), (float)trackBounds.getBottom(), 1.0f);

        // Grid Lines (Bars)
        double barDuration = 4.0;  // 4 Sekunden pro Beat bei 15 BPM? TODO: Aus BPM berechnen
        double start = scrollPosition;
        double end = scrollPosition + bounds.getWidth() / pixelsPerSecond;

        g.setColour(getGridLineColour());

        for (double t = std::floor(start / barDuration) * barDuration; t <= end; t += barDuration)
        {
            int x = timeToPixels(t);
            if (x >= 0 && x <= bounds.getWidth())
            {
                g.drawLine((float)x, (float)trackBounds.getY(),
                    (float)x, (float)trackBounds.getBottom(), 1.0f);
            }
        }
    }
}

void TimelineViewComponent::drawPlayhead(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int x = timeToPixels(playheadPosition);

    // Playhead Line
    g.setColour(getNeonCyan());
    g.drawLine((float)x, 0, (float)x, (float)bounds.getHeight(), 2.0f);

    // Playhead Cap
    auto capBounds = juce::Rectangle<int>(x - 6, 0, 12, 15);
    g.setColour(getNeonCyan());
    g.drawRoundedRectangle(capBounds.toFloat(), 3.0f, 2.0f);
    g.fillRoundedRectangle(capBounds.toFloat(), 3.0f);
}

void TimelineViewComponent::drawLoopMarkers(juce::Graphics& g, const juce::Rectangle<int>& bounds)
{
    int startX = timeToPixels(loopStart);
    int endX = timeToPixels(loopEnd);

    if (endX > startX)
    {
        // Loop Region Background
        auto loopBounds = juce::Rectangle<int>(startX, rulerHeight, endX - startX, bounds.getHeight());
        g.setColour(getNeonCyan().withAlpha(0.15f));
        g.fillRect(loopBounds);

        // Loop Markers
        g.setColour(getNeonCyan());
        g.drawLine((float)startX, (float)rulerHeight, (float)startX, (float)bounds.getHeight(), 2.0f);
        g.drawLine((float)endX, (float)rulerHeight, (float)endX, (float)bounds.getHeight(), 2.0f);

        // Loop Marker Handles
        auto startHandle = juce::Rectangle<int>(startX - 4, rulerHeight + 4, 8, 20);
        auto endHandle = juce::Rectangle<int>(endX - 4, rulerHeight + 4, 8, 20);

        g.fillRoundedRectangle(startHandle.toFloat(), 2.0f);
        g.fillRoundedRectangle(endHandle.toFloat(), 2.0f);
    }
}
