#pragma once

/**
 * ModulationSource - Enum defining all available modulation sources
 * Used by the modulation matrix for routing
 */
enum class ModulationSource
{
    // LFOs
    LFO1,
    LFO2,
    LFO3,
    LFO4,

    // Envelopes
    Envelope1,   // Main amp envelope
    Envelope2,   // Filter envelope
    Envelope3,   // Free envelope

    // MIDI / Performance
    Velocity,
    Aftertouch,
    ModWheel,
    PitchBend,

    // Macros
    Macro1,
    Macro2,
    Macro3,
    Macro4,

    Count
};

/**
 * ModulationTarget - Enum defining all modulation destinations
 */
enum class ModulationTarget
{
    // Oscillator parameters
    Osc1_Pitch,
    Osc1_Level,
    Osc1_Morph,
    Osc1_Pan,
    Osc2_Pitch,
    Osc2_Level,
    Osc2_Morph,
    Osc2_Pan,
    Osc3_Pitch,
    Osc3_Level,
    Osc3_Morph,
    Osc3_Pan,

    // Sub oscillator
    Sub_Level,

    // Filter parameters
    Filter_Cutoff,
    Filter_Resonance,
    Filter_Drive,

    // FM/AM Modulation amounts
    FM_Amount12,
    FM_Amount13,
    FM_Amount23,
    AM_Amount12,
    AM_Amount13,
    AM_Amount23,

    // Waveshaper
    Shaper_Amount,
    Shaper_Mix,

    // Master
    Master_Level,

    // Effects (future)
    FX1_Param,
    FX2_Param,

    Count
};

/**
 * Helper functions for modulation sources and targets
 */
namespace ModulationHelpers
{
    inline const char* getSourceName(ModulationSource source)
    {
        switch (source)
        {
            case ModulationSource::LFO1: return "LFO 1";
            case ModulationSource::LFO2: return "LFO 2";
            case ModulationSource::LFO3: return "LFO 3";
            case ModulationSource::LFO4: return "LFO 4";
            case ModulationSource::Envelope1: return "Env 1";
            case ModulationSource::Envelope2: return "Env 2";
            case ModulationSource::Envelope3: return "Env 3";
            case ModulationSource::Velocity: return "Velocity";
            case ModulationSource::Aftertouch: return "Aftertouch";
            case ModulationSource::ModWheel: return "Mod Wheel";
            case ModulationSource::PitchBend: return "Pitch Bend";
            case ModulationSource::Macro1: return "Macro 1";
            case ModulationSource::Macro2: return "Macro 2";
            case ModulationSource::Macro3: return "Macro 3";
            case ModulationSource::Macro4: return "Macro 4";
            default: return "Unknown";
        }
    }

    inline const char* getTargetName(ModulationTarget target)
    {
        switch (target)
        {
            case ModulationTarget::Osc1_Pitch: return "Osc1 Pitch";
            case ModulationTarget::Osc1_Level: return "Osc1 Level";
            case ModulationTarget::Osc1_Morph: return "Osc1 Morph";
            case ModulationTarget::Osc1_Pan: return "Osc1 Pan";
            case ModulationTarget::Osc2_Pitch: return "Osc2 Pitch";
            case ModulationTarget::Osc2_Level: return "Osc2 Level";
            case ModulationTarget::Osc2_Morph: return "Osc2 Morph";
            case ModulationTarget::Osc2_Pan: return "Osc2 Pan";
            case ModulationTarget::Osc3_Pitch: return "Osc3 Pitch";
            case ModulationTarget::Osc3_Level: return "Osc3 Level";
            case ModulationTarget::Osc3_Morph: return "Osc3 Morph";
            case ModulationTarget::Osc3_Pan: return "Osc3 Pan";
            case ModulationTarget::Sub_Level: return "Sub Level";
            case ModulationTarget::Filter_Cutoff: return "Filter Cutoff";
            case ModulationTarget::Filter_Resonance: return "Filter Res";
            case ModulationTarget::Filter_Drive: return "Filter Drive";
            case ModulationTarget::FM_Amount12: return "FM 1>2";
            case ModulationTarget::FM_Amount13: return "FM 1>3";
            case ModulationTarget::FM_Amount23: return "FM 2>3";
            case ModulationTarget::AM_Amount12: return "AM 1>2";
            case ModulationTarget::AM_Amount13: return "AM 1>3";
            case ModulationTarget::AM_Amount23: return "AM 2>3";
            case ModulationTarget::Shaper_Amount: return "Shaper Amt";
            case ModulationTarget::Shaper_Mix: return "Shaper Mix";
            case ModulationTarget::Master_Level: return "Master Level";
            case ModulationTarget::FX1_Param: return "FX1";
            case ModulationTarget::FX2_Param: return "FX2";
            default: return "Unknown";
        }
    }
}
