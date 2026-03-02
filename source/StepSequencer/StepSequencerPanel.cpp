// ============================================================================
// StepSequencerPanel.cpp - Implementierung
// ============================================================================

#include "StepSequencerPanel.h"
#include "../TrackManager.h"
#include "../TrackComponent.h"
#include "../Timeline/PatternToClipConverter.h"
#include "../Timeline/TimelineData.h"
#include "../Timeline/TimelineTrack.h"

StepSequencerPanel::StepSequencerPanel()
    : DockablePanel(PanelType::StepSequencer, "Step Sequencer")
    , clipConverter(std::make_unique<PatternToClipConverter>())
{
    createStepGrid();

    // === Push to Timeline Button ===
    pushToTimelineButton.setButtonText("Push to Timeline");
    pushToTimelineButton.setColour(juce::TextButton::buttonColourId, juce::Colour(30, 30, 45));
    pushToTimelineButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0, 255, 255));
    pushToTimelineButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0, 255, 255));
    pushToTimelineButton.onClick = [this]() {
        pushCurrentTrackToTimeline();
    };
    addAndMakeVisible(pushToTimelineButton);

    // === Push All Button ===
    pushAllButton.setButtonText("Push All");
    pushAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(30, 30, 45));
    pushAllButton.setColour(juce::TextButton::textColourOnId, juce::Colour(180, 0, 255));
    pushAllButton.setColour(juce::TextButton::textColourOffId, juce::Colour(180, 0, 255));
    pushAllButton.onClick = [this]() {
        pushAllTracksToTimeline();
    };
    addAndMakeVisible(pushAllButton);

    // === Status Label ===
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(150, 150, 170));
    statusLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(statusLabel);
}

StepSequencerPanel::~StepSequencerPanel() = default;

void StepSequencerPanel::setNumTracks(int num)
{
    if (numTracks != num && num > 0 && num <= 16)
    {
        numTracks = num;
        createStepGrid();
        resized();
    }
}

void StepSequencerPanel::setNumSteps(int num)
{
    if (numSteps != num && num > 0 && num <= 64)
    {
        numSteps = num;
        createStepGrid();
        resized();
    }
}

void StepSequencerPanel::setTrackManager(TrackManager* manager)
{
    trackManager = manager;
}

void StepSequencerPanel::setCurrentTrack(int trackIndex)
{
    if (trackIndex >= 0 && trackIndex < numTracks)
    {
        currentTrack = trackIndex;
    }
}

bool StepSequencerPanel::isStepActive(int track, int step) const
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        if (stepButtons[track][step])
            return stepButtons[track][step]->isActive();
    }
    return false;
}

void StepSequencerPanel::setStepActive(int track, int step, bool active)
{
    if (track >= 0 && track < numTracks && step >= 0 && step < numSteps)
    {
        if (stepButtons[track][step])
            stepButtons[track][step]->setActive(active, true);
    }
}

void StepSequencerPanel::clearTrack(int track)
{
    if (track >= 0 && track < numTracks)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            if (stepButtons[track][step])
                stepButtons[track][step]->setActive(false, false);
        }
    }
}

void StepSequencerPanel::clearAll()
{
    for (int track = 0; track < numTracks; ++track)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            if (stepButtons[track][step])
                stepButtons[track][step]->setActive(false, false);
        }
    }
}

void StepSequencerPanel::pushCurrentTrackToTimeline()
{
    if (!trackManager || !timelineData || !clipConverter)
    {
        statusLabel.setText("Error: No Timeline connection", juce::dontSendNotification);
        return;
    }

    // Get the TrackComponent for the current track
    auto& trackComponent = trackManager->getTrack(currentTrack);

    // Get the TrackModel and TrackAudioProcessor
    const auto& model = trackComponent.getModel();
    auto& processor = trackComponent.getAudioProcessor();

    // Convert pattern to clip
    auto clip = clipConverter->convertToClip(
        model,
        processor,
        playheadBeat,
        currentBPM,
        sampleRate
    );

    if (clip)
    {
        // Add clip to timeline
        timelineData->getTrack(currentTrack).addClip(std::move(clip));

        statusLabel.setText("Pushed Track " + juce::String(currentTrack + 1) + " to Timeline",
                           juce::dontSendNotification);

        if (onPatternChanged)
            onPatternChanged();
    }
    else
    {
        statusLabel.setText("Error: Failed to create clip", juce::dontSendNotification);
    }
}

