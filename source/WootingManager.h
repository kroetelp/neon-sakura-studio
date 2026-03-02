#pragma once

/**
 * WootingManager - Handles analog keyboard input from Wooting keyboards
 *
 * Architecture:
 * - Dedicated hardware thread (juce::Thread) polling at 1000Hz
 * - Lock-free queue for MIDI events (no mutex, no audio dropouts)
 * - UI thread only reads status for display purposes
 *
 * Thread Safety:
 * - Hardware thread writes to midiQueue (producer)
 * - Audio thread reads from midiQueue (consumer)
 * - Atomic variables for UI status display
 */

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <unordered_map>
#include "MidiEventQueue.h"

// Forward declaration of Wooting SDK functions
extern "C" {
    int wooting_analog_initialise(void);
    void wooting_analog_uninitialise(void);
    int wooting_analog_read_full_buffer(unsigned short* code_buffer, float* analog_buffer, int len);
}

class WootingManager : public juce::Thread
{
public:
    WootingManager()
        : juce::Thread("WootingHardwareThread")
        , sdkInitialized(false)
    {
        // Initialize Wooting SDK
        int result = wooting_analog_initialise();
        if (result >= 0)
        {
            sdkInitialized = true;
            juce::Logger::writeToLog("Wooting Analog SDK initialized - starting 1000Hz hardware thread");
            startThread(juce::Thread::Priority::highest);
        }
        else
        {
            juce::Logger::writeToLog("Wooting Analog SDK not found - keyboard features disabled");
        }
    }

    ~WootingManager() override
    {
        signalThreadShouldExit();
        stopThread(100);

        if (sdkInitialized)
        {
            wooting_analog_uninitialise();
        }
    }

    MidiEventQueue& getMidiQueue() { return midiQueue; }
    const MidiEventQueue& getMidiQueue() const { return midiQueue; }

    bool isConnected() const { return sdkInitialized.load(std::memory_order_relaxed); }
    int getNumActiveKeys() const { return numActiveKeys.load(std::memory_order_relaxed); }

    void setVelocityCurve(int curve)
    {
        velocityCurve.store(juce::jlimit(0, 2, curve), std::memory_order_relaxed);
    }

    int getVelocityCurve() const
    {
        return velocityCurve.load(std::memory_order_relaxed);
    }

    void setPressureCurve(int curve)
    {
        pressureCurve.store(juce::jlimit(0, 2, curve), std::memory_order_relaxed);
    }

    int getPressureCurve() const
    {
        return pressureCurve.load(std::memory_order_relaxed);
    }

    void setOctaveOffset(int offset)
    {
        octaveOffset.store(juce::jlimit(-2, 2, offset), std::memory_order_relaxed);
    }

protected:
    void run() override
    {
        const int sleepMs = 1;  // 1000Hz polling

        while (!threadShouldExit())
        {
            pollKeyboard();
            wait(sleepMs);
        }
    }

private:
    MidiEventQueue midiQueue;

    std::atomic<bool> sdkInitialized{false};
    std::atomic<int> numActiveKeys{0};
    std::atomic<int> velocityCurve{0};
    std::atomic<int> pressureCurve{0};  // 0=Linear, 1=Soft, 2=Hard
    std::atomic<int> octaveOffset{0};

    // Thread-local state
    std::unordered_map<unsigned short, float> activeKeys;

