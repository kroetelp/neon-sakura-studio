#include "TimelineComponent.h"
#include <cmath>

TimelineComponent::TimelineComponent(TimelineData& data, RecordingManager& recordingMgr)
    : timelineData(data)
    , recordingManager(recordingMgr)
{
    setOpaque(false);
    setInterceptsMouseClicks(true, true);
    startTimerHz(30); // 30 FPS for playhead updates

    // Register audio formats for sample loading
    formatManager.registerBasicFormats();

    initializeToolbar();
    initializeTrackControls();
}

TimelineComponent::~TimelineComponent()
{
    stopTimer();
}

void TimelineComponent::initializeToolbar()
{
    // Record button
    recordButton.setButtonText("Rec");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(100, 30, 30));
    recordButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    recordButton.onClick = [this] { toggleRecording(); };
    addAndMakeVisible(recordButton);

    // Loop button
    loopButton.setButtonText("Loop");
    loopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 80, 50));
    loopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loopButton.setClickingTogglesState(true);
    loopButton.onClick = [this] { toggleLoop(); };
    addAndMakeVisible(loopButton);

    // Delete button
    deleteButton.setButtonText("Delete");
    deleteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 30, 30));
    deleteButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    deleteButton.onClick = [this] { deleteSelectedClip(); };
    addAndMakeVisible(deleteButton);

    // Import Sample button
    importSampleButton.setButtonText("Import");
    importSampleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0, 100, 80));
    importSampleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    importSampleButton.onClick = [this] {
        juce::FileChooser chooser("Import Audio Sample",
                                   juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                   "*.wav;*.aiff;*.mp3;*.flac;*.ogg", true);
        chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                if (fc.getResults().size() > 0)
                {
                    const auto& file = fc.getResult();
                    // Import at playhead position on first track
                    double startBeat = timelineData.playheadBeat.load();
                    loadSampleIntoTimeline(file, 0, startBeat);
                }
            });
    };
    addAndMakeVisible(importSampleButton);

    // Zoom label
    zoomLabel.setText("Zoom:", juce::dontSendNotification);
    zoomLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(zoomLabel);

    // Zoom slider
    zoomSlider.setRange(5.0, 200.0, 1.0);
    zoomSlider.setValue(horizontalZoom);
    zoomSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    zoomSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    zoomSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xff00b894));
    zoomSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    zoomSlider.onValueChange = [this] {
        setHorizontalZoom(zoomSlider.getValue());
    };
    addAndMakeVisible(zoomSlider);

    // BPM label
    bpmLabel.setText("BPM: 120", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe94560));
    bpmLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    addAndMakeVisible(bpmLabel);
}

void TimelineComponent::initializeTrackControls()
{
    for (int i = 0; i < TimelineData::numTracks; ++i)
    {
        // Mute button
        muteButtons[i] = std::make_unique<juce::TextButton>("M");
        muteButtons[i]->setClickingTogglesState(true);
        muteButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff533483));
        muteButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        muteButtons[i]->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffe94560));
        muteButtons[i]->onClick = [this, i] {
            bool muted = muteButtons[i]->getToggleState();
            timelineData.getTrack(i).muted.store(muted);
            repaint();
        };
        addAndMakeVisible(*muteButtons[i]);

        // Solo button
        soloButtons[i] = std::make_unique<juce::TextButton>("S");
        soloButtons[i]->setClickingTogglesState(true);
        soloButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff533483));
        soloButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        soloButtons[i]->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xfffeca57));
        soloButtons[i]->onClick = [this, i] {
            bool soloed = soloButtons[i]->getToggleState();
            timelineData.getTrack(i).soloed.store(soloed);
            repaint();
        };
        addAndMakeVisible(*soloButtons[i]);

        // Arm button
        armButtons[i] = std::make_unique<juce::TextButton>("R");
        armButtons[i]->setClickingTogglesState(true);
        armButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff533483));
        armButtons[i]->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        armButtons[i]->setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
        armButtons[i]->onClick = [this, i] {
            bool armed = armButtons[i]->getToggleState();
            timelineData.getTrack(i).armed.store(armed);
            repaint();
        };
        addAndMakeVisible(*armButtons[i]);
    }
}

