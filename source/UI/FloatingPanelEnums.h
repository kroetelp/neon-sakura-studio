#pragma once

/**
 * FloatingPanelEnums - Enums für Floating Panels
 *
 * Diese Datei wird von FloatingPanelBase.h inkludiert
 * und kann auch von anderen Klassen inkludiert werden, ohne
 * Zirkuläre Abhängigkeiten zu erzeugen.
 */

// ============================================================================
// Neue Enums für Floating Panels
// ============================================================================

/**
 * PanelSnapZone - Zu welcher Zone kann ein Panel gesnappt werden?
 */
enum class PanelSnapZone
{
    None,           // Free floating, nicht gesnappt
    SynthesisLeft,  // Linke Zone: Wavetable, Melody, Rhythm
    MixingRight,    // Rechte Zone: Plugin Chains, Routing, Master
    SequencingCenter, // Center-Untere Zone: Timeline, Pattern, Track
    TransportTop    // Obere Zone: Transport Controls
};

/**
 * PanelState - Erweiterte States für Floating Panels
 */
enum class PanelState
{
    Floating,        // Vollständig gefloatet
    Snapped,        // An Zone gesnappt
    Minimized,       // Minimiert (nur Header sichtbar)
    Collapsed,       // Auf Symbol reduziert
    Hidden            // Komplett versteckt
};

/**
 * PanelSizeMode - Verschiedene Panel-Größen
 */
enum class PanelSizeMode
{
    Compact,         // Klein, nur essentielle Controls
    Standard,        // Standard-Größe
    Expanded,        // Vollständige Features
    Full             // Maximiert in Zone
};

/**
 * SynthMode - Synthesis Workspace Modi
 */
enum class SynthMode
{
    Perform,         // Large Display, optimiert für Live-Performance
    Design,          // Vollständiger Editor-Access mit allen Controls
    Patch            // Minimiert, Preset-Browser hat Fokus
};
