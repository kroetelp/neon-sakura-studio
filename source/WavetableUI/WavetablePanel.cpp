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

//==============================================================================
// SynthMode Integration
//==============================================================================

void WavetablePanel::setSynthMode(SynthMode mode)
{
    if (currentMode == mode)
        return;

    currentMode = mode;

    // Mode-spezifische Flags setzen
    switch (mode)
    {
        case SynthMode::Perform:
            largeDisplayMode = true;
            fullEditorMode = false;
            minimizedMode = false;
            setDisplayMode(DisplayMode::Large);
            break;

        case SynthMode::Design:
            largeDisplayMode = false;
            fullEditorMode = true;
            minimizedMode = false;
            setDisplayMode(DisplayMode::Standard);
            break;

        case SynthMode::Patch:
            largeDisplayMode = false;
            fullEditorMode = false;
            minimizedMode = true;
            setDisplayMode(DisplayMode::Compact);
            break;
    }

    // Layout aktualisieren
    updateSynthModeLayout();
    resized();
}

//==============================================================================
// SynthMode-spezifische Features
//==============================================================================

void WavetablePanel::setLargeDisplayMode(bool enabled)
{
    largeDisplayMode = enabled;

    if (enabled)
        setDisplayMode(DisplayMode::Large);
    else
        setDisplayMode(DisplayMode::Standard);

    updateSynthModeLayout();
}

void WavetablePanel::setFullEditorMode(bool enabled)
{
    fullEditorMode = enabled;

    // Alle Editor-Sections zeigen/verstecken
    setWavetableEditorVisible(enabled);
    setEnvelopeEditorVisible(enabled);
    setFilterSectionVisible(enabled);

    updateSynthModeLayout();
}

void WavetablePanel::setMinimizedMode(bool enabled)
{
    minimizedMode = enabled;

    if (enabled)
        setDisplayMode(DisplayMode::Compact);
    else
        setDisplayMode(DisplayMode::Standard);

    updateSynthModeLayout();
}

//==============================================================================
// Perform Mode Features
//==============================================================================

void WavetablePanel::setQuickOscillator(const QuickOscillator& osc)
{
    quickOscillator = osc;

    // SynthEditor aktualisieren (TODO: Implementieren)
    if (synthEditor && sharedParams)
    {
        // Oscillator-Parameter setzen
        // ...
    }

    // Callback aufrufen
    if (quickOscillatorCallback)
        quickOscillatorCallback(osc);
}

//==============================================================================
// Design Mode Features
//==============================================================================

void WavetablePanel::setWavetableEditorVisible(bool show)
{
    wavetableEditorVisible = show;

    // TODO: WavetableEditor in SynthEditor zeigen/verstecken
    if (synthEditor)
    {
        // synthEditor->setWavetableEditorVisible(show);
    }
}

void WavetablePanel::setEnvelopeEditorVisible(bool show)
{
    envelopeEditorVisible = show;

    // TODO: EnvelopeEditor in SynthEditor zeigen/verstecken
    if (synthEditor)
    {
        // synthEditor->setEnvelopeEditorVisible(show);
    }
}

void WavetablePanel::setFilterSectionVisible(bool show)
{
    filterSectionVisible = show;

    // TODO: FilterSection in SynthEditor zeigen/verstecken
    if (synthEditor)
    {
        // synthEditor->setFilterSectionVisible(show);
    }
}

//==============================================================================
// Display Customization
//==============================================================================

void WavetablePanel::setDisplayMode(DisplayMode mode)
{
    displayMode = mode;

    // Display-Mode an SynthEditor weitergeben
    if (synthEditor)
    {
        // TODO: SynthEditor Display-Mode setzen
        // synthEditor->setDisplayMode(mode);
    }

    repaint();
}

//==============================================================================
// Layout Updates
//==============================================================================

void WavetablePanel::updateSynthModeLayout()
{
    if (!synthEditor)
        return;

    // Layout basierend auf Mode aktualisieren
    switch (currentMode)
    {
        case SynthMode::Perform:
            // Perform Mode: Großes Display zentriert
            // Quick-Controls werden von SynthWorkspacePanel verwaltet
            break;

        case SynthMode::Design:
            // Design Mode: Vollständiger Editor-Access
            // Alle Controls und Editors sichtbar
            break;

        case SynthMode::Patch:
            // Patch Mode: Minimiert
            // Nur Wavetable-Preview sichtbar
            break;
    }
}

//==============================================================================
// Painting
//==============================================================================

void WavetablePanel::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();

    // Background
    g.fillAll(theme.getPanelBackgroundColor());

    // Border
    g.setColour(theme.getAccentColor().withAlpha(0.3f));
    g.drawRect(getLocalBounds(), 1);

    // Mode-Indicator (kleiner Badge oben rechts)
    juce::String modeText;
    juce::Colour modeColor;

    switch (currentMode)
    {
        case SynthMode::Perform:
            modeText = "PERFORM";
            modeColor = juce::Colours::green;
            break;

        case SynthMode::Design:
            modeText = "DESIGN";
            modeColor = juce::Colours::orange;
            break;

        case SynthMode::Patch:
            modeText = "PATCH";
            modeColor = juce::Colours::purple;
            break;
    }

    // Badge zeichnen
    if (showLabels && !minimizedMode)
    {
        int badgeWidth = 70;
        int badgeHeight = 20;
        int badgeX = getWidth() - badgeWidth - 10;
        int badgeY = 10;

        g.setColour(modeColor);
        g.fillRoundedRectangle(juce::Rectangle<float>(badgeX, badgeY, badgeWidth, badgeHeight), 5.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(modeText, badgeX, badgeY, badgeWidth, badgeHeight, juce::Justification::centred);
    }

    // TODO: Mode change animation (requires Timer support)
}
