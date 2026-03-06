// ============================================================================
// ZoneSnapManager.cpp - Implementierung
// ============================================================================

#include "ZoneSnapManager.h"
#include "FloatingPanelBase.h"
#include "../Theme/ThemeManager.h"
#include <algorithm>

//==============================================================================
// Konstruktor
//==============================================================================
ZoneSnapManager::ZoneSnapManager()
{
    initializeZonePaddings();
    initializeZoneEnabled();
}

//==============================================================================
// Destruktor
//==============================================================================
ZoneSnapManager::~ZoneSnapManager()
{
    clearPanelMappings();
}

//==============================================================================
// Zone Bounds Management
//==============================================================================
void ZoneSnapManager::updateZoneBounds(const juce::Rectangle<int>& containerBounds)
{
    calculateZoneBounds(containerBounds);
}

void ZoneSnapManager::updateZoneBounds(PanelSnapZone zone, const juce::Rectangle<int>& bounds)
{
    zoneBounds[zone] = bounds;
}

juce::Rectangle<int> ZoneSnapManager::getZoneBounds(PanelSnapZone zone) const
{
    auto it = zoneBounds.find(zone);
    if (it != zoneBounds.end())
        return it->second;
    return juce::Rectangle<int>();
}

//==============================================================================
// Zone Detection
//==============================================================================
PanelSnapZone ZoneSnapManager::detectZone(const juce::Point<int>& point) const
{
    // Prüfe jede Zone in Reihenfolge der Priorität
    if (isZoneEnabled(PanelSnapZone::TransportTop) && isPointInZone(point, PanelSnapZone::TransportTop))
        return PanelSnapZone::TransportTop;

    if (isZoneEnabled(PanelSnapZone::SynthesisLeft) && isPointInZone(point, PanelSnapZone::SynthesisLeft))
        return PanelSnapZone::SynthesisLeft;

    if (isZoneEnabled(PanelSnapZone::MixingRight) && isPointInZone(point, PanelSnapZone::MixingRight))
        return PanelSnapZone::MixingRight;

    if (isZoneEnabled(PanelSnapZone::SequencingCenter) && isPointInZone(point, PanelSnapZone::SequencingCenter))
        return PanelSnapZone::SequencingCenter;

    return PanelSnapZone::None;
}

PanelSnapZone ZoneSnapManager::detectZoneForComponent(const juce::Component& component) const
{
    // Prüfe die center-Position des Panels
    juce::Point<int> center = component.getBounds().getCentre();
    return detectZone(center);
}

bool ZoneSnapManager::isPointInZone(const juce::Point<int>& point, PanelSnapZone zone) const
{
    auto it = zoneBounds.find(zone);
    if (it == zoneBounds.end())
        return false;

    return it->second.contains(point);
}

//==============================================================================
// Snapping
//==============================================================================
bool ZoneSnapManager::snapPanelToZone(FloatingPanelBase* panel, PanelSnapZone zone, bool animate)
{
    if (panel == nullptr)
        return false;

    if (!isZoneEnabled(zone))
        return false;

    // Hole die alte Zone
    PanelSnapZone oldZone = getPanelZone(panel);

    // Setze die neue Zone im Panel
    panel->setSnapZone(zone);

    // Aktualisiere die Zuordnung
    panelZoneMap[panel] = zone;

    // Trigger Callback
    triggerZoneChangeCallback(panel, oldZone, zone);

    return true;
}

bool ZoneSnapManager::unsnapPanel(FloatingPanelBase* panel, bool animate)
{
    if (panel == nullptr)
        return false;

    // Hole die alte Zone
    PanelSnapZone oldZone = getPanelZone(panel);

    // Entferne die Zone aus dem Panel
    panel->setSnapZone(PanelSnapZone::None);

    // Entferne die Zuordnung
    panelZoneMap.erase(panel);

    // Trigger Callback
    triggerZoneChangeCallback(panel, oldZone, PanelSnapZone::None);

    return true;
}