void StepSequencerPanel::pushAllTracksToTimeline()
{
    if (!trackManager || !timelineData || !clipConverter)
    {
        statusLabel.setText("Error: No Timeline connection", juce::dontSendNotification);
        return;
    }

    int successCount = 0;

    for (int track = 0; track < TrackManager::numTracks; ++track)
    {
        auto& trackComponent = trackManager->getTrack(track);

        // Get the TrackModel and TrackAudioProcessor
        const auto& model = trackComponent.getModel();
        auto& processor = trackComponent.getAudioProcessor();

        // Convert pattern to clip
        auto clip = clipConverter->convertToClip(
            model,
            processor,
            playheadBeat,
            currentBPM,
            sampleRate
        );

        if (clip)
        {
            timelineData->getTrack(track).addClip(std::move(clip));
            successCount++;
        }
    }

    statusLabel.setText("Pushed " + juce::String(successCount) + " tracks to Timeline",
                       juce::dontSendNotification);

    if (onPatternChanged)
        onPatternChanged();
}

void StepSequencerPanel::createStepGrid()
{
    // Alte Buttons entfernen
    stepButtons.clear();
    trackLabels.clear();
    removeAllChildren();

    // Neue Struktur erstellen
    stepButtons.resize(numTracks);
    trackLabels.resize(numTracks);

    // Track-spezifische Farben (Neon Sakura Palette)
    juce::Colour trackColours[] = {
        juce::Colour(255, 20, 147),   // Neon Pink
        juce::Colour(0, 255, 255),    // Neon Cyan
        juce::Colour(180, 0, 255),    // Neon Purple
        juce::Colour(255, 165, 0),    // Neon Orange
        juce::Colour(0, 255, 127),    // Neon Green
        juce::Colour(255, 255, 0),    // Neon Yellow
        juce::Colour(255, 0, 127),    // Neon Rose
        juce::Colour(127, 255, 255),  // Neon Aqua
    };

    for (int track = 0; track < numTracks; ++track)
    {
        stepButtons[track].resize(numSteps);

        // Track Label
        trackLabels[track] = std::make_unique<juce::Label>();
        trackLabels[track]->setText("Track " + juce::String(track + 1), juce::dontSendNotification);
        trackLabels[track]->setColour(juce::Label::textColourId, trackColours[track % 8]);
        trackLabels[track]->setFont(juce::Font(12.0f, juce::Font::bold));
        addAndMakeVisible(*trackLabels[track]);

        // Step Buttons
        for (int step = 0; step < numSteps; ++step)
        {
            auto& button = stepButtons[track][step];
            button = std::make_unique<NeonStepButton>();

            // Farbe pro Track
            button->setActiveColour(trackColours[track % 8]);
            button->setGlowColour(trackColours[track % 8]);

            // Callback für State-Änderungen
            button->onStateChanged = [this, track, step](bool active)
            {
                if (onStepChanged)
                    onStepChanged(track, step, active);
                if (onPatternChanged)
                    onPatternChanged();
            };

            addAndMakeVisible(*button);
        }
    }

    // Re-add buttons
    addAndMakeVisible(pushToTimelineButton);
    addAndMakeVisible(pushAllButton);
    addAndMakeVisible(statusLabel);

    // Header nach vorne bringen
    if (header)
        header->toFront(false);
}

void StepSequencerPanel::resized()
{
    DockablePanel::resized();

    auto contentBounds = getContentBounds();
    if (contentBounds.isEmpty())
        contentBounds = getLocalBounds();

    // Header-Bereich abziehen
    auto headerArea = contentBounds.removeFromTop(headerHeight);

    // Footer-Bereich für Buttons
    auto footerArea = contentBounds.removeFromBottom(footerHeight);

    // Buttons im Footer platzieren
    auto footerRow = footerArea;
    pushToTimelineButton.setBounds(footerRow.removeFromLeft(120).reduced(2));
    pushAllButton.setBounds(footerRow.removeFromLeft(80).reduced(2));
    statusLabel.setBounds(footerRow.reduced(5));

    // Verfügbare Größe für Grid
    int availableWidth = contentBounds.getWidth() - trackLabelWidth;
    int availableHeight = contentBounds.getHeight();

    int stepWidth = (availableWidth - (numSteps - 1) * stepGap) / numSteps;
    int trackHeight = (availableHeight - (numTracks - 1) * trackGap) / numTracks;

    // Minimum/Maximum Limits
    stepWidth = juce::jlimit(24, 60, stepWidth);
    trackHeight = juce::jlimit(28, 50, trackHeight);

    int startX = contentBounds.getX() + trackLabelWidth + 10;
    int startY = contentBounds.getY();

    for (int track = 0; track < numTracks; ++track)
    {
        int y = startY + track * (trackHeight + trackGap);

        // Track Label
        if (trackLabels[track])
        {
            trackLabels[track]->setBounds(contentBounds.getX() + 5, y,
                                           trackLabelWidth - 10, trackHeight);
        }

        // Step Buttons
        for (int step = 0; step < numSteps; ++step)
        {
            int x = startX + step * (stepWidth + stepGap);

            if (stepButtons[track][step])
            {
                stepButtons[track][step]->setBounds(x, y, stepWidth, trackHeight);
            }
        }
    }
}

