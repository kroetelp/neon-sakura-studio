#include "TimelineViewport.h"
#include "TimelineRenderer.h"
#include <melatonin_blur.h>

//==============================================================================
// TimelineViewport Implementation
//==============================================================================

TimelineViewport::TimelineViewport(TimelineRenderer& renderer, TimelineData& data, TimelineTransport& transport)
    : renderer(renderer)
{
    setTimelineData(&data);
    setTimelineTransport(&transport);

    initializeComponents();
    setupListeners();
}

TimelineViewport::~TimelineViewport() = default;

void TimelineViewport::initializeComponents()
{
    // Zoom control panel
    zoomControlPanel = std::make_unique<juce::Component>();

    // Zoom buttons
    zoomInButton = std::make_unique<juce::TextButton>();
    zoomInButton->setButtonText("+");
    zoomInButton->setTooltip("Zoom In");
    zoomInButton->setSize(24, 24);

    zoomOutButton = std::make_unique<juce::TextButton>();
    zoomOutButton->setButtonText("-");
    zoomOutButton->setTooltip("Zoom Out");
    zoomOutButton->setSize(24, 24);

    resetZoomButton = std::make_unique<juce::TextButton>();
    resetZoomButton->setButtonText("1:1");
    resetZoomButton->setTooltip("Reset Zoom");
    resetZoomButton->setSize(40, 24);

    // Zoom level label
    zoomLevelLabel = std::make_unique<juce::Label>();
    zoomLevelLabel->setText(formatZoomLevel(horizontalZoom), juce::dontSendNotification);
    zoomLevelLabel->setFont(juce::Font(10));
    zoomLevelLabel->setJustificationType(juce::Justification::centred);

    // Add to panel
    zoomControlPanel->addAndMakeVisible(*zoomInButton);
    zoomControlPanel->addAndMakeVisible(*zoomOutButton);
    zoomControlPanel->addAndMakeVisible(*resetZoomButton);
    zoomControlPanel->addAndMakeVisible(*zoomLevelLabel);
}

void TimelineViewport::setupListeners()
{
    zoomInButton->onClick = [this]
    {
        zoomIn();
    };

    zoomOutButton->onClick = [this]
    {
        zoomOut();
    };

    resetZoomButton->onClick = [this]
    {
        resetZoom();
    };
}

void TimelineViewport::resized()
{
    auto bounds = getLocalBounds();

    // Update max scroll based on new size
    updateMaxScroll();

    // Layout zoom controls (bottom right corner)
    int zoomPanelWidth = 120;
    int zoomPanelHeight = 32;
    int zoomPanelX = bounds.getRight() - zoomPanelWidth - 8;
    int zoomPanelY = bounds.getBottom() - zoomPanelHeight - 8;

    zoomControlPanel->setBounds(zoomPanelX, zoomPanelY, zoomPanelWidth, zoomPanelHeight);

    // Layout buttons in panel
    zoomInButton->setBounds(0, 4, 24, 24);
    zoomOutButton->setBounds(30, 4, 24, 24);
    resetZoomButton->setBounds(60, 4, 40, 24);
    zoomLevelLabel->setBounds(105, 4, 35, 24);

    // Update minimap position (top right)
    if (minimapVisible)
    {
        const int minimapWidth = 200;
        const int minimapHeight = 80;
        minimapRect = juce::Rectangle<float>(
            bounds.getRight() - minimapWidth - 8,
            bounds.getY() + 8,
            static_cast<float>(minimapWidth),
            static_cast<float>(minimapHeight)
        );
    }
}

void TimelineViewport::paint(juce::Graphics& g)
{
    g.fillAll(getDarkBackground());

    // Draw minimap if visible
    if (minimapVisible)
    {
        drawMinimap(g);
    }

    // Draw zoom controls background
    auto zoomBounds = zoomControlPanel->getBounds();
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(zoomBounds.toFloat(), 4);
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(zoomBounds.toFloat(), 4, 1);
}

void TimelineViewport::mouseWheelMove(const juce::MouseEvent& e,
                                   const juce::MouseWheelDetails& wheel)
{
    // Horizontal zoom (Ctrl + scroll)
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        double zoomFactor = 1.0 + (wheel.deltaY > 0 ? -0.1 : 0.1);
        setHorizontalZoom(horizontalZoom * zoomFactor);
    }
    // Horizontal scroll (Shift + scroll)
    else if (e.mods.isShiftDown())
    {
        scrollX += wheel.deltaY * 50 * horizontalZoom;
        clampScroll();
        repaint();
    }
    // Vertical scroll (normal scroll)
    else
    {
        scrollY += wheel.deltaY * 50 * verticalZoom;
        clampScroll();
        repaint();
    }
}

void TimelineViewport::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging)
        return;

    double dragDeltaX = e.position.x - dragStartPos.x;
    double dragDeltaY = e.position.y - dragStartPos.y;

    scrollX = dragStartScrollX - dragDeltaX;
    scrollY = dragStartScrollY - dragDeltaY;

    clampScroll();
    repaint();
}

