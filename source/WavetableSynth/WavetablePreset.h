#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

/**
 * WavetablePreset - Complete preset data structure for the Wavetable Synthesiser
 *
 * Stores all synthesiser parameters including:
 * - 3 main oscillators + sub oscillator
 * - Filter settings
 * - Amp envelope
 * - 4 LFOs
 * - Modulation routings
 * - Master volume
 */
struct WavetablePreset
{
    // Metadata
    juce::String name = "Init";
    juce::String author = "";
    juce::String description = "";

    // Oscillator Parameters
    struct OscParams
    {
        float level = 0.8f;
        float morph = 0.0f;
        float detune = 0.0f;
        int unisonCount = 1;
        float panSpread = 0.0f;
        float pan = 0.0f;
        float pitchOffset = 0.0f;
        int wavetableIndex = 0;

        juce::DynamicObject::Ptr toJSON() const
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("level", level);
            obj->setProperty("morph", morph);
            obj->setProperty("detune", detune);
            obj->setProperty("unisonCount", unisonCount);
            obj->setProperty("panSpread", panSpread);
            obj->setProperty("pan", pan);
            obj->setProperty("pitchOffset", pitchOffset);
            obj->setProperty("wavetableIndex", wavetableIndex);
            return obj;
        }

        static OscParams fromJSON(const juce::DynamicObject* obj)
        {
            OscParams params;
            if (obj)
            {
                params.level = static_cast<float>(obj->getProperty("level"));
                params.morph = static_cast<float>(obj->getProperty("morph"));
                params.detune = static_cast<float>(obj->getProperty("detune"));
                params.unisonCount = static_cast<int>(obj->getProperty("unisonCount"));
                params.panSpread = static_cast<float>(obj->getProperty("panSpread"));
                params.pan = static_cast<float>(obj->getProperty("pan"));
                params.pitchOffset = static_cast<float>(obj->getProperty("pitchOffset"));
                params.wavetableIndex = static_cast<int>(obj->getProperty("wavetableIndex"));
            }
            return params;
        }
    };

    std::array<OscParams, 3> oscParams;

    // Sub oscillator parameters
    struct SubOscParams
    {
        float level = 0.0f;
        int octave = 1;
        int waveform = 0;  // 0 = Sine, 1 = Square, 2 = Saw, 3 = Triangle

        juce::DynamicObject::Ptr toJSON() const
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("level", level);
            obj->setProperty("octave", octave);
            obj->setProperty("waveform", waveform);
            return obj;
        }

        static SubOscParams fromJSON(const juce::DynamicObject* obj)
        {
            SubOscParams params;
            if (obj)
            {
                params.level = static_cast<float>(obj->getProperty("level"));
                params.octave = static_cast<int>(obj->getProperty("octave"));
                params.waveform = static_cast<int>(obj->getProperty("waveform"));
            }
            return params;
        }
    } subOscParams;

    // Filter Parameters
    struct FilterParams
    {
        float cutoff = 20000.0f;
        float resonance = 0.0f;
        float drive = 0.0f;
        int type = 0;  // 0 = LP, 1 = HP, 2 = BP, 3 = Notch

        juce::DynamicObject::Ptr toJSON() const
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("cutoff", cutoff);
            obj->setProperty("resonance", resonance);
            obj->setProperty("drive", drive);
            obj->setProperty("type", type);
            return obj;
        }

        static FilterParams fromJSON(const juce::DynamicObject* obj)
        {
            FilterParams params;
            if (obj)
            {
                params.cutoff = static_cast<float>(obj->getProperty("cutoff"));
                params.resonance = static_cast<float>(obj->getProperty("resonance"));
                params.drive = static_cast<float>(obj->getProperty("drive"));
                params.type = static_cast<int>(obj->getProperty("type"));
            }
            return params;
        }
    } filterParams;

    // Amp Envelope Parameters
    struct EnvelopeParams
    {
        float attack = 0.01f;
        float decay = 0.3f;
        float sustain = 0.7f;
        float release = 0.2f;

        juce::DynamicObject::Ptr toJSON() const
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("attack", attack);
            obj->setProperty("decay", decay);
            obj->setProperty("sustain", sustain);
            obj->setProperty("release", release);
            return obj;
        }

        static EnvelopeParams fromJSON(const juce::DynamicObject* obj)
        {
            EnvelopeParams params;
            if (obj)
            {
                params.attack = static_cast<float>(obj->getProperty("attack"));
                params.decay = static_cast<float>(obj->getProperty("decay"));
                params.sustain = static_cast<float>(obj->getProperty("sustain"));
                params.release = static_cast<float>(obj->getProperty("release"));
            }
            return params;
        }
    } ampEnvelope;

    // LFO Parameters
    struct LFOParams
    {
        float rate = 1.0f;
        float depth = 0.5f;
        int waveform = 0;      // 0 = Sine, 1 = Triangle, 2 = Saw, 3 = Square, 4 = S&H
        bool tempoSync = false;
        int syncRate = 0;      // 0 = 1/4, 1 = 1/8, 2 = 1/16, 3 = 1/32, etc.
        bool bipolar = true;

        juce::DynamicObject::Ptr toJSON() const
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("rate", rate);
            obj->setProperty("depth", depth);
            obj->setProperty("waveform", waveform);
            obj->setProperty("tempoSync", tempoSync);
            obj->setProperty("syncRate", syncRate);
            obj->setProperty("bipolar", bipolar);
            return obj;
        }

        static LFOParams fromJSON(const juce::DynamicObject* obj)
        {
            LFOParams params;
            if (obj)
            {
                params.rate = static_cast<float>(obj->getProperty("rate"));
                params.depth = static_cast<float>(obj->getProperty("depth"));
                params.waveform = static_cast<int>(obj->getProperty("waveform"));
                params.tempoSync = static_cast<bool>(obj->getProperty("tempoSync"));
                params.syncRate = static_cast<int>(obj->getProperty("syncRate"));
                params.bipolar = static_cast<bool>(obj->getProperty("bipolar"));
            }
            return params;
        }
    };

    std::array<LFOParams, 4> lfoParams;

    // Modulation Routing
    struct ModRouting
    {
        int source = 0;      // ModulationSource enum value
        int target = 0;      // ModulationTarget enum value
        float amount = 0.0f;
        bool bipolar = true;

        juce::DynamicObject::Ptr toJSON() const
        {
            auto obj = new juce::DynamicObject();
            obj->setProperty("source", source);
            obj->setProperty("target", target);
            obj->setProperty("amount", amount);
            obj->setProperty("bipolar", bipolar);
            return obj;
        }

        static ModRouting fromJSON(const juce::DynamicObject* obj)
        {
            ModRouting routing;
            if (obj)
            {
                routing.source = static_cast<int>(obj->getProperty("source"));
                routing.target = static_cast<int>(obj->getProperty("target"));
                routing.amount = static_cast<float>(obj->getProperty("amount"));
                routing.bipolar = static_cast<bool>(obj->getProperty("bipolar"));
            }
            return routing;
        }
    };

    juce::Array<ModRouting> modRoutings;

    // Master volume
    float masterVolume = 0.8f;

    // JSON Serialization
    juce::String toJSON() const
    {
        auto* root = new juce::DynamicObject();

        // Metadata
        root->setProperty("name", name);
        root->setProperty("author", author);
        root->setProperty("description", description);
        root->setProperty("presetVersion", 1);

        // Oscillators
        juce::Array<juce::var> oscArray;
        for (const auto& osc : oscParams)
            oscArray.add(juce::var(osc.toJSON()));
        root->setProperty("oscillators", oscArray);

        // Sub oscillator
        root->setProperty("subOsc", juce::var(subOscParams.toJSON()));

        // Filter
        root->setProperty("filter", juce::var(filterParams.toJSON()));

        // Envelope
        root->setProperty("ampEnvelope", juce::var(ampEnvelope.toJSON()));

        // LFOs
        juce::Array<juce::var> lfoArray;
        for (const auto& lfo : lfoParams)
            lfoArray.add(juce::var(lfo.toJSON()));
        root->setProperty("lfos", lfoArray);

        // Modulation routings
        juce::Array<juce::var> modArray;
        for (const auto& mod : modRoutings)
            modArray.add(juce::var(mod.toJSON()));
        root->setProperty("modRoutings", modArray);

        // Master
        root->setProperty("masterVolume", masterVolume);

        return juce::JSON::toString(juce::var(root));
    }

    static WavetablePreset fromJSON(const juce::String& jsonString)
    {
        WavetablePreset preset;

        auto parsed = juce::JSON::parse(jsonString);
        if (auto* root = parsed.getDynamicObject())
        {
            // Metadata
            preset.name = root->getProperty("name").toString();
            preset.author = root->getProperty("author").toString();
            preset.description = root->getProperty("description").toString();

            // Oscillators
            if (auto* oscArray = root->getProperty("oscillators").getArray())
            {
                for (int i = 0; i < juce::jmin(3, oscArray->size()); ++i)
                {
                    if (auto* oscObj = (*oscArray)[i].getDynamicObject())
                        preset.oscParams[i] = OscParams::fromJSON(oscObj);
                }
            }

            // Sub oscillator
            if (auto* subObj = root->getProperty("subOsc").getDynamicObject())
                preset.subOscParams = SubOscParams::fromJSON(subObj);

            // Filter
            if (auto* filterObj = root->getProperty("filter").getDynamicObject())
                preset.filterParams = FilterParams::fromJSON(filterObj);

            // Envelope
            if (auto* envObj = root->getProperty("ampEnvelope").getDynamicObject())
                preset.ampEnvelope = EnvelopeParams::fromJSON(envObj);

            // LFOs
            if (auto* lfoArray = root->getProperty("lfos").getArray())
            {
                for (int i = 0; i < juce::jmin(4, lfoArray->size()); ++i)
                {
                    if (auto* lfoObj = (*lfoArray)[i].getDynamicObject())
                        preset.lfoParams[i] = LFOParams::fromJSON(lfoObj);
                }
            }

            // Modulation routings
            if (auto* modArray = root->getProperty("modRoutings").getArray())
            {
                for (const auto& modVar : *modArray)
                {
                    if (auto* modObj = modVar.getDynamicObject())
                        preset.modRoutings.add(ModRouting::fromJSON(modObj));
                }
            }

            // Master
            preset.masterVolume = static_cast<float>(root->getProperty("masterVolume"));
        }

        return preset;
    }

    // Create default init preset
    static WavetablePreset createInitPreset()
    {
        WavetablePreset preset;
        preset.name = "Init";
        preset.description = "Default init preset";

        // OSC 1 enabled by default
        preset.oscParams[0].level = 0.8f;
        preset.oscParams[1].level = 0.0f;
        preset.oscParams[2].level = 0.0f;

        return preset;
    }
};