void TimelineComponent::updateTrackControlButtonStates(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= TimelineData::numTracks)
        return;

    auto& track = timelineData.getTrack(trackIndex);

    if (muteButtons[trackIndex])
        muteButtons[trackIndex]->setToggleState(track.muted.load(), juce::dontSendNotification);
    if (soloButtons[trackIndex])
        soloButtons[trackIndex]->setToggleState(track.soloed.load(), juce::dontSendNotification);
    if (armButtons[trackIndex])
        armButtons[trackIndex]->setToggleState(track.armed.load(), juce::dontSendNotification);
}

void TimelineComponent::updateToolbarState()
{
    // Update record button
    if (recordingManager.isRecording())
    {
        recordButton.setButtonText("Stop");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 50, 50));
    }
    else
    {
        recordButton.setButtonText("Rec");
        recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(100, 30, 30));
    }

    // Update loop button
    if (timelineData.loopEnabled.load())
    {
        loopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 150, 50));
    }
    else
    {
        loopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 80, 50));
    }

    // Update BPM label
    bpmLabel.setText("BPM: " + juce::String(static_cast<int>(timelineData.getBPM())), juce::dontSendNotification);

    // Update zoom slider
    zoomSlider.setValue(horizontalZoom, juce::dontSendNotification);
}

void TimelineComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Toolbar background
    auto toolbarArea = getToolbarArea();
    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(toolbarArea);

    // Draw track controls background
    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(juce::Rectangle<int>(0, toolbarHeight, trackControlsWidth, getHeight() - toolbarHeight));

    // Draw time ruler background
    g.setColour(juce::Colour(0xff0f3460));
    g.fillRect(juce::Rectangle<int>(trackControlsWidth, toolbarHeight,
                                     getWidth() - trackControlsWidth, timeRulerHeight));

    // Draw time ruler
    drawTimeRuler(g);

    // Draw tracks and clips
    for (int i = 0; i < timelineData.getNumTracks(); ++i)
    {
        drawTrack(g, i);
        drawTrackControls(g, i);
    }

    // Draw playhead
    drawPlayhead(g);

    // Draw recording indicator
    if (recordingManager.isRecording())
    {
        g.setColour(juce::Colours::red.withAlpha(0.1f));
        g.fillRect(getTimelineArea());

        // Recording text in toolbar
        g.setColour(juce::Colours::red);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("REC", getWidth() - 60, toolbarHeight + 5, 50, 20, juce::Justification::right);
    }

    // Update toolbar state
    updateToolbarState();

    // Draw drag overlay when files are being dragged
    if (isDraggingFiles && dragOverTrackIndex >= 0)
    {
        auto trackBounds = getTrackBounds(dragOverTrackIndex);
        g.setColour(juce::Colour(0xff00b894).withAlpha(0.3f));
        g.fillRect(trackBounds);

        g.setColour(juce::Colour(0xff00b894));
        g.drawRect(trackBounds, 2);

        // Draw drop indicator
        int dropX = beatToX(dragOverBeat);
        g.setColour(juce::Colours::white);
        g.drawVerticalLine(dropX, static_cast<float>(trackBounds.getY()),
                          static_cast<float>(trackBounds.getBottom()));

        // Drop text
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("Drop to import", trackBounds, juce::Justification::centred);
    }
}

void TimelineComponent::resized()
{
    // Layout toolbar
    auto toolbar = getToolbarArea();
    int x = 10;
    int y = toolbar.getY() + 5;
    int btnWidth = 70;
    int btnHeight = 28;
    int gap = 10;

    recordButton.setBounds(x, y, btnWidth, btnHeight);
    x += btnWidth + gap;

    loopButton.setBounds(x, y, btnWidth, btnHeight);
    x += btnWidth + gap;

    deleteButton.setBounds(x, y, btnWidth, btnHeight);
    x += btnWidth + gap;

    importSampleButton.setBounds(x, y, btnWidth, btnHeight);
    x += btnWidth + gap + 20;

    zoomLabel.setBounds(x, y + 5, 45, 20);
    x += 45;

    zoomSlider.setBounds(x, y, 120, btnHeight);
    x += 130;

    // BPM on the right
    bpmLabel.setBounds(getWidth() - 100, y + 5, 90, 20);

    // Layout track control buttons
    int controlY = toolbarHeight + timeRulerHeight;
    for (int i = 0; i < TimelineData::numTracks; ++i)
    {
        int trackY = trackToY(i);
        int btnSize = 18;
        int btnY = trackY + 24;
        int spacing = 20;

        if (muteButtons[i])
            muteButtons[i]->setBounds(5, btnY, btnSize, btnSize);
        if (soloButtons[i])
            soloButtons[i]->setBounds(5 + spacing, btnY, btnSize, btnSize);
        if (armButtons[i])
            armButtons[i]->setBounds(5 + spacing * 2, btnY, btnSize, btnSize);

        // Update button states
        updateTrackControlButtonStates(i);
    }
}

