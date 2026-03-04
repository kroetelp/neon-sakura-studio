#pragma once

/**
 * TrackType - Defines the type of audio engine for a track
 *
 * Sampler: Sample-based playback (drums, one-shots)
 * Wavetable: Wavetable synthesiser engine (synth sounds)
 * Audio: Audio input/recording tracks
 * Plugin: External VST3/AU plugin tracks
 */
enum class TrackType
{
    Sampler,      // Sample-basiert (aktuell)
    Wavetable,    // Wavetable-Synth (neu)
    Audio,        // Audio-Input/Recording
    Plugin        // Externes VST3/AU Plugin
};

/**
 * Helper functions for TrackType
 */
namespace TrackTypeHelpers
{
    inline const char* getTrackTypeName(TrackType type)
    {
        switch (type)
        {
            case TrackType::Sampler: return "Sampler";
            case TrackType::Wavetable: return "Wavetable";
            case TrackType::Audio: return "Audio";
            case TrackType::Plugin: return "Plugin";
            default: return "Unknown";
        }
    }

    inline TrackType trackTypeFromIndex(int index)
    {
        switch (index)
        {
            case 0: return TrackType::Sampler;
            case 1: return TrackType::Wavetable;
            case 2: return TrackType::Audio;
            case 3: return TrackType::Plugin;
            default: return TrackType::Sampler;
        }
    }

    inline int trackTypeToIndex(TrackType type)
    {
        switch (type)
        {
            case TrackType::Sampler: return 0;
            case TrackType::Wavetable: return 1;
            case TrackType::Audio: return 2;
            case TrackType::Plugin: return 3;
            default: return 0;
        }
    }
}
