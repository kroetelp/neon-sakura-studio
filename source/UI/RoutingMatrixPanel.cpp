#include "RoutingMatrixPanel.h"
#include "../AudioRouting/AudioRoutingGraph.h"
#include "../Theme/ThemeManager.h"

//==============================================================================
RoutingMatrixPanel::RoutingMatrixPanel(AudioRoutingGraph& graph)
    : DockablePanel(PanelType::RoutingMatrix, "Routing Matrix")
    , audioGraph(graph)
{
    setOpaque(true);
}

RoutingMatrixPanel::~RoutingMatrixPanel()
{
    stopTimer();
}

//==============================================================================
void RoutingMatrixPanel::initializeContent()
{
    // Starte Timer für Signal-Level Animationen (30fps)
    startTimerHz(30);
}

//==============================================================================
int RoutingMatrixPanel::addConnection(const Connection& conn)
{
    Connection newConn = conn;
    newConn.id = nextConnectionId++;

    connections.push_back(newConn);

    if (onConnectionAdded)
        onConnectionAdded(newConn.id);

    repaint();
    return newConn.id;
}

bool RoutingMatrixPanel::removeConnection(int connectionId)
{
    auto it = std::find_if(connections.begin(), connections.end(),
        [connectionId](const Connection& conn) { return conn.id == connectionId; });

    if (it != connections.end())
    {
        connections.erase(it);

        if (onConnectionRemoved)
            onConnectionRemoved(connectionId);

        repaint();
        return true;
    }

    return false;
}

void RoutingMatrixPanel::updateConnection(int connectionId, const Connection& conn)
{
    for (auto& existingConn : connections)
    {
        if (existingConn.id == connectionId)
        {
            existingConn = conn;

            if (onConnectionChanged)
                onConnectionChanged(connectionId);

            repaint();
            return;
        }
    }
}

void RoutingMatrixPanel::setConnectionEnabled(int connectionId, bool enabled)
{
    for (auto& conn : connections)
    {
        if (conn.id == connectionId)
        {
            conn.enabled = enabled;
            repaint();
            return;
        }
    }
}

//==============================================================================
void RoutingMatrixPanel::routeAllToMaster()
{
    int numTracks = 8;  // TODO: Get from AudioGraph

    for (int i = 0; i < numTracks; ++i)
    {
        Connection conn;
        conn.sourceTrackIndex = i;
        conn.targetTrackIndex = -1;  // Master output
        conn.targetPluginNodeId = 0;
        conn.gain = 1.0f;
        conn.type = visibleConnectionType;
        conn.enabled = true;

        addConnection(conn);
    }
}

void RoutingMatrixPanel::clearAllConnections()
{
    for (const auto& conn : connections)
    {
        if (onConnectionRemoved)
            onConnectionRemoved(conn.id);
    }

    connections.clear();
    repaint();
}

void RoutingMatrixPanel::muteAllSidechains()
{
    for (auto& conn : connections)
    {
        if (conn.type == ConnectionType::Sidechain && conn.enabled)
        {
            conn.enabled = false;

            if (onConnectionChanged)
                onConnectionChanged(conn.id);
        }
    }

    repaint();
}

//==============================================================================
void RoutingMatrixPanel::showConnectionType(ConnectionType type)
{
    if (visibleConnectionType != type)
    {
        visibleConnectionType = type;
        repaint();
    }
}

void RoutingMatrixPanel::highlightTrack(int trackIndex, bool highlight)
{
    highlightedTracks[trackIndex] = highlight;
    repaint();
}

void RoutingMatrixPanel::highlightPlugin(uint32_t pluginId, bool highlight)
{
    highlightedPlugins[pluginId] = highlight;
    repaint();
}

void RoutingMatrixPanel::setShowSignalLevels(bool show)
{
    if (showSignalLevels != show)
    {
        showSignalLevels = show;
        repaint();
    }
}

void RoutingMatrixPanel::setSignalThreshold(float threshold)
{
    signalThreshold = threshold;
    repaint();
}