void TimelineComponent::timerCallback()
{
    repaint(); // Update playhead position
}

void TimelineComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    repaint();
}

void TimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    auto timelineArea = getTimelineArea();

    // Skip if clicking in track controls area (buttons handle their own clicks)
    if (pos.x < trackControlsWidth && pos.y > toolbarHeight)
    {
        // Check if it's a right-click in track controls - show context menu
        if (e.mods.isRightButtonDown())
        {
            int trackIndex = yToTrack(pos.y);
            if (trackIndex >= 0 && trackIndex < timelineData.getNumTracks())
            {
                showTrackContextMenu(trackIndex, pos);
            }
        }
        return;
    }

    if (!timelineArea.contains(pos))
        return;

    int trackIndex = yToTrack(pos.y);
    double beat = xToBeat(pos.x);

    if (trackIndex < 0 || trackIndex >= timelineData.getNumTracks())
        return;

    // Right-click context menu
    if (e.mods.isRightButtonDown())
    {
        showTrackContextMenu(trackIndex, pos);
        return;
    }

    // Check if clicking on existing clip
    TimelineClip* clickedClip = timelineData.findClipAt(trackIndex, beat);

    if (clickedClip)
    {
        selectClip(clickedClip->getId());

        // Determine drag mode based on click position
        auto clipBounds = getClipBounds(*clickedClip, trackIndex);
        int margin = 10;

        if (pos.x < clipBounds.getX() + margin)
        {
            currentDragMode = DragMode::ResizeClipLeft;
        }
        else if (pos.x > clipBounds.getRight() - margin)
        {
            currentDragMode = DragMode::ResizeClipRight;
        }
        else
        {
            currentDragMode = DragMode::MoveClip;
        }

        draggedClipId = clickedClip->getId();
        dragStartBeat = beat;
        dragOriginalStartBeat = clickedClip->startBeat.load();
        dragOriginalLengthBeats = clickedClip->lengthBeats.load();
        dragStartMouse = pos;
    }
    else
    {
        clearSelection();

        // Create new clip on double-click
        if (e.mods.isLeftButtonDown() && e.getNumberOfClicks() == 2)
        {
            // Create a new MIDI clip at click position
            auto newClip = std::make_unique<TimelineClip>(TimelineClip::Type::Midi);
            newClip->startBeat.store(std::floor(beat));
            newClip->lengthBeats.store(4.0);
            newClip->name = "New Clip";

            timelineData.getTrack(trackIndex).addClip(std::move(newClip));

            if (timelineData.onStructureChanged)
                timelineData.onStructureChanged();

            repaint();
        }
    }
}

void TimelineComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (currentDragMode == DragMode::None)
        return;

    auto pos = e.getPosition();
    double currentBeat = xToBeat(pos.x);

    TimelineClip* clip = timelineData.getTrack(selectedTrackIndex).findClip(draggedClipId);
    if (!clip)
        return;

    double beatDelta = currentBeat - dragStartBeat;

    switch (currentDragMode)
    {
        case DragMode::MoveClip:
            clip->startBeat.store(std::max(0.0, dragOriginalStartBeat + beatDelta));
            break;

        case DragMode::ResizeClipRight:
            clip->lengthBeats.store(std::max(0.5, dragOriginalLengthBeats + beatDelta));
            break;

        case DragMode::ResizeClipLeft:
            {
                double newStart = dragOriginalStartBeat + beatDelta;
                double newLength = dragOriginalLengthBeats - beatDelta;
                if (newStart >= 0 && newLength >= 0.5)
                {
                    clip->startBeat.store(newStart);
                    clip->lengthBeats.store(newLength);
                }
            }
            break;

        default:
            break;
    }

    repaint();
}

void TimelineComponent::mouseUp(const juce::MouseEvent& e)
{
    if (currentDragMode != DragMode::None)
    {
        if (timelineData.onStructureChanged)
            timelineData.onStructureChanged();
    }

    currentDragMode = DragMode::None;
    draggedClipId = juce::Uuid();
}

void TimelineComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    // Ctrl + Scroll = Horizontal Zoom
    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        double zoomFactor = 1.15;
        double oldZoom = horizontalZoom;

        if (wheel.deltaY > 0)
        {
            horizontalZoom = juce::jmin(200.0, horizontalZoom * zoomFactor);
        }
        else if (wheel.deltaY < 0)
        {
            horizontalZoom = juce::jmax(5.0, horizontalZoom / zoomFactor);
        }

        // Zoom centered on mouse position
        double mouseBeat = xToBeat(e.x);
        double newMouseBeat = scrollBeat + (e.x - trackControlsWidth) / horizontalZoom;
        scrollBeat += (mouseBeat - newMouseBeat);
        scrollBeat = std::max(0.0, scrollBeat);

        repaint();
    }
    // Shift + Scroll = Scroll horizontally
    else if (e.mods.isShiftDown())
    {
        double scrollAmount = wheel.deltaY * 100.0 / horizontalZoom;
        scrollBeat = std::max(0.0, scrollBeat - scrollAmount);
        repaint();
    }
    // Plain scroll = Scroll horizontally (or vertically with trackpad)
    else
    {
        // Horizontal scroll
        if (std::abs(wheel.deltaX) > 0.001)
        {
            double scrollAmount = wheel.deltaX * 50.0 / horizontalZoom;
            scrollBeat = std::max(0.0, scrollBeat - scrollAmount);
        }
        // Vertical scroll - scroll horizontally too (common for trackpads)
        else if (std::abs(wheel.deltaY) > 0.001)
        {
            double scrollAmount = wheel.deltaY * 50.0 / horizontalZoom;
            scrollBeat = std::max(0.0, scrollBeat - scrollAmount);
        }
        repaint();
    }
}

void TimelineComponent::setHorizontalZoom(double zoom)
{
    horizontalZoom = juce::jlimit(5.0, 200.0, zoom);
    repaint();
}

void TimelineComponent::setVerticalZoom(double zoom)
{
    verticalZoom = juce::jlimit(0.5, 2.0, zoom);
    repaint();
}

void TimelineComponent::scrollToBeat(double beat)
{
    scrollBeat = std::max(0.0, beat);
    repaint();
}

void TimelineComponent::scrollToTrack(int trackIndex)
{
    scrollTrack = juce::jlimit(0, timelineData.getNumTracks() - 1, trackIndex);
    repaint();
}

void TimelineComponent::selectClip(const juce::Uuid& clipId)
{
    selectedClipId = clipId;

    // Find which track contains this clip
    for (int i = 0; i < timelineData.getNumTracks(); ++i)
    {
        if (timelineData.getTrack(i).findClip(clipId))
        {
            selectedTrackIndex = i;
            break;
        }
    }

    repaint();
}

void TimelineComponent::clearSelection()
{
    selectedClipId = juce::Uuid();
    selectedTrackIndex = -1;
    repaint();
}

void TimelineComponent::deleteSelectedClip()
{
    if (selectedClipId.isNull())
        return;

    if (selectedTrackIndex >= 0 && selectedTrackIndex < timelineData.getNumTracks())
    {
        timelineData.getTrack(selectedTrackIndex).removeClip(selectedClipId);
        clearSelection();

        if (timelineData.onStructureChanged)
            timelineData.onStructureChanged();

        repaint();
    }
}

void TimelineComponent::copySelectedClip()
{
    // TODO: Implement clipboard
}

void TimelineComponent::pasteClip()
{
    // TODO: Implement clipboard paste
}

void TimelineComponent::armTrack(int trackIndex, bool armed)
{
    if (trackIndex >= 0 && trackIndex < timelineData.getNumTracks())
    {
        timelineData.getTrack(trackIndex).armed.store(armed);
        repaint();
    }
}

void TimelineComponent::startRecordingOnArmedTrack()
{
    // Find first armed track
    for (int i = 0; i < timelineData.getNumTracks(); ++i)
    {
        if (timelineData.getTrack(i).armed.load())
        {
            recordingManager.startRecording(i, RecordingManager::RecordingSource::AudioInput);
            break;
        }
    }
    repaint();
}

void TimelineComponent::stopRecording()
{
    auto recordedClip = recordingManager.stopRecording();

    if (recordedClip)
    {
        int trackIndex = recordingManager.getRecordingTrackIndex();
        if (trackIndex >= 0 && trackIndex < timelineData.getNumTracks())
        {
            timelineData.getTrack(trackIndex).addClip(std::move(recordedClip));

            if (timelineData.onStructureChanged)
                timelineData.onStructureChanged();
        }
    }

    repaint();
}

int TimelineComponent::beatToX(double beat) const
{
    return trackControlsWidth + static_cast<int>((beat - scrollBeat) * horizontalZoom);
}

