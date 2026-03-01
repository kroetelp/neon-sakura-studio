// ============================================================================
// DockablePanel.h - Basisklasse für alle dockbaren Panels
// ============================================================================
//
// Jedes Panel in Neon Sakura Studio erbt von dieser Klasse.
// Sie verwaltet:
// - Den aktuellen Dock-Status (Docked/Floating/Hidden)
// - Die Panel-ID und Position im Layout
// - Den Übergang zwischen Docked/Floating ohne State-Verlust
// - State-Persistenz über ValueTree

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>

// Forward declarations
class DockingManager;

/**
 * Enum für Panel-Typen - eindeutige IDs für jedes Panel
 */
enum class PanelType
{
    Timeline,
    WavetableSynth,
    RhythmExplorer,
    MelodyPanel,
    Mixer,
    StepSequencer,
    TrackEditor,
    Unknown
};

/**
 * DockState - Wo befindet sich das Panel gerade?
 */
enum class DockState
{
    Docked,     // Im Hauptfenster eingebettet
    Floating,   // In eigenem DocumentWindow
    Hidden,     // Nicht sichtbar
    Minimized   // Minimiert (optional für später)
};

/**
 * DockPosition - Wo im Layout ist das Panel angedockt?
 */
enum class DockPosition
{
    Center,     // Hauptbereich (Tabs)
    Left,       // Linke Sidebar
    Right,      // Rechte Sidebar
    Bottom,     // Unten (z.B. Mixer)
    Top         // Oben (selten verwendet)
};

// ============================================================================
/**
 * PanelHeader - Header mit Undock-Button und Titel
 *
 * Wird automatisch über jedem DockablePanel angezeigt,
 * wenn es im Hauptfenster angedockt ist.
 */
class PanelHeader : public juce::Component
{
public:
    explicit PanelHeader(const juce::String& panelName);
    ~PanelHeader() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // === Callbacks ===
    std::function<void()> onUndockClicked;
    std::function<void()> onCloseClicked;

    // === Configuration ===
    void setPanelName(const juce::String& name);
    void setShowUndockButton(bool show);
    void setShowCloseButton(bool show);

    static constexpr int headerHeight = 32;

private:
    juce::String panelName;

    juce::TextButton undockButton;
    juce::TextButton closeButton;
    juce::Label titleLabel;

    bool showUndock = true;
    bool showClose = true;

    void setupButton(juce::TextButton& btn, const juce::String& tooltip);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelHeader)
};

// ============================================================================
/**
 * DockablePanel - Basisklasse für alle Panels
 *
 * Jedes Panel in der Anwendung erbt von dieser Klasse.
 *
 * WICHTIG: Die Panel-Instanz bleibt IMMER im Besitz des DockingManager.
 * Beim Undocken wird das Panel nur aus dem Parent entfernt und in ein
 * FloatingWindow verschoben - es wird NIEMALS neu erstellt.
 */
class DockablePanel : public juce::Component
{
public:
    DockablePanel(PanelType type, const juce::String& panelName);
    ~DockablePanel() override;

    // === Identität ===
    PanelType getPanelType() const { return panelType; }
    juce::String getPanelName() const { return panelName; }
    juce::String getPanelID() const;

    // === Docking State ===
    DockState getDockState() const { return dockState; }
    DockPosition getDockPosition() const { return dockPosition; }
    bool isDocked() const { return dockState == DockState::Docked; }
    bool isFloating() const { return dockState == DockState::Floating; }
    bool isHidden() const { return dockState == DockState::Hidden; }
    bool isVisibleInLayout() const { return dockState == DockState::Docked || dockState == DockState::Floating; }

    // === Docking Operations (aufgerufen vom DockingManager) ===

    // Wird aufgerufen BEVOR das Panel aus dem MainComponent entfernt wird
    // Subklassen können hier State sichern oder Audio pausieren
    virtual void prepareForUndock() {}

    // Wird aufgerufen BEVOR das Panel in das MainComponent eingefügt wird
    // Subklassen können hier State wiederherstellen
    virtual void prepareForDock() {}

    // Wird aufgerufen NACH dem Dock-State geändert wurde
    virtual void onDockStateChanged(DockState /*newState*/) {}

    // === State-Persistenz ===

    // Speichere Panel-spezifischen State (Override in Subklassen)
    virtual juce::ValueTree saveState() const;

    // Stelle Panel-spezifischen State wieder her (Override in Subklassen)
    virtual void restoreState(const juce::ValueTree& state);

    // === Preferred Size ===

    // Bevorzugte Größe wenn angedockt
    virtual juce::Rectangle<int> getPreferredDockedBounds() const;

    // Bevorzugte Größe wenn floating
    virtual juce::Rectangle<int> getPreferredFloatingBounds() const;

    // Minimale Größe
    virtual int getMinimumWidth() const { return 200; }
    virtual int getMinimumHeight() const { return 150; }

    // === Header Control ===
    void setShowHeader(bool show);
    bool shouldShowHeader() const { return showHeader; }

    // === Internal: Vom DockingManager gesetzt ===
    void setDockStateInternal(DockState state);
    void setDockPositionInternal(DockPosition position);

    // === Callbacks für DockingManager ===
    // Diese werden vom DockingManager gesetzt, damit das Panel
    // Undock/Close Requests an den Manager weiterleiten kann
    std::function<void()> onRequestUndock;
    std::function<void()> onRequestClose;

    // === Look & Feel Colors (static für konsistente Colors) ===
    static juce::Colour getHeaderBackgroundColor();
    static juce::Colour getHeaderTextColor();
    static juce::Colour getHeaderButtonColor();

protected:
    PanelType panelType;
    juce::String panelName;
    DockState dockState = DockState::Hidden;
    DockPosition dockPosition = DockPosition::Right;
    bool showHeader = true;

    // Header-Component (wird automatisch erstellt/verwaltet)
    std::unique_ptr<PanelHeader> header;

    // === Content Area (für Subklassen) ===
    // Gibt den Bereich zurück, in dem der eigentliche Panel-Content
    // gezeichnet werden soll (unterhalb des Headers)
    juce::Rectangle<int> getContentBounds() const;

    // === Component Override ===
    void resized() override;
    void paint(juce::Graphics& g) override;

    // === Wird von Subklassen aufgerufen, um den Content zu erstellen ===
    // Subklassen können ihre UI-Elemente hier initialisieren
    virtual void initializeContent() {}

private:
    void createHeader();
    void updateHeaderVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockablePanel)
};
