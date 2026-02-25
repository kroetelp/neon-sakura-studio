#pragma once

/**
 * TrackType - Defines the type of audio engine for a track
 *
 * Sampler: Sample-based playback (drums, one-shots)
 * Wavetable: Wavetable synthesiser engine (synth sounds)
 */
enum class TrackType
{
    Sampler,      // Sample-basiert (aktuell)
    Wavetable     // Wavetable-Synth (neu)
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
            default: return "Unknown";
        }
    }

    inline TrackType trackTypeFromIndex(int index)
    {
        switch (index)
        {
            case 0: return TrackType::Sampler;
            case 1: return TrackType::Wavetable;
            default: return TrackType::Sampler;
        }
    }

    inline int trackTypeToIndex(TrackType type)
    {
        switch (type)
        {
            case TrackType::Sampler: return 0;
            case TrackType::Wavetable: return 1;
            default: return 0;
        }
    }
}