double TimelineComponent::xToBeat(int x) const
{
    return scrollBeat + static_cast<double>(x - trackControlsWidth) / horizontalZoom;
}

int TimelineComponent::trackToY(int trackIndex) const
{
    return toolbarHeight + timeRulerHeight + (trackIndex - scrollTrack) * static_cast<int>(trackHeight * verticalZoom);
}

int TimelineComponent::yToTrack(int y) const
{
    return scrollTrack + (y - toolbarHeight - timeRulerHeight) / static_cast<int>(trackHeight * verticalZoom);
}

juce::Rectangle<int> TimelineComponent::getClipBounds(const TimelineClip& clip, int trackIndex) const
{
    int x = beatToX(clip.startBeat.load());
    int y = trackToY(trackIndex);
    int width = static_cast<int>(clip.lengthBeats.load() * horizontalZoom);
    int height = static_cast<int>(trackHeight * verticalZoom);

    return { x, y, width, height };
}

juce::Rectangle<int> TimelineComponent::getTrackBounds(int trackIndex) const
{
    int y = trackToY(trackIndex);
    return { trackControlsWidth, y, getWidth() - trackControlsWidth,
             static_cast<int>(trackHeight * verticalZoom) };
}

juce::Rectangle<int> TimelineComponent::getTimelineArea() const
{
    return { trackControlsWidth, toolbarHeight + timeRulerHeight,
             getWidth() - trackControlsWidth, getHeight() - toolbarHeight - timeRulerHeight };
}

juce::Rectangle<int> TimelineComponent::getToolbarArea() const
{
    return { 0, 0, getWidth(), toolbarHeight };
}

void TimelineComponent::drawTimeRuler(juce::Graphics& g)
{
    auto rulerBounds = juce::Rectangle<int>(trackControlsWidth, toolbarHeight,
                                             getWidth() - trackControlsWidth, timeRulerHeight);

    g.setColour(juce::Colour(0xff0f3460));
    g.fillRect(rulerBounds);

    int numerator = timelineData.timeSigNumerator.load();
    double startBeat = std::floor(scrollBeat);
    double endBeat = scrollBeat + (getWidth() - trackControlsWidth) / horizontalZoom;

    // Adaptive subdivision based on zoom level
    int beatStep = 1;
    bool showBeatNumbers = horizontalZoom >= 15.0;
    bool showSubBeats = horizontalZoom >= 40.0;

    if (horizontalZoom < 10.0)
        beatStep = 4;  // Only show every 4th beat
    else if (horizontalZoom < 20.0)
        beatStep = 2;  // Show every 2nd beat

    for (double beat = startBeat; beat <= endBeat; beat += beatStep)
    {
        int x = beatToX(beat);
        bool isBar = (static_cast<int>(beat) % numerator) == 0;
        int rulerBottom = toolbarHeight + timeRulerHeight;

        if (isBar)
        {
            // Bar marker (larger)
            g.setColour(juce::Colour(0xffe94560));
            g.drawLine(static_cast<float>(x), static_cast<float>(rulerBottom - 18),
                       static_cast<float>(x), static_cast<float>(rulerBottom), 2.0f);

            // Bar number
            g.setColour(juce::Colours::white);
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            int barNumber = static_cast<int>(beat) / numerator + 1;
            g.drawText(juce::String(barNumber), x + 4, toolbarHeight + 2, 40, 14, juce::Justification::left);
        }
        else
        {
            // Beat marker (smaller)
            g.setColour(juce::Colour(0xff533483));
            g.drawLine(static_cast<float>(x), static_cast<float>(rulerBottom - 10),
                       static_cast<float>(x), static_cast<float>(rulerBottom), 1.0f);

            // Show beat numbers when zoomed in enough
            if (showBeatNumbers)
            {
                g.setColour(juce::Colour(0xff888888));
                g.setFont(juce::FontOptions(9.0f));
                int beatInBar = (static_cast<int>(beat) % numerator) + 1;
                g.drawText(juce::String(beatInBar), x + 2, toolbarHeight + 12, 20, 10, juce::Justification::left);
            }
        }
    }

    // Draw sub-beat divisions when very zoomed in
    if (showSubBeats)
    {
        g.setColour(juce::Colour(0xff333355));
        double subBeatStep = 0.5; // 8th notes
        int rulerBottom = toolbarHeight + timeRulerHeight;

        for (double beat = startBeat; beat <= endBeat; beat += subBeatStep)
        {
            // Skip beats (they're drawn above)
            if (std::fmod(beat, 1.0) < 0.01) continue;

            int x = beatToX(beat);
            g.drawLine(static_cast<float>(x), static_cast<float>(rulerBottom - 5),
                       static_cast<float>(x), static_cast<float>(rulerBottom), 0.5f);
        }
    }
}

