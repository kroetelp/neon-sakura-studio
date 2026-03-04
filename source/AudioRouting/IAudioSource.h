#pragma once

/**
 * IAudioSource - Unified interface for all audio sources in Neon Sakura Studio
 *
 * This interface provides a common abstraction for:
 *   - Internal synthesizers (WavetableEngine, etc.)
 *   - Track processors
 *   - VST/AU plugins
 *   - Audio file players
 *   - Step sequencer clips
 *
 * Design Goals:
 *   - Real-time safe: No allocations in processBlock()
 *   - Latency aware: Supports delay compensation
 *   - Tail aware: Handles reverb tails and release times
 *   - Double precision ready: For high-quality processing
 *
 * Thread Safety:
 *   - prepareToPlay() and releaseResources() are called from UI thread
 *   - processBlock() is called from audio thread
 *   - Getters are thread-safe (should return atomic or const values)
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <string>

class IAudioSource
{
public:
    virtual ~IAudioSource() = default;

    // ============================================================
    // Core Audio Processing
    // ============================================================

    /** Process audio and MIDI data.
        Called from the audio thread - must be real-time safe!
        No heap allocations, no locks, no blocking operations.

        @param buffer    Audio buffer to fill (input/output)
        @param midi      MIDI buffer for note events (input/output)
    */
    virtual void processBlock(juce::AudioBuffer<float>& buffer,
                              juce::MidiBuffer& midi) = 0;

    /** Prepare the source for playback.
        Called before processing starts. Allocate buffers here.

        @param sampleRate     Target sample rate
        @param maxBlockSize   Maximum expected block size
    */
    virtual void prepareToPlay(double sampleRate, int maxBlockSize) = 0;

    /** Release resources allocated in prepareToPlay().
        Called when playback stops or settings change.
    */
    virtual void releaseResources() = 0;

    // ============================================================
    // Latency & Tail Support (for Delay Compensation)
    // ============================================================

    /** Returns the latency in samples introduced by this source.
        Used for automatic delay compensation across tracks.

        Examples:
        - Lookahead compressor: latency = lookahead samples
        - FFT-based effect: latency = FFT size / 2
        - Simple filter: latency = 0

        @return Latency in samples (default: 0)
    */
    virtual int getLatencySamples() const { return 0; }

    /** Returns the tail time in seconds.
        Used to know how long to keep processing after playback stops.

        Examples:
        - Reverb: 2-5 seconds depending on settings
        - Delay: Depends on feedback and tempo sync
        - Synth: Release envelope time
        - Compressor: Usually 0

        @return Tail time in seconds (default: 0.0)
    */
    virtual double getTailSeconds() const { return 0.0; }

    // ============================================================
    // Double Precision Support (Optional)
    // ============================================================

    /** Returns true if this source supports 64-bit float processing.
        Some high-quality plugins benefit from double precision.

        @return true if double precision is supported (default: false)
    */
    virtual bool supportsDoublePrecision() const { return false; }

    /** Process audio with double precision (optional).
        Only called if supportsDoublePrecision() returns true.

        @param buffer    Audio buffer in double precision
        @param midi      MIDI buffer for note events
    */
    virtual void processBlockDouble(juce::AudioBuffer<double>& /*buffer*/,
                                    juce::MidiBuffer& /*midi*/)
    {
        // Default: do nothing, must be overridden if supportsDoublePrecision() is true
    }

    // ============================================================
    // Identification
    // ============================================================

    /** Returns the name of this audio source.
        Used for UI display and debugging.
        Note: Returns const juce::String to match JUCE's AudioProcessor::getName()

        @return Display name
    */
    virtual const juce::String getName() const = 0;

    // ============================================================
    // State Management (Optional)
    // ============================================================

    /** Returns true if this source has a non-empty state to save.
        Used to optimize project file size.

        @return true if state should be saved (default: false)
    */
    virtual bool hasState() const { return false; }

    /** Save state to XML for project persistence.
        Only called if hasState() returns true.

        @return XML element containing state
    */
    virtual std::unique_ptr<juce::XmlElement> saveState() const { return nullptr; }

    /** Restore state from XML.
        Called during project load.

        @param xml    XML element containing saved state
    */
    virtual void loadState(const juce::XmlElement& /*xml*/) {}

    // ============================================================
    // Bypass Support
    // ============================================================

    /** Returns true if this source is currently bypassed.
        Bypassed sources should pass audio through unprocessed.

        @return true if bypassed (default: false)
    */
    virtual bool isBypassed() const { return false; }

    /** Set bypass state.
        Called from UI thread when user enables/disables the source.

        @param bypass    true to bypass processing
    */
    virtual void setBypassed(bool /*bypass*/) {}

protected:
    // Protected constructor - this is an interface
    IAudioSource() = default;

    // Prevent copying
    IAudioSource(const IAudioSource&) = delete;
    IAudioSource& operator=(const IAudioSource&) = delete;
};