//==============================================================================
void RoutingMatrixPanel::setEditMode(bool edit)
{
    editMode = edit;
    repaint();
}

const Connection* RoutingMatrixPanel::getConnection(int connectionId) const
{
    auto it = std::find_if(connections.begin(), connections.end(),
        [connectionId](const Connection& conn) { return conn.id == connectionId; });

    if (it != connections.end())
    {
        return &(*it);
    }

    return nullptr;
}

//==============================================================================
void RoutingMatrixPanel::timerCallback()
{
    // Animate signal levels for smooth transitions
    bool needsRepaint = false;
    for (auto& [connId, signalLevel] : connectionSignalLevels)
    {
        float diff = signalLevel.targetLevel - signalLevel.level;
        if (std::abs(diff) > 0.1f)
        {
            signalLevel.level += diff * 0.1f;  // Smooth interpolation
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

//==============================================================================
void RoutingMatrixPanel::paint(juce::Graphics& g)
{
    auto& theme = ThemeManager::getInstance();
    auto bounds = getLocalBounds().toFloat();

    // Hintergrund
    g.fillAll(theme.getPanelBackgroundColor());

    // Content-Bereich
    auto contentArea = getContentBounds().toFloat();

    // Header
    auto headerBounds = juce::Rectangle<float>(
        contentArea.getX(),
        contentArea.getY(),
        contentArea.getWidth(),
        headerHeight
    );
    auto headerBg = theme.getPanelHeaderColor();
    g.setColour(headerBg);
    g.fillRect(headerBounds);

    // Header-Titel
    g.setColour(theme.getTextPrimaryColor());
    g.setFont(14.0f);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("Routing Matrix",
              headerBounds.reduced(10, 0),
              juce::Justification::centredLeft,
              false);

    // Connection Type Tabs
    float tabWidth = 90.0f;
    float tabY = headerBounds.getBottom() - tabHeight - 4;

    auto connectionTypes = { ConnectionType::Audio, ConnectionType::Sidechain,
                            ConnectionType::MIDI, ConnectionType::SendReturn };

    size_t tabIndex = 0;
    for (auto type : connectionTypes)
    {
        bool isActive = (type == visibleConnectionType);
        float x = 10 + static_cast<float>(tabIndex) * (tabWidth + 4);

        auto tabBounds = juce::Rectangle<float>(x, tabY, tabWidth, tabHeight);

        // Tab Hintergrund
        g.setColour(isActive ? theme.getAccentColor() : theme.getButtonColor());
        g.fillRoundedRectangle(tabBounds, 4.0f);

        // Tab Text
        g.setColour(isActive ? juce::Colours::white : theme.getTextPrimaryColor());
        g.setFont(11.0f);
        g.drawText(getConnectionTypeName(type),
                  tabBounds,
                  juce::Justification::centred,
                  false);

        tabIndex++;
    }

    // Grid-Bereich
    auto gridBounds = contentArea.withTrimmedTop(headerHeight);

    // Zeichne Grid
    int numTracks = 8;  // TODO: Get from AudioGraph
    int numPlugins = 8;  // TODO: Get from AudioGraph

    auto gridWidth = gridBounds.getWidth();
    auto gridHeight = gridBounds.getHeight();

    auto startX = gridBounds.getX();
    auto startY = gridBounds.getY();

    // Grid-Hintergrund
    g.setColour(theme.getPanelBackgroundColor());
    g.fillRect(gridBounds);

    // Source-Spalte (Tracks)
    for (int track = 0; track < numTracks; ++track)
    {
        auto y = startY + tabHeight + static_cast<float>(track) * gridCellSize;

        // Track-Label
        g.setColour(theme.getTextSecondaryColor());
        g.setFont(10.0f);
        g.drawText("Track " + juce::String(track + 1),
                  juce::Rectangle<float>(startX, y - 15, sourceColumnWidth - 10, 12),
                  juce::Justification::centredRight,
                  false);

        // Track-Highlight
        if (highlightedTracks[track])
        {
            g.setColour(theme.getAccentColor().withAlpha(0.15f));
            g.fillRect(juce::Rectangle<float>(
                startX,
                y,
                gridWidth,
                gridCellSize
            ));
        }

        // Zellen-Border
        g.setColour(theme.getPanelBorderColor().withAlpha(0.3f));
        g.drawLine(juce::Line<float>(
            startX, y + gridCellSize,
            startX + gridWidth, y + gridCellSize
        ));
    }

    // Header-Zeile (Plugins)
    auto gridStartX = startX + sourceColumnWidth;
    for (int plugin = 0; plugin < numPlugins; ++plugin)
    {
        auto x = gridStartX + static_cast<float>(plugin) * gridCellSize;

        g.setColour(theme.getTextSecondaryColor());
        g.setFont(10.0f);
        g.drawText("P" + juce::String(plugin + 1),
                  juce::Rectangle<float>(x, startY, gridCellSize, 15),
                  juce::Justification::centred,
                  false);

        // Plugin-Highlight
        if (highlightedPlugins[static_cast<uint32_t>(plugin)])
        {
            g.setColour(theme.getAccentColor().withAlpha(0.15f));
            g.fillRect(juce::Rectangle<float>(
                x, startY,
                gridCellSize, tabHeight + gridCellSize * numTracks
            ));
        }

        // Vertikale Lines
        g.setColour(theme.getPanelBorderColor().withAlpha(0.2f));
        g.drawLine(juce::Line<float>(
            x + gridCellSize, startY,
            x + gridCellSize, startY + tabHeight + gridCellSize * numTracks
        ));
    }

    // Grid-Zellen und Verbindungen
    for (int track = 0; track < numTracks; ++track)
    {
        for (int plugin = 0; plugin < numPlugins; ++plugin)
        {
            auto cellBounds = getGridCellBounds(track, plugin);

            // Zellen-Hintergrund (Schachbrett-Muster)
            if ((track + plugin) % 2 == 0)
            {
                g.setColour(theme.getPanelBorderColor().withAlpha(0.05f));
                g.fillRect(cellBounds.toFloat());
            }

            // Zeichne Connection Dot
            for (const auto& conn : connections)
            {
                if (conn.sourceTrackIndex == track &&
                    conn.targetPluginNodeId == static_cast<uint32_t>(plugin))
                {
                    if (conn.type == visibleConnectionType || visibleConnectionType == ConnectionType::Audio)
                    {
                        auto isActive = conn.enabled;
                        auto color = getConnectionColor(conn.type, isActive);

                        if (isActive)
                        {
                            drawConnectionDot(g, cellBounds.getCentre().toFloat(), color, true);

                            // Signal-Level Anzeige
                            if (showSignalLevels && conn.enabled)
                            {
                                float signalLevel = connectionSignalLevels[conn.id].level;
                                if (signalLevel > signalThreshold)
                                {
                                    auto dotSize = 12.0f;
                                    auto alpha = juce::jmap(signalLevel, signalThreshold, 0.0f, 0.3f, 0.8f);
                                    g.setColour(color.withAlpha(alpha));
                                    g.fillEllipse(
                                        juce::Rectangle<float>(
                                            cellBounds.getCentre().getX() - dotSize / 2,
                                            cellBounds.getCentre().getY() - dotSize / 2,
                                            dotSize, dotSize
                                        )
                                    );
                                }
                            }
                        }
                        else
                        {
                            drawConnectionDot(g, cellBounds.getCentre().toFloat(), color, false);
                        }
                    }
                }
            }

            // Zellen-Border
            g.setColour(theme.getPanelBorderColor().withAlpha(0.2f));
            g.drawRect(cellBounds.toFloat(), 0.5f);
        }
    }

    // Drag-Line zeichnen
    if (isDragging)
    {
        auto gridY = startY + tabHeight;
        auto sourceX = startX + sourceColumnWidth / 2;
        auto sourceY = gridY + static_cast<float>(dragSourceTrack) * gridCellSize + gridCellSize / 2;

        g.setColour(getConnectionColor(dragConnectionType, true).withAlpha(0.6f));
        g.drawLine(juce::Line<float>(
            juce::Point<float>(sourceX, sourceY),
            currentDragPos.toFloat()
        ), 2.0f);

        // Ziel-Highlight anzeigen
        int trackIndex, pluginIndex;
        if (isOverGridCell(currentDragPos, trackIndex, pluginIndex))
        {
            if (trackIndex >= 0)
            {
                auto highlightY = gridY + static_cast<float>(trackIndex) * gridCellSize;
                g.setColour(getConnectionColor(dragConnectionType, true).withAlpha(0.1f));
                g.fillRect(juce::Rectangle<float>(
                    startX, highlightY,
                    sourceColumnWidth, gridCellSize
                ));
            }
            else if (pluginIndex >= 0)
            {
                auto cellBounds = getGridCellBounds(dragSourceTrack, pluginIndex);
                g.setColour(getConnectionColor(dragConnectionType, true).withAlpha(0.2f));
                g.fillRoundedRectangle(cellBounds.toFloat(), 6.0f);
            }
        }
    }
}

void RoutingMatrixPanel::resized()
{
    // Layout wird komplett in paint() über graphics berechnet
}

void RoutingMatrixPanel::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        currentDragPos = e.position.toInt();
        repaint();
    }
}

void RoutingMatrixPanel::mouseUp(const juce::MouseEvent& e)
{
    if (isDragging)
    {
        isDragging = false;
        handleConnectionDrop(e.position.toInt());
    }
}

void RoutingMatrixPanel::mouseDown(const juce::MouseEvent& e)
{
    if (!editMode)
        return;

    auto pos = e.position.toInt();

    // Prüfe, ob wir über einer Source-Zelle starten
    int trackIndex, pluginIndex;
    if (isOverSourceColumn(pos, trackIndex))
    {
        if (trackIndex >= 0)
        {
            isDragging = true;
            dragSourceTrack = trackIndex;
            dragStartPos = pos;
            currentDragPos = pos;
            dragConnectionType = visibleConnectionType;
        }
    }
}

//==============================================================================
juce::Rectangle<int> RoutingMatrixPanel::getGridCellBounds(int sourceIndex, int targetIndex) const
{
    auto gridStartX = sourceColumnWidth;
    auto gridStartY = headerHeight + tabHeight;

    auto x = gridStartX + targetIndex * gridCellSize + gridPadding;
    auto y = gridStartY + sourceIndex * gridCellSize + gridPadding;

    return juce::Rectangle<int>(x, y, gridCellSize - gridPadding * 2, gridCellSize - gridPadding * 2);
}

void RoutingMatrixPanel::drawConnectionLine(juce::Graphics& g, const Connection& conn)
{
    auto color = getConnectionColor(conn.type, conn.enabled);
    float lineThickness = conn.enabled ? 2.5f : 1.0f;

    // Berechne Koordinaten
    auto gridY = headerHeight + tabHeight;

    auto sourceX = sourceColumnWidth / 2;
    auto sourceY = gridY + conn.sourceTrackIndex * gridCellSize + gridCellSize / 2;

    auto targetX = sourceColumnWidth + static_cast<int>(conn.targetPluginNodeId) * gridCellSize + gridCellSize / 2;
    auto targetY = gridY;

    // Zeichne Verbindung
    g.setColour(color);

    // Bézier-Kurve für schönere Verbindung
    juce::Path path;
    path.startNewSubPath(juce::Point<float>(sourceX, sourceY));

    // Kontrollpunkte für Bézier-Kurve
    auto ctrl1 = juce::Point<float>(
        sourceX,
        sourceY + (targetY - sourceY) * 0.5f
    );
    auto ctrl2 = juce::Point<float>(
        targetX,
        targetY - (targetY - sourceY) * 0.5f
    );
    auto end = juce::Point<float>(targetX, targetY);

    path.cubicTo(ctrl1, ctrl2, end);
    g.strokePath(path, juce::PathStrokeType(lineThickness));

    // Signal-Level Anzeige (als kleiner Punkt an der Linie)
    if (showSignalLevels && conn.enabled)
    {
        float signalLevel = connectionSignalLevels[conn.id].level;
        if (signalLevel > signalThreshold)
        {
            auto t = 0.5f;
            auto pos = juce::Point<float>(
                sourceX + (targetX - sourceX) * t,
                sourceY + (targetY - sourceY) * t
            );

            auto indicatorSize = lineThickness * 2.0f;
            g.setColour(color.withAlpha(0.8f));
            g.fillEllipse(
                juce::Rectangle<float>(
                    pos.x - indicatorSize / 2,
                    pos.y - indicatorSize / 2,
                    indicatorSize,
                    indicatorSize
                )
            );
        }
    }
}

void RoutingMatrixPanel::drawConnectionDot(juce::Graphics& g, juce::Point<float> position, juce::Colour color, bool isActive)
{
    float size = isActive ? 10.0f : 7.0f;

    // Glow-Effekt für aktive Verbindungen
    if (isActive)
    {
        auto glowColor = color.withAlpha(0.25f);
        g.setColour(glowColor);
        g.fillEllipse(
            juce::Rectangle<float>(
                position.x - size / 2 - 3,
                position.y - size / 2 - 3,
                size + 6, size + 6
            )
        );
    }

    // Haupt-Dot
    g.setColour(color);
    g.fillEllipse(
        juce::Rectangle<float>(
            position.x - size / 2,
            position.y - size / 2,
            size, size
        )
    );

    // Punkt in der Mitte für aktive Verbindungen
    if (isActive)
    {
        g.setColour(juce::Colours::white);
        g.fillEllipse(
            juce::Rectangle<float>(
                position.x - 2,
                position.y - 2,
                4, 4
            )
        );
    }
}

juce::Colour RoutingMatrixPanel::getConnectionColor(ConnectionType type, bool isActive) const
{
    auto& theme = ThemeManager::getInstance();

    switch (type)
    {
        case ConnectionType::Audio:
            return isActive ? theme.getAccentColor() : theme.getInfoColor();

        case ConnectionType::Sidechain:
            return isActive ? theme.getSuccessColor() : theme.getWarningColor();

        case ConnectionType::MIDI:
            return isActive ? theme.getMidiNoteColor() : theme.getTextSecondaryColor();

        case ConnectionType::SendReturn:
            return isActive ? theme.getWarningColor() : theme.getInfoColor();
    }

    return theme.getTextPrimaryColor();
}

juce::String RoutingMatrixPanel::getConnectionTypeName(ConnectionType type) const
{
    switch (type)
    {
        case ConnectionType::Audio:      return "Audio";
        case ConnectionType::Sidechain: return "Sidechain";
        case ConnectionType::MIDI:       return "MIDI";
        case ConnectionType::SendReturn: return "Send/Ret";
    }

    return "Unknown";
}

//==============================================================================
void RoutingMatrixPanel::handleConnectionDrop(const juce::Point<int>& position)
{
    int sourceIndex, targetIndex;
    if (isOverGridCell(position, sourceIndex, targetIndex))
    {
        if (targetIndex >= 0 && dragSourceTrack >= 0)
        {
            // Prüfe ob Verbindung bereits existiert
            bool exists = false;
            for (const auto& conn : connections)
            {
                if (conn.sourceTrackIndex == dragSourceTrack &&
                    conn.targetPluginNodeId == static_cast<uint32_t>(targetIndex) &&
                    conn.type == visibleConnectionType)
                {
                    // Toggle existierende Verbindung
                    setConnectionEnabled(conn.id, !conn.enabled);
                    exists = true;
                    break;
                }
            }

            // Neue Verbindung erstellen
            if (!exists)
            {
                Connection conn;
                conn.id = -1;  // wird von addConnection gesetzt
                conn.sourceTrackIndex = dragSourceTrack;
                conn.targetTrackIndex = -1;  // Plugin target
                conn.targetPluginNodeId = static_cast<uint32_t>(targetIndex);
                conn.gain = 1.0f;
                conn.type = visibleConnectionType;
                conn.enabled = true;

                addConnection(conn);

                // Initialisiere Signal-Level für Animation
                connectionSignalLevels[nextConnectionId - 1] = { -60.0f, -60.0f };
            }
        }
    }

    isDragging = false;
    repaint();
}

bool RoutingMatrixPanel::isOverGridCell(const juce::Point<int>& position, int& sourceIndex, int& targetIndex) const
{
    if (isOverSourceColumn(position, sourceIndex))
    {
        targetIndex = -1;
        return sourceIndex >= 0;
    }

    if (isOverGridArea(position, sourceIndex, targetIndex))
    {
        return targetIndex >= 0;
    }

    sourceIndex = -1;
    targetIndex = -1;
    return false;
}

bool RoutingMatrixPanel::isOverSourceColumn(const juce::Point<int>& position, int& trackIndex) const
{
    auto gridStartY = headerHeight + tabHeight;

    if (position.getX() >= 10 && position.getX() < sourceColumnWidth &&
        position.getY() >= gridStartY)
    {
        int numTracks = 8;  // TODO: Get from AudioGraph
        trackIndex = (position.getY() - gridStartY) / gridCellSize;
        if (trackIndex >= 0 && trackIndex < numTracks)
            return true;
    }

    trackIndex = -1;
    return false;
}

bool RoutingMatrixPanel::isOverGridArea(const juce::Point<int>& position, int& trackIndex, int& pluginIndex) const
{
    auto gridStartX = sourceColumnWidth;
    auto gridStartY = headerHeight + tabHeight;
    int numTracks = 8;  // TODO: Get from AudioGraph
    int numPlugins = 8;  // TODO: Get from AudioGraph

    if (position.getX() >= gridStartX && position.getY() >= gridStartY)
    {
        pluginIndex = (position.getX() - gridStartX) / gridCellSize;
        trackIndex = (position.getY() - gridStartY) / gridCellSize;

        if (pluginIndex >= 0 && pluginIndex < numPlugins &&
            trackIndex >= 0 && trackIndex < numTracks)
            return true;
    }

    trackIndex = -1;
    pluginIndex = -1;
    return false;
}

//==============================================================================
juce::ValueTree Connection::saveState() const
{
    juce::ValueTree state("Connection");

    state.setProperty("id", id, nullptr);
    state.setProperty("sourceTrackIndex", sourceTrackIndex, nullptr);
    state.setProperty("targetTrackIndex", targetTrackIndex, nullptr);
    state.setProperty("targetPluginNodeId", static_cast<int>(targetPluginNodeId), nullptr);
    state.setProperty("busIndex", busIndex, nullptr);
    state.setProperty("gain", gain, nullptr);
    state.setProperty("type", static_cast<int>(type), nullptr);
    state.setProperty("enabled", enabled, nullptr);

    return state;
}

void Connection::restoreState(const juce::ValueTree& state)
{
    if (!state.isValid())
        return;

    id = state.getProperty("id", -1);
    sourceTrackIndex = state.getProperty("sourceTrackIndex", -1);
    targetTrackIndex = state.getProperty("targetTrackIndex", -1);
    targetPluginNodeId = static_cast<uint32_t>(state.getProperty("targetPluginNodeId", 0).operator int());
    busIndex = state.getProperty("busIndex", 0);
    gain = state.getProperty("gain", 1.0f);
    type = static_cast<ConnectionType>(state.getProperty("type", 0).operator int());
    enabled = state.getProperty("enabled", true);
}
