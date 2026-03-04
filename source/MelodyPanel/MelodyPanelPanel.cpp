// ============================================================================
// MelodyPanelPanel.cpp - Implementierung
// ============================================================================

#include "MelodyPanelPanel.h"
#include "MelodyPanel.h"

MelodyPanelPanel::MelodyPanelPanel()
    : DockablePanel(PanelType::MelodyPanel, "Melody Panel")
{
    // Header ist standardmäßig aktiviert für DockablePanels
}

MelodyPanelPanel::~MelodyPanelPanel()
{
    // MelodyPanel wird automatisch durch unique_ptr gelöscht
}

void MelodyPanelPanel::createMelodyPanel()
{
    if (melodyPanel)
        return;  // Bereits erstellt

    melodyPanel = std::make_unique<MelodyPanel>();

    // Callbacks weiterleiten
    melodyPanel->onApplyMelody = [this](int trackIndex, const std::vector<std::pair<int, int>>& stepPitches) {
        if (onApplyMelody)
            onApplyMelody(trackIndex, stepPitches);
    };

    // Als Child hinzufügen
    addAndMakeVisible(*melodyPanel);

    // Target Track setzen falls bereits bekannt
    if (targetTrack != 0)
        melodyPanel->setTargetTrack(targetTrack);

    // Header nach vorne bringen (über dem Content)
    if (header)
    {
        header->toFront(false);
    }

    // Initial Layout
    updateMelodyPanelBounds();
}

void MelodyPanelPanel::setTargetTrack(int trackIndex)
{
    targetTrack = trackIndex;

    if (melodyPanel)
    {
        melodyPanel->setTargetTrack(trackIndex);
    }
}

void MelodyPanelPanel::updateMelodyPanelBounds()
{
    if (melodyPanel)
    {
        // Content-Bereich vom DockablePanel holen
        auto contentBounds = getContentBounds();

        if (!contentBounds.isEmpty())
        {
            melodyPanel->setBounds(contentBounds);
        }
        else
        {
            // Fallback: Gesamter Bereich
            melodyPanel->setBounds(getLocalBounds());
        }
    }
}

void MelodyPanelPanel::prepareForUndock()
{
    // Wird aufgerufen BEVOR das Panel aus dem MainComponent entfernt wird
    // Der MelodyPanel bleibt vollständig intakt
}

void MelodyPanelPanel::prepareForDock()
{
    // Wird aufgerufen BEVOR das Panel wieder in das MainComponent eingefügt wird
    // Momentan nicht nötig
}

void MelodyPanelPanel::onDockStateChanged(DockState newState)
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

juce::ValueTree MelodyPanelPanel::saveState() const
{
    auto state = DockablePanel::saveState();
    state.setProperty("panelType", "MelodyPanel", nullptr);

    // Target Track speichern
    state.setProperty("targetTrack", targetTrack, nullptr);

    return state;
}

void MelodyPanelPanel::restoreState(const juce::ValueTree& state)
{
    DockablePanel::restoreState(state);

    if (state.hasProperty("targetTrack"))
    {
        targetTrack = static_cast<int>(state.getProperty("targetTrack"));
        if (melodyPanel)
            melodyPanel->setTargetTrack(targetTrack);
    }
}

void MelodyPanelPanel::resized()
{
    DockablePanel::resized();
    updateMelodyPanelBounds();
}