    void pollKeyboard()
    {
        if (!sdkInitialized.load(std::memory_order_relaxed))
            return;

        constexpr int maxKeys = 32;
        unsigned short codeBuffer[maxKeys];
        float analogBuffer[maxKeys];

        int keysPressed = wooting_analog_read_full_buffer(codeBuffer, analogBuffer, maxKeys);

        std::unordered_map<unsigned short, float> currentFrameKeys;

        if (keysPressed > 0)
        {
            for (int i = 0; i < keysPressed; ++i)
            {
                unsigned short hidCode = codeBuffer[i];
                float analogValue = analogBuffer[i];
                currentFrameKeys[hidCode] = analogValue;

                int midiNote = mapHIDToMidi(hidCode);
                if (midiNote < 0) continue;

                midiNote += octaveOffset.load(std::memory_order_relaxed) * 12;

                bool wasActive = (activeKeys.find(hidCode) != activeKeys.end());

                // NEW KEY PRESS
                if (!wasActive && analogValue > 0.03f)
                {
                    float velocity = calculateVelocity(analogValue);
                    midiQueue.push(MidiEvent::noteOn(1, static_cast<uint8_t>(midiNote), velocity));
                    activeKeys[hidCode] = analogValue;
                }
                // KEY HELD - send aftertouch on significant change
                else if (wasActive)
                {
                    float prevValue = activeKeys[hidCode];
                    if (std::abs(analogValue - prevValue) > 0.02f)
                    {
                        // Apply pressure curve to aftertouch
                        float pressure = calculatePressure(analogValue);
                        uint8_t aftertouchVal = static_cast<uint8_t>(pressure * 127.0f);
                        midiQueue.push(MidiEvent::polyAftertouch(1, static_cast<uint8_t>(midiNote), aftertouchVal));
                        activeKeys[hidCode] = analogValue;
                    }
                }
            }
        }

        // CHECK FOR KEY RELEASES
        for (auto it = activeKeys.begin(); it != activeKeys.end(); )
        {
            unsigned short hidCode = it->first;
            bool stillPressed = (currentFrameKeys.find(hidCode) != currentFrameKeys.end());
            float currentValue = stillPressed ? currentFrameKeys[hidCode] : 0.0f;

            if (!stillPressed || currentValue <= 0.02f)
            {
                int midiNote = mapHIDToMidi(hidCode);
                if (midiNote >= 0)
                {
                    midiNote += octaveOffset.load(std::memory_order_relaxed) * 12;
                    midiQueue.push(MidiEvent::noteOff(1, static_cast<uint8_t>(midiNote)));
                }
                it = activeKeys.erase(it);
            }
            else
            {
                ++it;
            }
        }

        numActiveKeys.store(static_cast<int>(activeKeys.size()), std::memory_order_relaxed);
    }

    float calculateVelocity(float analogValue)
    {
        int curve = velocityCurve.load(std::memory_order_relaxed);
        analogValue = juce::jmax(0.1f, analogValue);

        switch (curve)
        {
            case 1:  return juce::jlimit(0.1f, 1.0f, std::sqrt(analogValue));
            case 2:  return juce::jlimit(0.1f, 1.0f, analogValue * analogValue);
            default: return juce::jlimit(0.1f, 1.0f, analogValue);
        }
    }

    float calculatePressure(float analogValue)
    {
        int curve = pressureCurve.load(std::memory_order_relaxed);

        switch (curve)
        {
            case 1:  // Soft curve - more response at low pressure
                return juce::jlimit(0.0f, 1.0f, std::sqrt(analogValue));
            case 2:  // Hard curve - need more force for full value
                return juce::jlimit(0.0f, 1.0f, analogValue * analogValue);
            default: // Linear
                return juce::jlimit(0.0f, 1.0f, analogValue);
        }
    }

    int mapHIDToMidi(unsigned short hidCode)
    {
        // HID Usage ID mapping for keyboard
        // Middle row = C4-E5 (main playing position)
        switch (hidCode)
        {
            // Middle row: A S D F G H J K L
            case 0x04: return 60; // A -> C4
            case 0x16: return 62; // S -> D4
            case 0x07: return 64; // D -> E4
            case 0x09: return 65; // F -> F4
            case 0x0A: return 67; // G -> G4
            case 0x0B: return 69; // H -> A4
            case 0x0D: return 71; // J -> B4
            case 0x0E: return 72; // K -> C5
            case 0x0F: return 74; // L -> D5

            // Top row (black keys): W E T Y U I O P
            case 0x1A: return 61; // W -> C#4
            case 0x08: return 63; // E -> D#4
            case 0x17: return 66; // T -> F#4
            case 0x1C: return 68; // Y -> G#4 (Z on QWERTZ)
            case 0x18: return 70; // U -> A#4
            case 0x19: return 73; // I -> C#5
            case 0x22: return 75; // P -> D#5

            // Extended top row
            case 0x23: return 76; // [ -> E5
            case 0x11: return 77; // ] -> F5

            // Bottom row (bass): Z X C V B N M
            case 0x1D: return 48; // Z -> C3
            case 0x1B: return 50; // X -> D3
            case 0x06: return 52; // C -> E3
            case 0x05: return 53; // V -> F3
            case 0x10: return 55; // N -> G3
            case 0x36: return 57; // M -> A3

            default: return -1;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WootingManager)
};
