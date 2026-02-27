#pragma once

#include "WavetableSynth.h"
#include "../Modulation/ModulationSource.h"
#include "../MidiEventQueue.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>
#include <array>
#include <atomic>

/**
 * WavetableEngine - Integration layer between WavetableSynth and the main audio engine
 *
 * Supports two modes:
 * 1. Standalone - Generates audio directly (like a regular synth)
 * 2. Modulator - Modulates existing drum samples in tracks
 */
class WavetableEngine
{
public:
    enum class Mode
    {
        Standalone,     // Synth produces audio
        Modulator       // Modulates track samples
    };

    WavetableEngine();
    ~WavetableEngine() = default;

    // Setup
    void prepareToPlay(int samplesPerBlock, double sampleRate);
    void releaseResources();

    // Mode control
    void setMode(Mode mode) { currentMode = mode; }
    Mode getMode() const { return currentMode.load(); }

    // Audio processing - Standalone mode
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    // Modulator mode - Get modulation values for tracks
    float getModulationValue(int trackIndex, ModulationTarget target) const;
    bool isModulationEnabled(int trackIndex) const;
    void setModulationEnabled(int trackIndex, bool enabled);

    // Access to the synth for UI
    WavetableSynth& getSynthesiser() { return synth; }
    const WavetableSynth& getSynthesiser() const { return synth; }

    // BPM for LFO tempo sync
    void setBPM(float bpm);

    // MIDI input for standalone mode
    void handleMidiEvent(const juce::MidiMessage& message);

    // Note on/off for testing/keyboard (now with dynamic channel)
    void noteOn(int channel, int midiNote, float velocity);
    void noteOff(int channel, int midiNote);

    // Master volume
    void setMasterVolume(float volume) { masterVolume.store(volume); }
    float getMasterVolume() const { return masterVolume.load(); }

    /**
     * Set the MIDI event queue for lock-free thread communication
     * The queue is read by the audio thread during processBlock()
     */
    void setMidiEventQueue(MidiEventQueue* queue) { midiEventQueue = queue; }
    MidiEventQueue* getMidiEventQueue() const { return midiEventQueue; }

private:
    WavetableSynth synth;

    std::atomic<Mode> currentMode{Mode::Standalone};
    std::atomic<float> masterVolume{0.8f};
    std::atomic<float> currentBPM{120.0f};

    double sampleRate = 44100.0;

    // Per-track modulation enable
    std::array<std::atomic<bool>, 8> trackModulationEnabled;

    // Per-track modulation values cache
    mutable std::array<float, static_cast<int>(ModulationTarget::Count)> cachedModulationValues{};

    // Lock-free MIDI event queue (owned by WootingManager, read by audio thread)
    MidiEventQueue* midiEventQueue = nullptr;

    // Process MIDI events from the lock-free queue into the MidiBuffer
    void processMidiEventsFromQueue(juce::MidiBuffer& midiBuffer);
};
