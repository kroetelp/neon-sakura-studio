// ============================================================================
// WavetablePanel.cpp - Implementierung
// ============================================================================

#include "WavetablePanel.h"
#include "WavetableSynthEditor.h"
#include "../WavetableSynth/WavetableEngine.h"
#include "../WavetableSynth/WavetableParams.h"

WavetablePanel::WavetablePanel()
    : DockablePanel(PanelType::WavetableSynth, "Wavetable Synth")
{
    // Header standardmäßig anzeigen für Undock-Button
}

WavetablePanel::~WavetablePanel()
{
    // SynthEditor wird automatisch durch unique_ptr gelöscht
}

void WavetablePanel::setEngine(WavetableEngine* engine)
{
    wavetableEngine = engine;
    sharedParams.reset();  // Nur ein Modus aktiv
    wavetableData.reset();

    if (engine && !synthEditor)
    {
        createSynthEditor();
    }
}

void WavetablePanel::setSharedParams(std::shared_ptr<WavetableParams> params,
                                      std::shared_ptr<WavetableData> data)
{
    sharedParams = params;
    wavetableData = data;
    wavetableEngine = nullptr;  // Nur ein Modus aktiv

    if (params && !synthEditor)
    {
        createSynthEditor();
    }
}

void WavetablePanel::createSynthEditor()
{
    // SynthEditor basierend auf Modus erstellen
    if (wavetableEngine)
    {
        synthEditor = std::make_unique<WavetableSynthEditor>(*wavetableEngine);
    }
    else if (sharedParams)
    {
        synthEditor = std::make_unique<WavetableSynthEditor>(sharedParams);
    }
    else
    {
        return;
    }

    addAndMakeVisible(*synthEditor);

    // Header nach vorne bringen
    if (header)
    {
        header->toFront(false);
    }

    updateSynthBounds();
}

void WavetablePanel::prepareForUndock()
{
    // Timer stoppen falls nötig
}

void WavetablePanel::prepareForDock()
{
    // Timer fortsetzen falls nötig
}

void WavetablePanel::onDockStateChanged(DockState newState)
{
    if (newState == DockState::Floating)
    {
        // Im Floating-Mode können wir den Header verstecken
        // da das Fenster eine eigene Title-Bar hat
        setShowHeader(false);
    }
    else if (newState == DockState::Docked)
    {
        // Im Docked-Mode zeigen wir unseren Header
        setShowHeader(true);
    }
}

juce::ValueTree WavetablePanel::saveState() const
{
    auto state = DockablePanel::saveState();
    state.setProperty("panelType", "WavetableSynth", nullptr);
    return state;
}

void WavetablePanel::restoreState(const juce::ValueTree& state)
{
    DockablePanel::restoreState(state);
}

void WavetablePanel::resized()
{
    DockablePanel::resized();
    updateSynthBounds();
}

void WavetablePanel::updateSynthBounds()
{
    if (synthEditor)
    {
        auto contentBounds = getContentBounds();
        if (!contentBounds.isEmpty())
        {
            synthEditor->setBounds(contentBounds);
        }
        else
        {
            synthEditor->setBounds(getLocalBounds());
        }
    }
}
