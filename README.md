# Neon Sakura Studio

[![License: MIT](https://img.shields.io/badge/License-MIT-purple.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20macOS%20%7C%20Linux-blue.svg)](https://github.com/kroetelp/Neon-Sakura-Studio)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-orange.svg)](https://en.cppreference.com/w/cpp/20)
[![JUCE](https://img.shields.io/badge/JUCE-8.0.4-green.svg)](https://juce.com/)
[![VST3 Hosting](https://img.shields.io/badge/VST3-Hosting-enabled-brightgreen.svg)](https://juce.com/)
[![AU Hosting](https://img.shields.io/badge/AU-Hosting-enabled-blue.svg)](https://juce.com/)
[![LV2 Support](https://img.shields.io/badge/LV2-Support-enabled-blue.svg)](https://juce.com/)

**A modern, JUCE-based Digital Audio Workstation (DAW) with drum sequencing, wavetable synthesis, and VST3/AU plugin hosting - all wrapped in a stunning cyberpunk neon aesthetic.**

> **Keywords:** DAW, drum sequencer, wavetable synthesizer, VST3 hosting, music production, JUCE, C++20, audio plugin, MIDI, pattern generator, electronic music, techno, house, trap, drum and bass, ambient

![NeonSakuraStudio Screenshot](screenshots/sequencer.png)

---

## Language / Sprache

| Language | Link |
|:--------:|:----:|
| **English** | [You are here](#neon-sakura-studio) |
| **Deutsch** | [Hier klicken](#neon-sakura-studio-deutsch) |

---

## Features

### 🎹 Full DAW Functionality
- **Timeline-based Multi-Track Editing** - Professional DAW workflow with tracks, clips, and automation
- **8-Track Drum Sequencer** with 64 steps per track
- **Three View Modes** - Track View, Pattern Grid View, and Timeline View
- **Audio Recording** - Record audio directly to tracks
- **Pattern-to-Clip Conversion** - Seamless workflow between pattern and timeline modes

### 🎛️ VST3/AU Plugin Hosting ⭐ NEW
- **Full VST3 Plugin Support** - Load and host third-party VST3 instruments and effects
- **AU Plugin Support** (macOS) - Native Audio Unit hosting
- **LV2 Support** (Linux) - Open-source plugin format
- **Plugin Browser** - Easy plugin discovery and management
- **Plugin Scanner** - Automatic plugin detection at startup
- **Plugin Windows** - Native plugin UI integration
- **Plugin Parameters** - Full parameter automation support

### 🥁 Drum Sequencer
- **Genre-based Pattern Generation** - Techno, House, Trap, DnB, Ambient, Garage
- **Euclidean Rhythms** for organic, polyrhythmic patterns
- **Melody Generator** with scales, arpeggios, and chord progressions
- **Music Theory Engine** for harmonically correct patterns
- **Swing & Groove** for natural-feeling rhythms
- **Step Modifiers** - Gain, Pitch, Pan, Ratchet, Reverb, Delay, Filter per step
- **Sample Management** with category-based loading

### 🎹 Wavetable Synthesizer
- **3 Wavetable Oscillators** with frame morphing and unison
- **Sub Oscillator** with multiple waveforms
- **Wavetable Import** - Load your own WAV files or Serum-compatible wavetables
- **Waveshaper** - Add harmonics and saturation
- **FM/AM Modulation** - Complex sound design possibilities
- **Modulation System:**
  - 4 LFOs with various waveforms and tempo sync
  - 3 ADSR envelopes
  - Flexible modulation matrix
- **Filter Section** - Lowpass, Highpass, Bandpass, Notch
- **Preset Management** with factory and user presets
- **Real-time Visualization** of wavetable waveforms
- **Oscilloscope** for audio output monitoring

### 🎨 Professional UI System
- **Unified Sequencer Panel** with Track/Pattern/Timeline views
- **Docking System** - Flexible panel management with dock/float options
- **Theme Manager** - Switch between multiple visual themes
- **Workspace Management** - Save and restore complete UI layouts
- **Neon-inspired Cyberpunk UI** with custom designed aesthetics
- **Serum-inspired Synth Layout** for intuitive sound design
- **NeonSakuraLookAndFeel** - Custom LookAndFeel for consistent design
- **Responsive Panels** for different workflow zones
- **Real-time Visualizations** and animated elements

### 🖥️ Hardware Integration
- **Wooting Analog Keyboard Support** - Use your Wooting keyboard as an expressive MIDI controller with analog velocity
  - Powered by [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/)
  - Full analog key pressure sensitivity
  - Aftertouch and modulation support

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
│
├── Sequencer/                 # Multi-Mode Sequencer ⭐ NEW
│   ├── UnifiedSequencerModel.h/cpp    # Shared data model
│   ├── UnifiedSequencerPanel.h/cpp    # Main panel with mode tabs
│   ├── TrackViewComponent.h/cpp       # Track-based view
│   ├── PatternViewComponent.h/cpp     # Pattern grid view
│   └── TimelineViewComponent.h/cpp    # Timeline DAW view
│
├── VSTHost/                  # VST3/AU Plugin Hosting ⭐ NEW
│   ├── VSTPluginManager.h/cpp         # Plugin management
│   ├── PluginScanner.h/cpp             # Plugin discovery
│   ├── PluginInstance.h/cpp            # Plugin wrapper
│   ├── PluginWindow.h/cpp              # Plugin UI window
│   ├── PluginBrowserComponent.h/cpp     # Plugin browser UI
│   └── PluginLoadingCoordinator.h/cpp  # Async plugin loading
│
├── AudioRouting/              # Audio Processing Graph ⭐ NEW
│   ├── AudioRoutingGraph.h/cpp         # Main routing graph
│   ├── TrackProcessor.h/cpp             # Per-track processing
│   ├── MasterOutputProcessor.h/cpp      # Master bus
│   ├── SidechainManager.h/cpp          # Sidechain routing
│   ├── MIDIOutputHandler.h/cpp         # MIDI routing
│   └── PluginParameterAutomation.h/cpp  # Automation support
│
├── InternalSynth/             # Native Wavetable Synth ⭐ NEW
│   ├── InternalSynthProcessor.h/cpp      # Audio processor
│   └── InternalSynthEditor.h/cpp        # Synth UI
│
├── Theme/                    # Theme System ⭐ NEW
│   ├── ThemeManager.h/cpp                # Theme management
│   ├── ProfessionalTheme.h              # Color schemes
│   ├── WorkspaceManager.h/cpp           # UI state
│   └── WorkspacePreset.h/cpp           # Preset layouts
│
├── UI/                       # UI Components ⭐ NEW
│   ├── TransportBar.h/cpp               # Playback controls
│   ├── GlobalControlsBar.h/cpp          # Global settings
│   ├── TrackToolsBar.h/cpp             # Track tools
│   └── PanelTogglesBar.h/cpp          # Panel visibility
│
├── Timeline/                  # Timeline Features
│   ├── TimelineClip.h/cpp              # Clip data structure
│   ├── TimelineData.h/cpp              # Timeline state
│   ├── TimelineRenderer.h/cpp          # Visual rendering
│   ├── TimelinePanel.h/cpp             # Timeline UI
│   ├── TimelineViewport.h/cpp           # Scrollable viewport ⭐ NEW
│   ├── TimelinePlayHead.h/cpp          # Playhead visualization ⭐ NEW
│   ├── AutomationClip.h/cpp            # Automation data ⭐ NEW
│   ├── AutomationLane.h/cpp            # Automation UI ⭐ NEW
│   └── RecordingManager.h/cpp         # Audio recording
│
├── StepSequencer/            # Step Sequencer
│   ├── StepSequencerPanel.h/cpp       # Main UI
│   ├── StepSequencerClip.h/cpp        # Clip data ⭐ NEW
│   └── NeonStepButton.h/cpp           # Custom step button
│
├── WavetableSynth/             # Wavetable Synthesizer Engine
│   ├── WavetableData.h/cpp             # Wavetable data
│   ├── WavetableOscillator.h/cpp       # Oscillator
│   ├── SubOscillator.h/cpp             # Sub oscillator
│   ├── WavetableFilter.h/cpp           # Filter
│   ├── Waveshaper.h/cpp              # Waveshaper
│   ├── WavetableVoice.h/cpp            # Voice
│   ├── WavetableSynth.h/cpp           # Synthesiser
│   ├── WavetableEngine.h/cpp           # Standalone engine
│   ├── WavetableParams.h/cpp           # Parameters
│   ├── WavetablePreset.h              # Preset data
│   └── WavetablePresetManager.h/cpp   # Preset management
│
├── WavetableUI/               # Synthesizer UI
│   ├── WavetableSynthEditor.h/cpp     # Main editor
│   ├── WavetableDisplay.h/cpp        # Visualization
│   ├── OscillatorSection.h/cpp        # Oscillator controls
│   ├── FilterSection.h/cpp            # Filter controls
│   ├── ShaperSection.h/cpp            # Waveshaper controls ⭐ NEW
│   ├── EnvelopeSection.h/cpp          # Envelope controls
│   ├── ModulationSection.h/cpp        # Modulation UI
│   ├── FXSection.h/cpp               # Effects controls
│   ├── ModulationGrid.h/cpp           # Modulation matrix
│   ├── Oscilloscope.h/cpp             # Audio visualization
│   └── NeonSakuraLookAndFeel          # Custom look & feel
│
├── Modulation/                # Modulation System
│   ├── ModulationMatrix.h/cpp          # Routing
│   ├── ModulationSource.h             # Source definitions
│   ├── Modulator.h/cpp               # Base class
│   ├── LFOModulator.h/cpp            # LFOs
│   └── EnvelopeModulator.h/cpp        # Envelopes
│
└── MelodyPanel/               # Melody Features
    ├── MelodyPanel.h/cpp              # Main UI
    └── MelodyGenerator.h/cpp          # Melody generation
```

---

## Step Modifiers

Each step in sequencer can have following modifiers:

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

## VST3/AU Plugin Support

Neon Sakura Studio now fully supports third-party plugins:

**Features:**
- Automatic plugin scanning on startup
- Plugin browser with category filtering
- Native plugin UI in floating windows
- Parameter automation
- MIDI plugin support
- Multi-out plugins support

**Supported Formats:**
- **Windows:** VST3 (.vst3)
- **macOS:** VST3 + AU
- **Linux:** VST3 + LV2

---

## Roadmap

### Completed ✅
- [x] Multi-track Timeline with DAW features
- [x] VST3/AU/LV2 plugin hosting
- [x] Audio recording
- [x] Unified Sequencer with multiple views
- [x] Professional theme system
- [x] Docking panel system
- [x] Waveshaper synthesis
- [x] FM/AM modulation

### In Progress 🚧
- [ ] Automation lane improvements
- [ ] Plugin parameter automation UI
- [ ] MIDI export/import
- [ ] More wavetable presets

### Planned 📋
- [ ] Plugin presets management
- [ ] Macro controls
- [ ] Sidechain routing UI
- [ ] Project save/load
- [ ] Multi-output audio
- [ ] Plugin sandboxing

---

## Third-Party Libraries

| Library | Purpose | License |
|---------|---------|---------|
| [JUCE](https://juce.com/) | Audio Application Framework | JUCE License |
| [melatonin_blur](https://github.com/sudara/melatonin_blur) | UI Blur Effects | MIT |
| [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/) | Analog Keyboard Input | MIT |

---

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork repository
2. Create your feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## License

This project is licensed under MIT License - see [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- [JUCE Framework](https://juce.com/) - Amazing audio application framework
- [Wooting](https://wooting.io/) - For analog keyboard technology
- [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/) - Open source analog input SDK
- [melatonin](https://github.com/sudara/melatonin_blur) - Beautiful blur effects for JUCE

---

<p align="center">
  <i>Neon Sakura Studio - Cyberpunk Music Production</i>
</p>

---

# Neon Sakura Studio (Deutsch)

Ein moderner, JUCE-basierter DAW mit Drum-Sequencer, Wavetable-Synthesizer und VST3/AU-Plugin-Hosting - alles in einem atemberaubenden Cyberpunk-Neon-Design.

## Features

### 🎹 Vollständige DAW-Funktionalität
- **Timeline-basierte Multi-Track-Bearbeitung** - Professioneller DAW-Workflow mit Tracks, Clips und Automation
- **8-Track Drum Sequencer** mit 64 Steps pro Track
- **Drei Ansichts-Modi** - Track View, Pattern Grid View und Timeline View
- **Audio-Aufnahme** - Nehme Audio direkt in Tracks auf
- **Pattern-zu-Clip-Konvertierung** - Nahtloser Workflow zwischen Pattern- und Timeline-Modi

### 🎛️ VST3/AU-Plugin-Hosting ⭐ NEU
- **Vollständige VST3-Plugin-Unterstützung** - Lade und hoste Drittanbieter-VST3-Instrumente und -Effekte
- **AU-Plugin-Unterstützung** (macOS) - Natives Audio Unit Hosting
- **LV2-Support** (Linux) - Open-Source-Plugin-Format
- **Plugin-Browser** - Einfache Plugin-Entdeckung und -Verwaltung
- **Plugin-Scanner** - Automatische Plugin-Erkennung beim Start
- **Plugin-Fenster** - Native Plugin-UI-Integration
- **Plugin-Parameter** - Vollständige Parameter-Automatisierung

### 🥁 Drum Sequencer
- **Genre-basierte Pattern-Generierung** - Techno, House, Trap, DnB, Ambient, Garage
- **Euklidische Rhythmen** für organische, polyrhythmische Patterns
- **Melodie-Generator** mit Skalen, Arpeggios und Akkord-Fortschreitungen
- **Music-Theory-Engine** für harmonisch korrekte Patterns
- **Swing & Groove** für natürliches Rhythmus-Gefühl
- **Step Modifiers** - Gain, Pitch, Pan, Ratchet, Reverb, Delay, Filter pro Step
- **Sample-Management** mit kategoriebasiertem Laden

### 🎹 Wavetable-Synthesizer
- **3 Wavetable-Oszillatoren** mit Frame-Morphing und Unison
- **Sub-Oszillator** mit verschiedenen Wellenformen
- **Wavetable-Import** - Lade eigene WAV-Dateien oder Serum-kompatible Wavetables
- **Waveshaper** - Füge Harmonische und Saturation hinzu
- **FM/AM-Modulation** - Komplexe Sound-Design-Möglichkeiten
- **Modulationssystem:**
  - 4 LFOs mit verschiedenen Wellenformen und Tempo-Sync
  - 3 ADSR-Hüllkurven
  - Flexible Modulations-Matrix
- **Filter-Sektion** - Lowpass, Highpass, Bandpass, Notch
- **Preset-Management** mit Factory- und User-Presets
- **Echtzeit-Visualisierung** der Wavetable-Wellenformen
- **Oszilloskop** für Audio-Ausgabe

### 🎨 Professionelles UI-System
- **Unified Sequencer Panel** mit Track/Pattern/Timeline-Ansichten
- **Docking-System** - Flexibles Panel-Management mit Dock/Float-Optionen
- **Theme-Manager** - Wechsle zwischen verschiedenen visuellen Themes
- **Workspace-Management** - Speichere und stelle vollständige UI-Layouts wieder her
- **Neon-inspiriertes Cyberpunk-UI** mit maßgeschneidertem Design
- **Serum-inspiriertes Synth-Layout** für intuitives Sound-Design
- **NeonSakuraLookAndFeel** - Custom LookAndFeel für konsistentes Design
- **Responsive Panels** für verschiedene Arbeitsbereiche
- **Echtzeit-Visualisierungen** und animierte Elemente

### 🖥️ Hardware-Integration
- **Wooting Analog Tastatur-Support** - Nutze deine Wooting-Tastatur als ausdrucksstarken MIDI-Controller mit analoger Velocity
  - Unterstützt durch [Wooting Analog SDK](https://github.com/WootingKb/wooting-analog-sdk/)
  - Volle analoge Tastendruck-Empfindlichkeit
  - Aftertouch und Modulations-Unterstützung

---

## Lizenzen

Dieses Projekt steht unter der MIT-Lizenz.

---

<p align="center">
  <i>Neon Sakura Studio - Cyberpunk Music Production</i>
</p>