void TimelineViewport::mouseDown(const juce::MouseEvent& e)
{
    // Check if clicking on zoom controls
    if (zoomControlPanel->getBounds().contains(e.position.toInt()))
    {
        isDragging = false;
        return;
    }

    // Start drag for scrolling
    if (e.mods.isMiddleButtonDown() || (e.mods.isLeftButtonDown() && !e.mods.isCommandDown()))
    {
        isDragging = true;
        dragStartPos = e.position.toFloat();
        dragStartScrollX = scrollX;
        dragStartScrollY = scrollY;
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
}

void TimelineViewport::mouseUp(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        isDragging = false;
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void TimelineViewport::setHorizontalZoom(double zoom)
{
    horizontalZoom = juce::jlimit(zoom, minHorizontalZoom, maxHorizontalZoom);
    updateMaxScroll();
    clampScroll();
    zoomLevelLabel->setText(formatZoomLevel(horizontalZoom), juce::dontSendNotification);

    if (onHorizontalZoomChanged)
        onHorizontalZoomChanged(horizontalZoom);

    repaint();
}

void TimelineViewport::setVerticalZoom(double zoom)
{
    verticalZoom = juce::jlimit(zoom, minVerticalZoom, maxVerticalZoom);
    updateMaxScroll();
    clampScroll();
    repaint();
}

void TimelineViewport::zoomIn()
{
    setHorizontalZoom(horizontalZoom * 1.25);
    setVerticalZoom(verticalZoom * 1.1);
}

void TimelineViewport::zoomOut()
{
    setHorizontalZoom(horizontalZoom * 0.8);
    setVerticalZoom(verticalZoom * 0.9);
}

void TimelineViewport::resetZoom()
{
    setHorizontalZoom(1.0);
    setVerticalZoom(1.0);
    setScrollX(0.0);
    setScrollY(0.0);
}

void TimelineViewport::setScrollX(double x)
{
    scrollX = x;
    clampScroll();
    repaint();
}

void TimelineViewport::setScrollY(double y)
{
    scrollY = y;
    clampScroll();
    repaint();
}

void TimelineViewport::setPlayheadTracking(bool track)
{
    playheadTracking = track;
}

void TimelineViewport::showMinimap(bool show)
{
    minimapVisible = show;
    resized();
    repaint();
}

void TimelineViewport::setTimelineData(TimelineData* data)
{
    timelineData = data;
    updateMaxScroll();
}

void TimelineViewport::setTimelineTransport(TimelineTransport* transport)
{
    timelineTransport = transport;
}

void TimelineViewport::updateMaxScroll()
{
    if (!timelineData)
        return;

    auto bounds = getLocalBounds();
    int numTracks = timelineData->getNumTracks();
    int totalBeats = static_cast<int>(timelineData->getTotalLengthBeats());

    maxScrollX = std::max(0.0, totalBeats * horizontalZoom - bounds.getWidth());
    maxScrollY = std::max(0.0, numTracks * verticalZoom - bounds.getHeight());
}

void TimelineViewport::clampScroll()
{
    scrollX = std::max(0.0, std::min(scrollX, maxScrollX));
    scrollY = std::max(0.0, std::min(scrollY, maxScrollY));

    if (onScrollChanged)
        onScrollChanged(scrollX, scrollY);
}

void TimelineViewport::clampZoom()
{
    horizontalZoom = juce::jlimit(horizontalZoom, minHorizontalZoom, maxHorizontalZoom);
    verticalZoom = juce::jlimit(verticalZoom, minVerticalZoom, maxVerticalZoom);
}

void TimelineViewport::drawMinimap(juce::Graphics& g)
{
    if (!minimapVisible)
        return;

    // Minimap background
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(minimapRect, 4);

    // Minimap border
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(minimapRect, 4, 1);

    // Draw minimap tracks (simplified representation)
    if (!timelineData)
        return;

    float trackHeight = minimapRect.getHeight() / timelineData->getNumTracks();

    for (int i = 0; i < timelineData->getNumTracks(); ++i)
    {
        float y = minimapRect.getY() + i * trackHeight;
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRect(minimapRect.getX(), y, minimapRect.getWidth() - 1, trackHeight - 1);

        // Draw clips as small rectangles
        const auto& trackRef = timelineData->getTrack(i);
        const TimelineTrack* track = &trackRef;
        if (track != nullptr)
        {
            std::vector<TimelineClip*> clips = track->getClips();
            for (const auto* clip : clips)
            {
                if (clip != nullptr)
                {
                    float clipX = minimapRect.getX() +
                        (clip->startBeat.load() / timelineData->getTotalLengthBeats()) * (minimapRect.getWidth() - 1);
                    float clipWidth = ((clip->startBeat.load() + clip->lengthBeats.load()) / timelineData->getTotalLengthBeats()) *
                                           (minimapRect.getWidth() - 1);

                    g.setColour(getNeonPink().withAlpha(0.4f));
                    g.fillRect(clipX, y + 1, clipWidth, trackHeight - 2);
                }
            }
        }
    }

    // Draw viewport indicator
    float viewportWidth = getWidth() / (maxScrollX + getWidth()) * (minimapRect.getWidth() - 1);
    float viewportHeight = getHeight() / (maxScrollY + getHeight()) * minimapRect.getHeight();

    minimapViewportRect = juce::Rectangle<float>(
        minimapRect.getX() + (scrollX / (maxScrollX + getWidth())) * (minimapRect.getWidth() - 1),
        minimapRect.getY() + (scrollY / (maxScrollY + getHeight())) * minimapRect.getHeight(),
        viewportWidth,
        viewportHeight
    );

    g.setColour(getNeonPink().withAlpha(0.3f));
    g.fillRect(minimapViewportRect);
}

void TimelineViewport::drawZoomControls(juce::Graphics& g)
{
    // Already drawn as a separate component
}

juce::String TimelineViewport::formatZoomLevel(double zoom) const
{
    if (zoom >= 1.0)
        return juce::String::formatted("%.0fx", zoom);
    else
        return juce::String::formatted("1/%.0fx", 1.0 / zoom);
}
