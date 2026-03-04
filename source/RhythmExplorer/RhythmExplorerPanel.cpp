// ============================================================================
// RhythmExplorerPanel.cpp - Implementierung
// ============================================================================

#include "RhythmExplorerPanel.h"
#include "RhythmExplorer.h"

RhythmExplorerPanel::RhythmExplorerPanel()
    : DockablePanel(PanelType::RhythmExplorer, "Rhythm Explorer")
{
    // Header ist standardmäßig aktiviert für DockablePanels
}

RhythmExplorerPanel::~RhythmExplorerPanel()
{
    // RhythmExplorer wird automatisch durch unique_ptr gelöscht
}

void RhythmExplorerPanel::createRhythmExplorer()
{
    if (rhythmExplorer)
        return;  // Bereits erstellt

    rhythmExplorer = std::make_unique<RhythmExplorer>();

    // Callbacks weiterleiten
    rhythmExplorer->onApplyPattern = [this](int trackIndex, const std::vector<int>& steps, bool clearFirst) {
        if (onApplyPattern)
            onApplyPattern(trackIndex, steps, clearFirst);
    };

    rhythmExplorer->onApplyFill = [this](int trackIndex, const std::vector<int>& steps) {
        if (onApplyFill)
            onApplyFill(trackIndex, steps);
    };

    // Als Child hinzufügen
    addAndMakeVisible(*rhythmExplorer);

    // Target Track setzen falls bereits bekannt
    if (targetTrack != 0)
        rhythmExplorer->setTargetTrack(targetTrack);

    // Header nach vorne bringen (über dem Content)
    if (header)
    {
        header->toFront(false);
    }

    // Initial Layout
    updateRhythmExplorerBounds();
}

void RhythmExplorerPanel::setTargetTrack(int trackIndex)
{
    targetTrack = trackIndex;

    if (rhythmExplorer)
    {
        rhythmExplorer->setTargetTrack(trackIndex);
    }
}

void RhythmExplorerPanel::updateRhythmExplorerBounds()
{
    if (rhythmExplorer)
    {
        // Content-Bereich vom DockablePanel holen
        // (berücksichtigt automatisch den Header wenn sichtbar)
        auto contentBounds = getContentBounds();

        if (!contentBounds.isEmpty())
        {
            rhythmExplorer->setBounds(contentBounds);
        }
        else
        {
            // Fallback: Gesamter Bereich
            rhythmExplorer->setBounds(getLocalBounds());
        }
    }
}

void RhythmExplorerPanel::prepareForUndock()
{
    // Wird aufgerufen BEVOR das Panel aus dem MainComponent entfernt wird
    // Der RhythmExplorer bleibt vollständig intakt
}

void RhythmExplorerPanel::prepareForDock()
{
    // Wird aufgerufen BEVOR das Panel wieder in das MainComponent eingefügt wird
    // Momentan nicht nötig
}

void RhythmExplorerPanel::onDockStateChanged(DockState newState)
{
    if (newState == DockState::Floating)
    {
        setShowHeader(false);
    }
    else if (newState == DockState::Docked)
    {
        setShowHeader(true);
    }
}

juce::ValueTree RhythmExplorerPanel::saveState() const
{
    auto state = DockablePanel::saveState();
    state.setProperty("panelType", "RhythmExplorer", nullptr);

    // Target Track speichern
    state.setProperty("targetTrack", targetTrack, nullptr);

    return state;
}

void RhythmExplorerPanel::restoreState(const juce::ValueTree& state)
{
    DockablePanel::restoreState(state);

    if (state.hasProperty("targetTrack"))
    {
        targetTrack = static_cast<int>(state.getProperty("targetTrack"));
        if (rhythmExplorer)
            rhythmExplorer->setTargetTrack(targetTrack);
    }
}

void RhythmExplorerPanel::resized()
{
    DockablePanel::resized();
    updateRhythmExplorerBounds();
}
