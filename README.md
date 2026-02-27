# Neon Sakura Studio

[![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-blue.svg)](https://github.com/kroetelp/NeonSakuraStudio)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-orange.svg)](https://en.cppreference.com/w/cpp/20)
[![JUCE](https://img.shields.io/badge/JUCE-8.0.4-green.svg)](https://juce.com/)

**A modern, JUCE-based drum sequencer and wavetable synthesizer with a stunning cyberpunk neon aesthetic.**

> **Keywords:** drum sequencer, wavetable synthesizer, DAW, music production, JUCE, C++20, audio plugin, MIDI, pattern generator, electronic music, techno, house, trap, drum and bass, ambient

![NeonSakuraStudio Screenshot](screenshots/sequencer.png)

---

## Language / Sprache

| Language | Link |
|:--------:|:----:|
| **English** | [You are here](#neon-sakura-studio) |
| **Deutsch** | [Hier klicken](#neon-sakura-studio-deutsch) |

---

## Features

### Drum Sequencer
- **8-Track Drum Sequencer** with 64 steps per track
- **Genre-based Pattern Generation** - Techno, House, Trap, DnB, Ambient, Garage
- **Euclidean Rhythms** for organic, polyrhythmic patterns
- **Melody Generator** with scales, arpeggios, and chord progressions
- **Music Theory Engine** for harmonically correct patterns
- **Swing & Groove** for natural-feeling rhythms
- **Step Modifiers** - Gain, Pitch, Pan, Ratchet, Reverb, Delay, Filter per step
- **Sample Management** with category-based loading

### Wavetable Synthesizer
- **3 Wavetable Oscillators** with frame morphing and unison
- **Sub Oscillator** with multiple waveforms
- **Wavetable Import** - Load your own WAV files or Serum-compatible wavetables
- **Modulation System:**
  - 4 LFOs with various waveforms and tempo sync
  - 3 ADSR envelopes
  - Flexible modulation matrix
- **Filter Section** - Lowpass, Highpass, Bandpass, Notch
- **Preset Management** with factory and user presets
- **Real-time Visualization** of wavetable waveforms
- **Oscilloscope** for audio output monitoring

### Hardware Integration
- **Wooting Analog Keyboard Support** - Use your Wooting keyboard as an expressive MIDI controller with analog velocity
  - Powered by [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/)
  - Full analog key pressure sensitivity
  - Aftertouch and modulation support

### User Interface
- **Neon-inspired Cyberpunk UI** with custom designed aesthetics
- **Serum-inspired Synth Layout** for intuitive sound design
- **NeonSakuraLookAndFeel** - Custom LookAndFeel for consistent design
- **Responsive Panels** for different workflow zones
- **Real-time Visualizations** and animated elements

---

## Screenshots

| Drum Sequencer | Wavetable Synthesizer |
|:--------------:|:---------------------:|
| ![Sequencer](screenshots/sequencer.png) | ![Synth](screenshots/synth.png) |

---

## Requirements

### Dependencies
- **JUCE 8.0.4** (automatically loaded via CMake FetchContent)
- **melatonin_blur** (for UI blur effects, automatically loaded)
- **CMake 3.22+**
- **C++20 compatible compiler**

### Optional
- **Wooting Analog Keyboard** - For analog MIDI input (https://github.com/WootingKb/wooting-analog-sdk/)
- **super-strudel-desktop** - For Strudel/TidalCycles integration (separate repository)

---

## Building

### Windows (Visual Studio 2019/2022)
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### macOS
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Linux
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The executable will be located at:
- **Windows:** `build/NeonSakuraStudio_artefacts/Release/NeonSakuraStudio.exe`
- **macOS/Linux:** `build/NeonSakuraStudio_artefacts/Release/NeonSakuraStudio`

---

## Project Structure

```
source/
├── Main.cpp                    # Application Entry Point
├── MainComponent.h/cpp         # Main GUI and Audio Setup
├── WootingManager.h            # Wooting Analog Keyboard Integration
│
├── Audio/
│   ├── AudioEngine.h/cpp       # Audio Processing and Playback
│   ├── TrackAudioProcessor.h/cpp # Per-track Audio Processing
│   ├── SampleManager.h/cpp     # Sample Management
│   └── PlaybackController.h/cpp # Playback Control
│
├── Core/
│   ├── TrackModel.h/cpp        # Track Data Model
│   ├── TrackManager.h/cpp      # Track Management
│   ├── TrackType.h             # Track Type Definitions
│   ├── PanelManager.h/cpp      # Panel Management
│   └── ITrackDataProvider.h    # Interface for Track Data
│
├── Sequencer/
│   ├── PatternGenerator.h/cpp  # Genre-based Pattern Generation
│   ├── RhythmExplorer.h/cpp    # Rhythmic Exploration UI
│   ├── MusicTheory.h/cpp       # Scales, Chords, Progressions
│   ├── MelodyGenerator.h/cpp   # Melodic Pattern Generation
│   └── MelodyPanel.h/cpp       # Melody Workstation UI
│
├── WavetableSynth/             # Wavetable Synthesizer Engine
│   ├── WavetableData.h/cpp     # Wavetable Data and Loading
│   ├── WavetableOscillator.h/cpp # Oscillator with Unison/Morphing
│   ├── SubOscillator.h/cpp     # Sub Oscillator
│   ├── WavetableFilter.h/cpp   # Filter Implementation
│   ├── WavetableVoice.h/cpp    # Voice with Filter/Envelope
│   ├── WavetableSynth.h/cpp    # Synthesiser with Voices
│   ├── WavetableEngine.h/cpp   # Standalone Engine
│   ├── WavetableParams.h/cpp   # Thread-safe Parameters
│   ├── WavetablePreset.h       # Preset Data Structure
│   └── WavetablePresetManager.h/cpp # Preset Management
│
├── WavetableUI/                # Synthesizer UI
│   ├── WavetableSynthEditor.h/cpp # Main Editor
│   ├── WavetableDisplay.h/cpp  # Wavetable Visualization
│   ├── OscillatorSection.h/cpp # Oscillator Controls
│   ├── FilterSection.h/cpp     # Filter Controls
│   ├── EnvelopeSection.h/cpp   # Envelope Controls
│   ├── ModulationGrid.h/cpp    # Modulation Matrix UI
│   ├── Oscilloscope.h/cpp      # Audio Visualization
│   └── NeonSakuraLookAndFeel   # Custom LookAndFeel
│
└── Modulation/                 # Modulation System
    ├── ModulationMatrix.h/cpp  # Routing System
    ├── ModulationSource.h      # Source Definitions
    ├── Modulator.h/cpp         # Base Class
    ├── LFOModulator.h/cpp      # LFOs
    └── EnvelopeModulator.h/cpp # Envelopes
```

---

## Step Modifiers

Each step in the sequencer can have the following modifiers:

| Modifier | Key | Description |
|----------|-----|-------------|
| Gain | `g` | Per-step volume |
| Pitch | `p` | Pitch shift in semitones |
| Pan | `n` | Stereo position |
| Ratchet | `r` | Rapid repetitions |
| Reverb | `v` | Reverb amount |
| Delay | `d` | Delay amount |
| Filter | `f` | Filter cutoff |

---

## Modulation Matrix

The wavetable synthesizer features a flexible modulation matrix:

**Sources:**
- LFO 1-4 (with multiple waveforms and tempo sync)
- Envelope 1-3 (full ADSR)
- Velocity, ModWheel, PitchBend, Aftertouch
- Macro Knobs 1-4

**Targets:**
- Oscillator Pitch, Wavetable Position, Level
- Filter Cutoff, Resonance
- Pan, Amplifier

---

## Third-Party Libraries

| Library | Purpose | License |
|---------|---------|---------|
| [JUCE](https://juce.com/) | Audio Application Framework | JUCE License |
| [melatonin_blur](https://github.com/sudara/melatonin_blur) | UI Blur Effects | MIT |
| [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/) | Analog Keyboard Input | MIT |

---

## Roadmap

- [ ] VST3/AU Plugin Version
- [ ] More Wavetable Presets
- [ ] MIDI Export
- [ ] More LFO Waveforms
- [ ] Step Sequencer Automation
- [ ] Multi-output Audio

---

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- [JUCE Framework](https://juce.com/) - Amazing audio application framework
- [Wooting](https://wooting.io/) - For the analog keyboard technology
- [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/) - Open source analog input SDK
- [melatonin](https://github.com/sudara/melatonin_blur) - Beautiful blur effects for JUCE

---

<p align="center">
  <i>Neon Sakura Studio - Cyberpunk Music Production</i>
</p>

---

# Neon Sakura Studio (Deutsch)

Ein moderner, JUCE-basierter Drum-Sequencer und Wavetable-Synthesizer mit integriertem Pattern-Generator, Music-Theory-Features und einem atemberaubenden Cyberpunk-Neon-Design.

![NeonSakuraStudio Screenshot](screenshots/sequencer.png)

---

## Features

### Drum Sequencer
- **8-Track Drum Sequencer** mit 64 Steps pro Track
- **Genre-basierte Pattern-Generierung** - Techno, House, Trap, DnB, Ambient, Garage
- **Euklidische Rhythmen** für organische, polyrhythmische Patterns
- **Melodie-Generator** mit Skalen, Arpeggios und Akkord-Fortschreitungen
- **Music-Theory-Engine** für harmonisch korrekte Patterns
- **Swing & Groove** für natürliches Rhythmus-Gefühl
- **Step Modifiers** - Gain, Pitch, Pan, Ratchet, Reverb, Delay, Filter pro Step
- **Sample-Management** mit kategoriebasiertem Laden

### Wavetable Synthesizer
- **3 Wavetable-Oszillatoren** mit Frame-Morphing und Unison
- **Sub-Oszillator** mit verschiedenen Wellenformen
- **Wavetable-Import** - Lade eigene WAV-Dateien oder Serum-kompatible Wavetables
- **Modulationssystem:**
  - 4 LFOs mit verschiedenen Wellenformen und Tempo-Sync
  - 3 ADSR-Hüllenkurven
  - Flexible Modulations-Matrix
- **Filter-Sektion** - Lowpass, Highpass, Bandpass, Notch
- **Preset-Management** mit Factory- und User-Presets
- **Echtzeit-Visualisierung** der Wavetable-Wellenformen
- **Oszilloskop** für Audio-Ausgabe

### Hardware-Integration
- **Wooting Analog Tastatur-Support** - Nutze deine Wooting-Tastatur als ausdrucksstarken MIDI-Controller mit analoger Velocity
  - Unterstützt durch [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/)
  - Volle analoge Tastendruck-Empfindlichkeit
  - Aftertouch und Modulations-Unterstützung

### Benutzeroberfläche
- **Neon-inspiriertes Cyberpunk-UI** mit maßgeschneidertem Design
- **Serum-inspiriertes Synth-Layout** für intuitives Sound-Design
- **NeonSakuraLookAndFeel** - Custom LookAndFeel für konsistentes Design
- **Responsive Panels** für verschiedene Arbeitsbereiche
- **Echtzeit-Visualisierungen** und animierte Elemente

---

## Build

### Windows (Visual Studio 2019/2022)
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### macOS / Linux
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Die ausführbare Datei befindet sich in `build/NeonSakuraStudio_artefacts/Release/`.

---

## Lizenzen

Dieses Projekt steht unter der MIT-Lizenz.

---

<p align="center">
  <i>Neon Sakura Studio - Cyberpunk Music Production</i>
</p>