PanelSnapZone ZoneSnapManager::getPanelZone(const FloatingPanelBase* panel) const
{
    auto it = panelZoneMap.find(panel);
    if (it != panelZoneMap.end())
        return it->second;
    return PanelSnapZone::None;
}

//==============================================================================
// Zone Configuration
//==============================================================================
void ZoneSnapManager::setZoneEnabled(PanelSnapZone zone, bool enabled)
{
    zoneEnabled[zone] = enabled;
}

bool ZoneSnapManager::isZoneEnabled(PanelSnapZone zone) const
{
    auto it = zoneEnabled.find(zone);
    if (it != zoneEnabled.end())
        return it->second;
    return true; // Standardmäßig aktiviert
}

void ZoneSnapManager::setSnapSensitivity(int sensitivity)
{
    snapSensitivity = juce::jlimit(0, 100, sensitivity);
}

//==============================================================================
// Visual Feedback
//==============================================================================
void ZoneSnapManager::setVisualFeedbackEnabled(bool enabled)
{
    visualFeedbackEnabled = enabled;
}

void ZoneSnapManager::paintZoneFeedback(juce::Graphics& g) const
{
    if (!visualFeedbackEnabled || highlightedZone == PanelSnapZone::None)
        return;

    auto it = zoneBounds.find(highlightedZone);
    if (it == zoneBounds.end())
        return;

    auto bounds = it->second;

    // Zeichne Highlight
    juce::Colour highlightColour;

    switch (highlightedZone)
    {
        case PanelSnapZone::SynthesisLeft:
            highlightColour = juce::Colour(static_cast<juce::uint8>(100), static_cast<juce::uint8>(180), static_cast<juce::uint8>(255), static_cast<juce::uint8>(80)); // Blau
            break;
        case PanelSnapZone::MixingRight:
            highlightColour = juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(150), static_cast<juce::uint8>(50), static_cast<juce::uint8>(80)); // Orange
            break;
        case PanelSnapZone::SequencingCenter:
            highlightColour = juce::Colour(static_cast<juce::uint8>(100), static_cast<juce::uint8>(255), static_cast<juce::uint8>(150), static_cast<juce::uint8>(80)); // Grün
            break;
        case PanelSnapZone::TransportTop:
            highlightColour = juce::Colour(static_cast<juce::uint8>(255), static_cast<juce::uint8>(100), static_cast<juce::uint8>(150), static_cast<juce::uint8>(80)); // Pink
            break;
        default:
            highlightColour = juce::Colour(static_cast<juce::uint8>(150), static_cast<juce::uint8>(150), static_cast<juce::uint8>(150), static_cast<juce::uint8>(80)); // Grau
            break;
    }

    g.setColour(highlightColour);
    g.fillRect(bounds);

    // Zeichne Border
    g.setColour(highlightColour.withAlpha(1.0f));
    g.drawRect(bounds, 2);
}

void ZoneSnapManager::setHighlightedZone(PanelSnapZone zone)
{
    highlightedZone = zone;
}

//==============================================================================
// Zone Padding & Margins
//==============================================================================
void ZoneSnapManager::setZonePadding(int padding)
{
    for (auto& entry : zonePadding)
    {
        entry.second = padding;
    }
}

void ZoneSnapManager::setZonePadding(PanelSnapZone zone, int padding)
{
    zonePadding[zone] = padding;
}

int ZoneSnapManager::getZonePadding(PanelSnapZone zone) const
{
    auto it = zonePadding.find(zone);
    if (it != zonePadding.end())
        return it->second;
    return 8; // Standard-Padding
}

//==============================================================================
// Callbacks
//==============================================================================
void ZoneSnapManager::setZoneChangeCallback(ZoneChangeCallback callback)
{
    zoneChangeCallback = std::move(callback);
}

//==============================================================================
// Utility
//==============================================================================
juce::String ZoneSnapManager::getZoneName(PanelSnapZone zone)
{
    switch (zone)
    {
        case PanelSnapZone::SynthesisLeft:
            return "Synthesis Left";
        case PanelSnapZone::MixingRight:
            return "Mixing Right";
        case PanelSnapZone::SequencingCenter:
            return "Sequencing Center";
        case PanelSnapZone::TransportTop:
            return "Transport Top";
        default:
            return "None";
    }
}

