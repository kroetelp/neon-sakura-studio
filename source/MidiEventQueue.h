#pragma once

/**
 * MidiEventQueue - Lock-free ring buffer for MIDI events
 *
 * Thread-safe communication between UI thread (producer) and audio thread (consumer).
 * Uses juce::AbstractFifo with a fixed-size array for zero heap allocations during operation.
 *
 * Usage:
 * - UI Thread: Call push() to add MIDI events
 * - Audio Thread: Call pop() in a loop to process all pending events
 */

#include <juce_core/juce_core.h>
#include <array>

/**
 * MidiEvent - Represents a single MIDI event in the queue
 */
struct MidiEvent
{
    enum Type : uint8_t
    {
        NoteOn,
        NoteOff,
        Controller,
        PitchBend,
        Aftertouch
    };

    Type type;
    uint8_t channel;      // MIDI channel (1-16)
    uint8_t noteNumber;   // For NoteOn/NoteOff (0-127)
    float velocity;       // For NoteOn (0.0-1.0)
    uint8_t controllerNum; // For Controller events (CC number)
    uint8_t controllerVal; // For Controller events (0-127)
    int16_t pitchBendVal; // For PitchBend (-8192 to 8191)
    float aftertouchVal;  // For Aftertouch (0.0-1.0)

    // Factory methods for creating events
    static MidiEvent noteOn(uint8_t ch, uint8_t note, float vel)
    {
        MidiEvent e{};
        e.type = NoteOn;
        e.channel = ch;
        e.noteNumber = note;
        e.velocity = vel;
        return e;
    }

    static MidiEvent noteOff(uint8_t ch, uint8_t note)
    {
        MidiEvent e{};
        e.type = NoteOff;
        e.channel = ch;
        e.noteNumber = note;
        return e;
    }

    static MidiEvent controller(uint8_t ch, uint8_t num, uint8_t val)
    {
        MidiEvent e{};
        e.type = Controller;
        e.channel = ch;
        e.controllerNum = num;
        e.controllerVal = val;
        return e;
    }

    static MidiEvent pitchBend(uint8_t ch, int16_t val)
    {
        MidiEvent e{};
        e.type = PitchBend;
        e.channel = ch;
        e.pitchBendVal = val;
        return e;
    }

    static MidiEvent aftertouch(uint8_t ch, float val)
    {
        MidiEvent e{};
        e.type = Aftertouch;
        e.channel = ch;
        e.aftertouchVal = val;
        return e;
    }

    // Polyphonic Aftertouch (per-note pressure)
    static MidiEvent polyAftertouch(uint8_t ch, uint8_t note, uint8_t val)
    {
        MidiEvent e{};
        e.type = Aftertouch;  // Reuse Aftertouch type
        e.channel = ch;
        e.noteNumber = note;  // Store note number for poly aftertouch
        e.controllerVal = val; // Store pressure value here (0-127)
        return e;
    }
};

/**
 * MidiEventQueue - Lock-free single-producer single-consumer queue
 *
 * Implementation uses juce::AbstractFifo which provides atomic-free synchronization
 * for the SPSC (Single Producer Single Consumer) pattern through careful memory ordering.
 */
class MidiEventQueue
{
public:
    static constexpr int capacity = 256;  // Large enough for 200Hz polling with safety margin

    MidiEventQueue() = default;

    /**
     * Push an event onto the queue (called from UI thread)
     * @param event The MIDI event to push
     * @return true if successful, false if queue was full
     */
    bool push(const MidiEvent& event)
    {
        // Prepare to write 1 element
        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 == 0 && size2 == 0)
        {
            // Queue is full
            return false;
        }

        // Write to the first available slot
        if (size1 > 0)
        {
            buffer[static_cast<size_t>(start1)] = event;
        }
        else if (size2 > 0)
        {
            buffer[static_cast<size_t>(start2)] = event;
        }

        // Mark the write as complete
        fifo.finishedWrite(1);
        return true;
    }

    /**
     * Pop an event from the queue (called from audio thread)
     * @param event Output parameter for the popped event
     * @return true if an event was popped, false if queue was empty
     */
    bool pop(MidiEvent& event)
    {
        // Prepare to read 1 element
        int start1, size1, start2, size2;
        fifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 == 0 && size2 == 0)
        {
            // Queue is empty
            return false;
        }

        // Read from the first available slot
        if (size1 > 0)
        {
            event = buffer[static_cast<size_t>(start1)];
        }
        else if (size2 > 0)
        {
            event = buffer[static_cast<size_t>(start2)];
        }

        // Mark the read as complete
        fifo.finishedRead(1);
        return true;
    }

    /**
     * Pop all available events into a vector (convenience method)
     * Note: This method allocates memory, so only use when necessary
     * @param events Vector to fill with events
     */
    void popAll(std::vector<MidiEvent>& events)
    {
        events.clear();

        int start1, size1, start2, size2;
        int numReady = fifo.getNumReady();

        if (numReady == 0)
            return;

        fifo.prepareToRead(numReady, start1, size1, start2, size2);

        events.reserve(static_cast<size_t>(size1 + size2));

        for (int i = 0; i < size1; ++i)
            events.push_back(buffer[static_cast<size_t>(start1 + i)]);

        for (int i = 0; i < size2; ++i)
            events.push_back(buffer[static_cast<size_t>(start2 + i)]);

        fifo.finishedRead(numReady);
    }

    /**
     * Get the number of events available to read
     * @return Number of events in the queue
     */
    int getNumAvailable() const
    {
        return fifo.getNumReady();
    }

    /**
     * Get the remaining capacity of the queue
     * @return Number of events that can still be pushed
     */
    int getRemainingCapacity() const
    {
        return fifo.getFreeSpace();
    }

    /**
     * Reset the queue (not thread-safe - only call when both threads are stopped)
     */
    void reset()
    {
        fifo.reset();
    }

private:
    juce::AbstractFifo fifo{capacity};
    std::array<MidiEvent, static_cast<size_t>(capacity)> buffer{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiEventQueue)
};
