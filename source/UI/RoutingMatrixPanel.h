#pragma once

/**
 * RoutingMatrixPanel - Visuelle Matrix für Audio/MIDI/Sidechain Routing
 *
 * Features:
 * - Drag-to-Connect: Von Source-Track zu Ziel-Plugin ziehen
 * - Live Preview: Audio-Signal visualisieren während Routing-Änderung
 * - Context Actions: Rechtsklick auf Connection = Quick-Menu
 * - Bulk Operations: Alle Tracks an Master routen mit einem Klick
 * - Visual Feedback: Signal-Stärke an Connection-Linien anzeigen
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include "../DockablePanel.h"

// Forward declarations
class AudioRoutingGraph;

// ============================================================================
// Connection Type - Welche Art von Verbindung?
// ============================================================================

enum class ConnectionType
{
    Audio,      // Audio-Verbindung
    Sidechain,  // Sidechain-Verbindung
    MIDI,       // MIDI-Verbindung
    SendReturn  // Send/Return-Verbindung
};

// ============================================================================
// Connection - Definiert eine Routing-Verbindung
// ============================================================================

struct Connection
{
    int id = -1;                     // Eindeutige ID dieser Verbindung
    int sourceTrackIndex = -1;       // Welcher Track liefert das Signal
    int targetTrackIndex = -1;       // Welcher Track empfängt das Signal
    uint32_t targetPluginNodeId = 0;  // Welches Plugin empfängt das Signal (0 = alle)
    int busIndex = 0;                // Sidechain-Bus-Index
    float gain = 1.0f;               // Verstärkung
    ConnectionType type = ConnectionType::Audio;
    bool enabled = true;

    juce::ValueTree saveState() const;
    void restoreState(const juce::ValueTree& state);
};

// ============================================================================
/**
 * RoutingMatrixPanel - Visuelle Routing-Matrix
 */
class RoutingMatrixPanel : public DockablePanel, private juce::Timer
{
public:
    RoutingMatrixPanel(AudioRoutingGraph& graph);
    ~RoutingMatrixPanel() override;

    // ============================================================
    // Connection Management
    // ============================================================

    int addConnection(const Connection& conn);
    bool removeConnection(int connectionId);
    void updateConnection(int connectionId, const Connection& conn);
    void setConnectionEnabled(int connectionId, bool enabled);

    // Bulk Operations
    void routeAllToMaster();
    void clearAllConnections();
    void muteAllSidechains();

    // ============================================================
    // Visual Features
    // ============================================================

    void showConnectionType(ConnectionType type);
    ConnectionType getVisibleConnectionType() const { return visibleConnectionType; }

    void highlightTrack(int trackIndex, bool highlight = true);
    void highlightPlugin(uint32_t pluginId, bool highlight = true);

    void setShowSignalLevels(bool show);
    bool getShowSignalLevels() const { return showSignalLevels; }

    void setSignalThreshold(float threshold);

    // ============================================================
    // Edit Mode
    // ============================================================

    void setEditMode(bool editMode);
    bool isEditMode() const { return editMode; }

    // ============================================================
    // Data Access
    // ============================================================

    const std::vector<Connection>& getAllConnections() const { return connections; }
    const Connection* getConnection(int connectionId) const;

    // ============================================================
    // Callbacks
    // ============================================================

    std::function<void(int connectionId)> onConnectionAdded;
    std::function<void(int connectionId)> onConnectionRemoved;
    std::function<void(int connectionId)> onConnectionChanged;

    // ============================================================
    // Overrides
    // ============================================================

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;

protected:
    void initializeContent() override;

private:
    // ============================================================
    // Audio Graph Reference
    // ============================================================

    AudioRoutingGraph& audioGraph;

    // ============================================================
    // Connection Storage
    // ============================================================

    std::vector<Connection> connections;
    int nextConnectionId = 1;

    // ============================================================
    // Visual State
    // ============================================================

    ConnectionType visibleConnectionType = ConnectionType::Audio;
    std::unordered_map<int, bool> highlightedTracks;
    std::unordered_map<uint32_t, bool> highlightedPlugins;
    bool showSignalLevels = true;
    float signalThreshold = -60.0f;  // dB für Signal-Anzeige

    // ============================================================
    // Drag State
    // ============================================================

    bool isDragging = false;
    int dragSourceTrack = -1;
    juce::Point<int> dragStartPos;
    ConnectionType dragConnectionType = ConnectionType::Audio;
    juce::Point<int> currentDragPos;

    // ============================================================
    // Edit Mode
    // ============================================================

    bool editMode = false;

    // ============================================================
    // Visual Layout
    // ============================================================

    static constexpr int headerHeight = 40;
    static constexpr int tabHeight = 30;
    static constexpr int sourceColumnWidth = 120;
    static constexpr int gridCellSize = 50;
    static constexpr int gridPadding = 4;

    // ============================================================
    // Signal Level Animation
    // ============================================================

    struct SignalLevel
    {
        float level = -60.0f;
        float targetLevel = -60.0f;
    };
    std::unordered_map<int, SignalLevel> connectionSignalLevels;

    // ============================================================
    // Helpers
    // ============================================================

    juce::Rectangle<int> getGridCellBounds(int sourceIndex, int targetIndex) const;
    void drawConnectionLine(juce::Graphics& g, const Connection& conn);
    void drawConnectionDot(juce::Graphics& g, juce::Point<float> position, juce::Colour color, bool isActive);
    juce::Colour getConnectionColor(ConnectionType type, bool isActive) const;
    juce::String getConnectionTypeName(ConnectionType type) const;

    void handleConnectionDrop(const juce::Point<int>& position);
    bool isOverGridCell(const juce::Point<int>& position, int& sourceIndex, int& targetIndex) const;
    bool isOverSourceColumn(const juce::Point<int>& position, int& trackIndex) const;
    bool isOverGridArea(const juce::Point<int>& position, int& trackIndex, int& pluginIndex) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RoutingMatrixPanel)
};