void TimelineComponent::drawTrack(juce::Graphics& g, int trackIndex)
{
    auto trackBounds = getTrackBounds(trackIndex);

    // Track background
    g.setColour(getTrackColor(trackIndex).withAlpha(0.3f));
    g.fillRect(trackBounds);

    // Track border
    g.setColour(juce::Colour(0xff533483));
    g.drawRect(trackBounds, 1);

    // Draw clips
    auto& track = timelineData.getTrack(trackIndex);
    track.forEachClip([this, &g, trackIndex](TimelineClip& clip)
    {
        bool isSelected = (clip.getId() == selectedClipId);
        drawClip(g, clip, trackIndex, isSelected);
    });
}

void TimelineComponent::drawClip(juce::Graphics& g, const TimelineClip& clip,
                                  int trackIndex, bool isSelected)
{
    auto bounds = getClipBounds(clip, trackIndex);

    // Clip background
    juce::Colour clipColor = clip.getType() == TimelineClip::Type::Audio
        ? juce::Colour(0xff00b894)
        : juce::Colour(0xff6c5ce7);

    if (clip.muted.load())
        clipColor = clipColor.withAlpha(0.4f);

    g.setColour(clipColor);
    g.fillRoundedRectangle(bounds.toFloat().reduced(2), 4.0f);

    // Selection highlight
    if (isSelected)
    {
        g.setColour(getSelectionColor());
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 4.0f, 2.0f);
    }

    // Clip content preview
    auto contentBounds = bounds.reduced(4);
    if (clip.getType() == TimelineClip::Type::Audio && clip.audioBuffer)
    {
        drawWaveformPreview(g, clip, contentBounds);
    }
    else if (clip.getType() == TimelineClip::Type::Midi)
    {
        drawMidiPreview(g, clip, contentBounds);
    }

    // Clip name
    g.setColour(juce::Colours::white);
    g.setFont(11.0f);
    g.drawText(clip.name, contentBounds.removeFromTop(14), juce::Justification::left);
}

void TimelineComponent::drawWaveformPreview(juce::Graphics& g, const TimelineClip& clip,
                                             const juce::Rectangle<int>& bounds)
{
    if (!clip.audioBuffer)
        return;

    auto* buffer = clip.audioBuffer.get();
    int numSamples = buffer->getNumSamples();
    int width = bounds.getWidth();
    int height = bounds.getHeight() - 16; // Leave space for name
    int centerY = bounds.getY() + 16 + height / 2;

    g.setColour(juce::Colours::white.withAlpha(0.6f));

    float samplesPerPixel = static_cast<float>(numSamples) / width;

    for (int x = 0; x < width; ++x)
    {
        int startSample = static_cast<int>(x * samplesPerPixel);
        int endSample = static_cast<int>((x + 1) * samplesPerPixel);
        endSample = std::min(endSample, numSamples - 1);

        float maxVal = 0.0f;
        for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
        {
            for (int s = startSample; s <= endSample; ++s)
            {
                float sample = std::abs(buffer->getSample(ch, s));
                maxVal = std::max(maxVal, sample);
            }
        }

        int lineHeight = static_cast<int>(maxVal * height * 0.8f);
        g.drawVerticalLine(bounds.getX() + x, static_cast<float>(centerY - lineHeight / 2),
                          static_cast<float>(centerY + lineHeight / 2));
    }
}

void TimelineComponent::drawMidiPreview(juce::Graphics& g, const TimelineClip& clip,
                                         const juce::Rectangle<int>& bounds)
{
    const auto& notes = clip.getMidiNotes();
    if (notes.empty())
        return;

    int contentHeight = bounds.getHeight() - 16;
    int contentY = bounds.getY() + 16;

    double clipLength = clip.lengthBeats.load();
    double pixelsPerBeat = bounds.getWidth() / clipLength;

    // Note range
    int minNote = 127, maxNote = 0;
    for (const auto& note : notes)
    {
        minNote = std::min(minNote, note.noteNumber);
        maxNote = std::max(maxNote, note.noteNumber);
    }
    if (minNote > maxNote)
    {
        minNote = 60;
        maxNote = 72;
    }
    int noteRange = std::max(maxNote - minNote + 1, 12);
    float noteHeight = static_cast<float>(contentHeight) / noteRange;

    g.setColour(juce::Colour(0xffffeaa7));

    for (const auto& note : notes)
    {
        double noteStartRatio = note.startBeat / clipLength;
        double noteLengthRatio = note.lengthBeats / clipLength;

        int x = bounds.getX() + static_cast<int>(noteStartRatio * bounds.getWidth());
        int y = contentY + static_cast<int>((maxNote - note.noteNumber) * noteHeight);
        int width = std::max(2, static_cast<int>(noteLengthRatio * bounds.getWidth()));
        int height = static_cast<int>(noteHeight) - 1;

        g.fillRect(x, y, width, height);
    }
}