PanelSnapZone ZoneSnapManager::getZoneFromName(const juce::String& name)
{
    if (name == "Synthesis Left")
        return PanelSnapZone::SynthesisLeft;
    if (name == "Mixing Right")
        return PanelSnapZone::MixingRight;
    if (name == "Sequencing Center")
        return PanelSnapZone::SequencingCenter;
    if (name == "Transport Top")
        return PanelSnapZone::TransportTop;
    return PanelSnapZone::None;
}

void ZoneSnapManager::clearPanelMappings()
{
    panelZoneMap.clear();
}

//==============================================================================
// Private Methods
//==============================================================================
void ZoneSnapManager::initializeZonePaddings()
{
    zonePadding[PanelSnapZone::SynthesisLeft] = 8;
    zonePadding[PanelSnapZone::MixingRight] = 8;
    zonePadding[PanelSnapZone::SequencingCenter] = 8;
    zonePadding[PanelSnapZone::TransportTop] = 8;
}

void ZoneSnapManager::initializeZoneEnabled()
{
    zoneEnabled[PanelSnapZone::SynthesisLeft] = true;
    zoneEnabled[PanelSnapZone::MixingRight] = true;
    zoneEnabled[PanelSnapZone::SequencingCenter] = true;
    zoneEnabled[PanelSnapZone::TransportTop] = true;
}

void ZoneSnapManager::calculateZoneBounds(const juce::Rectangle<int>& containerBounds)
{
    if (containerBounds.isEmpty())
        return;

    int width = containerBounds.getWidth();
    int height = containerBounds.getHeight();
    int x = containerBounds.getX();
    int y = containerBounds.getY();

    int transportHeight = height * 0.15; // 15% für Transport
    int sideWidth = width * 0.25;        // 25% für jede Seite

    // Transport Top Zone
    zoneBounds[PanelSnapZone::TransportTop] = juce::Rectangle<int>(
        x + getZonePadding(PanelSnapZone::TransportTop),
        y + getZonePadding(PanelSnapZone::TransportTop),
        width - 2 * getZonePadding(PanelSnapZone::TransportTop),
        transportHeight - 2 * getZonePadding(PanelSnapZone::TransportTop)
    );

    // Synthesis Left Zone
    zoneBounds[PanelSnapZone::SynthesisLeft] = juce::Rectangle<int>(
        x + getZonePadding(PanelSnapZone::SynthesisLeft),
        y + transportHeight + getZonePadding(PanelSnapZone::SynthesisLeft),
        sideWidth - 2 * getZonePadding(PanelSnapZone::SynthesisLeft),
        height - transportHeight - 2 * getZonePadding(PanelSnapZone::SynthesisLeft)
    );

    // Sequencing Center Zone
    zoneBounds[PanelSnapZone::SequencingCenter] = juce::Rectangle<int>(
        x + sideWidth + getZonePadding(PanelSnapZone::SequencingCenter),
        y + transportHeight + getZonePadding(PanelSnapZone::SequencingCenter),
        width - 2 * sideWidth - 2 * getZonePadding(PanelSnapZone::SequencingCenter),
        height - transportHeight - 2 * getZonePadding(PanelSnapZone::SequencingCenter)
    );

    // Mixing Right Zone
    zoneBounds[PanelSnapZone::MixingRight] = juce::Rectangle<int>(
        x + width - sideWidth + getZonePadding(PanelSnapZone::MixingRight),
        y + transportHeight + getZonePadding(PanelSnapZone::MixingRight),
        sideWidth - 2 * getZonePadding(PanelSnapZone::MixingRight),
        height - transportHeight - 2 * getZonePadding(PanelSnapZone::MixingRight)
    );
}

void ZoneSnapManager::triggerZoneChangeCallback(FloatingPanelBase* panel, PanelSnapZone oldZone, PanelSnapZone newZone)
{
    if (zoneChangeCallback)
    {
        zoneChangeCallback(panel, oldZone, newZone);
    }
}
