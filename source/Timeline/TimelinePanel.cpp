// ============================================================================
// TimelinePanel.cpp - Implementierung
// ============================================================================

#include "TimelinePanel.h"
#include "TimelineComponent.h"
#include "../SampleManager.h"

TimelinePanel::TimelinePanel()
    : DockablePanel(PanelType::Timeline, "Timeline")
{
    // Header ist standardmäßig aktiviert für DockablePanels
    // Der Header zeigt den "Undock" Button wenn angedockt
}

TimelinePanel::~TimelinePanel()
{
    // TimelineComponent wird automatisch durch unique_ptr gelöscht
}

void TimelinePanel::setDependencies(TimelineData* data, RecordingManager* recorder)
{
    timelineData = data;
    recordingManager = recorder;

    // TimelineComponent erstellen falls noch nicht geschehen
    if (!timelineComponent && timelineData && recordingManager)
    {
        createTimelineComponent();
    }
}

void TimelinePanel::setSampleManager(SampleManager* manager)
{
    sampleManager = manager;

    // An existierende TimelineComponent weiterreichen
    if (timelineComponent)
    {
        timelineComponent->setSampleManager(sampleManager);
    }
}

void TimelinePanel::createTimelineComponent()
{
    if (!timelineData || !recordingManager)
    {
        jassertfalse;  // Dependencies müssen gesetzt sein!
        return;
    }

    // TimelineComponent erstellen
    timelineComponent = std::make_unique<TimelineComponent>(*timelineData, *recordingManager);

    // SampleManager weiterreichen falls bereits gesetzt
    if (sampleManager)
    {
        timelineComponent->setSampleManager(sampleManager);
    }

    // Als Child hinzufügen
    addAndMakeVisible(*timelineComponent);

    // Header nach vorne bringen (über dem Content)
    if (header)
    {
        header->toFront(false);
    }

    // Initial Layout
    updateTimelineBounds();
}

void TimelinePanel::prepareForUndock()
{
    // Wird aufgerufen BEVOR das Panel aus dem MainComponent entfernt wird
    //
    // WICHTIG: Wir zerstören hier NICHTS! Die TimelineComponent bleibt
    // vollständig intakt. Wir können hier z.B.:
    // - Timer pausieren
    // - Audio-Verbindungen sichern
    // - State sichern
    //
    // Momentan ist nichts davon nötig - die TimelineComponent
    // überlebt den Transfer problemlos.
}

void TimelinePanel::prepareForDock()
{
    // Wird aufgerufen BEVOR das Panel wieder in das MainComponent eingefügt wird
    //
    // Hier könnten wir:
    // - Timer fortsetzen
    // - Audio-Verbindungen wiederherstellen
    //
    // Momentan nicht nötig.
}

void TimelinePanel::onDockStateChanged(DockState newState)
{
    // Wird aufgerufen NACH dem Dock-State geändert wurde
    //
    // Hier können wir auf den neuen State reagieren:
    // - Docked: Header anzeigen, normale Größe
    // - Floating: Header versteckt (Fenster hat eigene Title-Bar)
    // - Hidden: Timer stoppen, Ressourcen freigeben

    if (newState == DockState::Floating)
    {
        // Im Floating-Mode zeigt das DocumentWindow die Title-Bar
        // Wir können unseren eigenen Header verstecken
        setShowHeader(false);
    }
    else if (newState == DockState::Docked)
    {
        // Im Docked-Mode zeigen wir unseren Header mit Undock-Button
        setShowHeader(true);
    }
}

juce::ValueTree TimelinePanel::saveState() const
{
    // Basis-State speichern
    auto state = DockablePanel::saveState();

    // Timeline-spezifischen State hinzufügen
    state.setProperty("panelType", "Timeline", nullptr);

    // Optional: Zoom-Level speichern
    if (timelineComponent)
    {
        state.setProperty("horizontalZoom", timelineComponent->getHorizontalZoom(), nullptr);
    }

    return state;
}

void TimelinePanel::restoreState(const juce::ValueTree& state)
{
    // Basis-State restoren
    DockablePanel::restoreState(state);

    // Timeline-spezifischen State restoren
    if (timelineComponent && state.hasProperty("horizontalZoom"))
    {
        double zoom = state.getProperty("horizontalZoom");
        timelineComponent->setHorizontalZoom(zoom);
    }
}

void TimelinePanel::resized()
{
    // Basis-Klasse resized (Header Layout)
    DockablePanel::resized();

    // Timeline bounds aktualisieren
    updateTimelineBounds();
}

void TimelinePanel::updateTimelineBounds()
{
    if (timelineComponent)
    {
        // Content-Bereich vom DockablePanel holen
        // (berücksichtigt automatisch den Header wenn sichtbar)
        auto contentBounds = getContentBounds();

        if (!contentBounds.isEmpty())
        {
            timelineComponent->setBounds(contentBounds);
        }
        else
        {
            // Fallback: Gesamter Bereich
            timelineComponent->setBounds(getLocalBounds());
        }
    }
}