void TimelineComponent::drawPlayhead(juce::Graphics& g)
{
    double playheadBeat = timelineData.playheadBeat.load();
    int x = beatToX(playheadBeat);

    if (x >= trackControlsWidth && x < getWidth())
    {
        g.setColour(getPlayheadColor());
        g.drawLine(static_cast<float>(x), static_cast<float>(toolbarHeight + timeRulerHeight),
                   static_cast<float>(x), static_cast<float>(getHeight()), 2.0f);

        // Playhead triangle
        juce::Path triangle;
        triangle.addTriangle(static_cast<float>(x - 6), static_cast<float>(toolbarHeight),
                            static_cast<float>(x + 6), static_cast<float>(toolbarHeight),
                            static_cast<float>(x), static_cast<float>(toolbarHeight + 10));
        g.fillPath(triangle);
    }
}

void TimelineComponent::drawTrackControls(juce::Graphics& g, int trackIndex)
{
    auto& track = timelineData.getTrack(trackIndex);
    int y = trackToY(trackIndex);
    int height = static_cast<int>(trackHeight * verticalZoom);

    auto controlBounds = juce::Rectangle<int>(0, y, trackControlsWidth, height);

    // Background
    g.setColour(juce::Colour(0xff16213e));
    g.fillRect(controlBounds);

    // Track number/name
    g.setColour(getTrackColor(trackIndex));
    g.setFont(12.0f);
    g.drawText(track.name, controlBounds.removeFromTop(20).withTrimmedLeft(5),
               juce::Justification::left);

    // Volume slider hint (below the buttons)
    g.setColour(juce::Colour(0xff533483));
    int sliderY = y + 48;
    int sliderWidth = trackControlsWidth - 20;
    g.fillRect(10, sliderY, sliderWidth, 4);

    float vol = track.volume.load();
    g.setColour(getTrackColor(trackIndex));
    g.fillRect(10, sliderY, static_cast<int>(vol * sliderWidth), 4);

    // Volume label
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(9.0f);
    g.drawText(juce::String(static_cast<int>(vol * 100)) + "%",
               juce::Rectangle<int>(10, sliderY + 6, sliderWidth, 12),
               juce::Justification::centred);
}

juce::Colour TimelineComponent::getTrackColor(int trackIndex) const
{
    static const juce::Colour colors[] = {
        juce::Colour(0xffe94560), // Pink
        juce::Colour(0xff00b894), // Green
        juce::Colour(0xff6c5ce7), // Purple
        juce::Colour(0xff00cec9), // Cyan
        juce::Colour(0xfffeca57), // Yellow
        juce::Colour(0xffff7675), // Red
        juce::Colour(0xff74b9ff), // Blue
        juce::Colour(0xffa29bfe)  // Light purple
    };

    return colors[trackIndex % 8];
}

void TimelineComponent::toggleRecording()
{
    if (recordingManager.isRecording())
    {
        stopRecording();
    }
    else
    {
        startRecordingOnArmedTrack();
    }
}

void TimelineComponent::toggleLoop()
{
    bool loopEnabled = !timelineData.loopEnabled.load();
    timelineData.loopEnabled.store(loopEnabled);
    repaint();
}

// === FileDragAndDropTarget Implementation ===

bool TimelineComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        juce::File f(file);
        juce::String ext = f.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".aif" ||
            ext == ".mp3" || ext == ".flac" || ext == ".ogg")
            return true;
    }
    return false;
}

void TimelineComponent::fileDragEnter(const juce::StringArray& files, int x, int y)
{
    isDraggingFiles = true;
    dragOverTrackIndex = yToTrack(y);
    dragOverBeat = xToBeat(x);
    repaint();
}