void StepSequencerPanel::paint(juce::Graphics& g)
{
    // Hintergrund
    g.fillAll(juce::Colour(15, 15, 25));

    // Content-Bereich Hintergrund
    auto contentBounds = getContentBounds();
    if (contentBounds.isEmpty())
        contentBounds = getLocalBounds();

    g.setColour(juce::Colour(20, 20, 35));
    g.fillRect(contentBounds);

    // Beat-Marker (alle 4 Steps)
    auto gridStartX = contentBounds.getX() + trackLabelWidth + 10;
    int stepWidth = (contentBounds.getWidth() - trackLabelWidth - (numSteps - 1) * stepGap) / numSteps;
    stepWidth = juce::jlimit(24, 60, stepWidth);

    g.setColour(juce::Colour(60, 60, 80));

    for (int step = 0; step < numSteps; step += 4)
    {
        if (step > 0)
        {
            int x = gridStartX + step * (stepWidth + stepGap) - stepGap / 2;
            g.drawVerticalLine(x, contentBounds.getY(), contentBounds.getBottom());
        }
    }

    // Playhead zeichnen
    if (showPlayhead && currentStep >= 0 && currentStep < numSteps)
    {
        drawPlayhead(g, currentStep);
    }

    // Header wird von DockablePanel gezeichnet
    DockablePanel::paint(g);
}

void StepSequencerPanel::drawPlayhead(juce::Graphics& g, int step)
{
    auto contentBounds = getContentBounds();
    if (contentBounds.isEmpty())
        contentBounds = getLocalBounds();

    auto gridStartX = contentBounds.getX() + trackLabelWidth + 10;
    int stepWidth = (contentBounds.getWidth() - trackLabelWidth - (numSteps - 1) * stepGap) / numSteps;
    stepWidth = juce::jlimit(24, 60, stepWidth);

    int x = gridStartX + step * (stepWidth + stepGap);
    int width = stepWidth;

    // Playhead-Highlight
    g.setColour(juce::Colour(0, 255, 255).withAlpha(0.15f));
    g.fillRect(x, contentBounds.getY(), width, contentBounds.getHeight());

    // Playhead-Linie oben
    g.setColour(juce::Colour(0, 255, 255));
    g.fillRect(x, contentBounds.getY(), width, 3);
}

void StepSequencerPanel::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        isDragging = true;
        lastDragButton = nullptr;

        // Ersten Button unter der Maus finden
        handleDragToDraw(e.getPosition());
    }
}

void StepSequencerPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        handleDragToDraw(e.getPosition());
    }
}

void StepSequencerPanel::mouseUp(const juce::MouseEvent& /*e*/)
{
    isDragging = false;
    lastDragButton = nullptr;

    // Alle Drag-Over States zurücksetzen
    for (int track = 0; track < numTracks; ++track)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            if (stepButtons[track][step])
                stepButtons[track][step]->setDragOverState(false);
        }
    }
}

void StepSequencerPanel::handleDragToDraw(const juce::Point<int>& position)
{
    auto* button = getButtonAtPosition(position);

    if (button != nullptr && button != lastDragButton)
    {
        // Erster Button bestimmt den State
        if (lastDragButton == nullptr)
        {
            dragState = !button->isActive();
        }

        // Button State setzen
        button->setActive(dragState, true);

        // Letzten Button merken
        lastDragButton = button;
    }
}

NeonStepButton* StepSequencerPanel::getButtonAtPosition(const juce::Point<int>& position)
{
    for (int track = 0; track < numTracks; ++track)
    {
        for (int step = 0; step < numSteps; ++step)
        {
            if (stepButtons[track][step])
            {
                if (stepButtons[track][step]->getBounds().contains(position))
                {
                    return stepButtons[track][step].get();
                }
            }
        }
    }
    return nullptr;
}
