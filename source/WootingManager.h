#pragma once

/**
 * WootingManager - Handles analog keyboard input from Wooting keyboards
 *
 * Thread Safety:
 * - Runs on UI thread (juce::Timer at 200Hz)
 * - Pushes MIDI events to a lock-free queue instead of directly calling the audio engine
 * - The audio thread reads from the queue to process events safely
 */

#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include "MidiEventQueue.h"

// Header aus der Wooting SDK
#include "wooting_analog_wrapper.h"
#include <map>

class WootingManager : public juce::Timer
{
public:
    WootingManager()
    {
        // Initialisiere das Wooting SDK
        int result = wooting_analog_initialise();
        if (result >= 0)
        {
            juce::Logger::writeToLog("Wooting Analog SDK erfolgreich initialisiert!");
            // Timer starten: alle 5 Millisekunden die Tasten abfragen (200 Hz)
            startTimer(5);
        }
        else
        {
            juce::Logger::writeToLog("Fehler beim Initialisieren der Wooting SDK.");
        }
    }

    ~WootingManager() override
    {
        stopTimer();
        wooting_analog_uninitialise();
    }

    /**
     * Get the MIDI event queue for the audio thread to read from
     */
    MidiEventQueue& getMidiQueue() { return midiQueue; }
    const MidiEventQueue& getMidiQueue() const { return midiQueue; }

    void timerCallback() override
    {
        // Puffer für bis zu 16 gleichzeitig gedrückte Tasten
        const int maxKeys = 16;
        unsigned short code_buffer[maxKeys];
        float analog_buffer[maxKeys];

        // Lese die aktuellen analogen Werte aller gedrückten Tasten
        int keysPressed = wooting_analog_read_full_buffer(code_buffer, analog_buffer, maxKeys);

        // Speichere, welche Tasten in DIESEM Frame gedrückt sind
        std::map<unsigned short, float> currentFrameKeys;

        if (keysPressed > 0)
        {
            for (int i = 0; i < keysPressed; ++i)
            {
                unsigned short hidCode = code_buffer[i];
                float analogValue = analog_buffer[i]; // Wert zwischen 0.0 und 1.0
                currentFrameKeys[hidCode] = analogValue;

                int midiNote = mapHIDToMidi(hidCode);
                if (midiNote < 0) continue; // Taste hat keine MIDI-Note zugewiesen

                // TASTE WIRD FRISCH GEDRÜCKT
                if (!activeKeys.count(hidCode) && analogValue > 0.05f)
                {
                    // Berechne Velocity aus dem initialen Sprung
                    // Je schneller/tiefer der erste registrierte Wert, desto lauter
                    float velocity = juce::jlimit(0.1f, 1.0f, analogValue * 2.5f);

                    // LOCK-FREE: Push to queue instead of direct engine call
                    midiQueue.push(MidiEvent::noteOn(1, static_cast<uint8_t>(midiNote), velocity));
                    activeKeys[hidCode] = analogValue;
                }
                // TASTE WIRD GEHALTEN (AFTERTOUCH / MODULATION)
                else if (activeKeys.count(hidCode))
                {
                    // Wenn der Wert sich ändert, sende Aftertouch/Mod-Wheel
                    int aftertouchVal = static_cast<int>(analogValue * 127.0f);

                    // LOCK-FREE: Push Mod-Wheel CC1 to queue
                    midiQueue.push(MidiEvent::controller(1, 1, static_cast<uint8_t>(aftertouchVal)));

                    activeKeys[hidCode] = analogValue;
                }
            }
        }

        // TASTEN LOSLASSEN PRÜFEN
        for (auto it = activeKeys.begin(); it != activeKeys.end(); )
        {
            unsigned short hidCode = it->first;

            if (currentFrameKeys.find(hidCode) == currentFrameKeys.end() || currentFrameKeys[hidCode] <= 0.05f)
            {
                int midiNote = mapHIDToMidi(hidCode);
                if (midiNote >= 0)
                {
                    // LOCK-FREE: Push note off to queue
                    midiQueue.push(MidiEvent::noteOff(1, static_cast<uint8_t>(midiNote)));
                }
                it = activeKeys.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    // Lock-free queue for MIDI events (UI thread writes, audio thread reads)
    MidiEventQueue midiQueue;

    // Speichert den Status der Tasten (HID Code -> Analoger Wert)
    std::map<unsigned short, float> activeKeys;

    // Mapping von PC-Tastatur auf MIDI-Noten (z.B. mittlere Tastenreihe)
    int mapHIDToMidi(unsigned short hidCode)
    {
        switch (hidCode)
        {
            case 0x04: return 60; // 'A' -> C4
            case 0x1A: return 61; // 'W' -> C#4
            case 0x16: return 62; // 'S' -> D4
            case 0x08: return 63; // 'E' -> D#4
            case 0x07: return 64; // 'D' -> E4
            case 0x09: return 65; // 'F' -> F4
            case 0x17: return 66; // 'T' -> F#4
            case 0x0A: return 67; // 'G' -> G4
            case 0x1C: return 68; // 'Y' / 'Z' -> G#4
            case 0x0B: return 69; // 'H' -> A4
            case 0x18: return 70; // 'U' -> A#4
            case 0x0D: return 71; // 'J' -> B4
            case 0x0E: return 72; // 'K' -> C5
            case 0x0F: return 74; // 'L' -> D5
            default: return -1;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WootingManager)
};