void TimelineComponent::fileDragExit(const juce::StringArray& files)
{
    isDraggingFiles = false;
    dragOverTrackIndex = -1;
    repaint();
}

void TimelineComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    isDraggingFiles = false;

    int trackIndex = yToTrack(y);
    double beat = xToBeat(x);

    if (trackIndex < 0 || trackIndex >= timelineData.getNumTracks())
        trackIndex = 0;

    for (const auto& filePath : files)
    {
        juce::File file(filePath);
        loadSampleIntoTimeline(file, trackIndex, beat);
        beat += 4.0; // Stagger multiple files
    }

    repaint();
}

// === Sample Loading ===

void TimelineComponent::loadSampleIntoTimeline(const juce::File& sampleFile, int trackIndex, double startBeat)
{
    if (trackIndex < 0 || trackIndex >= timelineData.getNumTracks())
        trackIndex = 0;

    // Create reader for the audio file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sampleFile));

    if (!reader)
    {
        DBG("Failed to load sample: " + sampleFile.getFullPathName());
        return;
    }

    // Read the entire file into a buffer
    auto buffer = std::make_shared<juce::AudioBuffer<float>>(static_cast<int>(reader->numChannels),
                                                              static_cast<int>(reader->lengthInSamples));
    reader->read(buffer.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    // Calculate length in beats
    double sampleRate = reader->sampleRate;
    double lengthSeconds = reader->lengthInSamples / sampleRate;
    double bpm = timelineData.getBPM();
    double beatsPerSecond = bpm / 60.0;
    double lengthBeats = lengthSeconds * beatsPerSecond;

    // Create audio clip
    auto clip = std::make_unique<TimelineClip>(TimelineClip::Type::Audio);
    clip->startBeat.store(std::floor(startBeat));
    clip->lengthBeats.store(lengthBeats);
    clip->name = sampleFile.getFileNameWithoutExtension();
    clip->setAudioBuffer(buffer, sampleRate);

    // Add to track
    timelineData.getTrack(trackIndex).addClip(std::move(clip));

    if (timelineData.onStructureChanged)
        timelineData.onStructureChanged();

    repaint();
}

// === Context Menu ===

void TimelineComponent::showTrackContextMenu(int trackIndex, const juce::Point<int>& position)
{
    juce::PopupMenu menu;

    menu.addItem("Import Sample...", [this, trackIndex] {
        juce::FileChooser chooser("Import Audio Sample",
                                   juce::File::getSpecialLocation(juce::File::userMusicDirectory),
                                   "*.wav;*.aiff;*.mp3;*.flac;*.ogg", true);
        chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, trackIndex](const juce::FileChooser& fc) {
                if (fc.getResults().size() > 0)
                {
                    const auto& file = fc.getResult();
                    double startBeat = timelineData.playheadBeat.load();
                    loadSampleIntoTimeline(file, trackIndex, startBeat);
                }
            });
    });

    menu.addItem("Create MIDI Clip", [this, trackIndex] {
        double startBeat = timelineData.playheadBeat.load();
        auto clip = std::make_unique<TimelineClip>(TimelineClip::Type::Midi);
        clip->startBeat.store(std::floor(startBeat));
        clip->lengthBeats.store(4.0);
        clip->name = "MIDI Clip";

        timelineData.getTrack(trackIndex).addClip(std::move(clip));

        if (timelineData.onStructureChanged)
            timelineData.onStructureChanged();

        repaint();
    });

    menu.addSeparator();

    // Track state options
    auto& track = timelineData.getTrack(trackIndex);

    menu.addItem("Mute", true, track.muted.load(), [this, trackIndex] {
        bool currentState = timelineData.getTrack(trackIndex).muted.load();
        timelineData.getTrack(trackIndex).muted.store(!currentState);
        updateTrackControlButtonStates(trackIndex);
        repaint();
    });

    menu.addItem("Solo", true, track.soloed.load(), [this, trackIndex] {
        bool currentState = timelineData.getTrack(trackIndex).soloed.load();
        timelineData.getTrack(trackIndex).soloed.store(!currentState);
        updateTrackControlButtonStates(trackIndex);
        repaint();
    });

    menu.addItem("Arm for Recording", true, track.armed.load(), [this, trackIndex] {
        bool currentState = timelineData.getTrack(trackIndex).armed.load();
        timelineData.getTrack(trackIndex).armed.store(!currentState);
        updateTrackControlButtonStates(trackIndex);
        repaint();
    });

    menu.showMenuAsync(juce::PopupMenu::Options());
}
