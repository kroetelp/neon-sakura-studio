# NeonSakuraStudio

Ein moderner, JUCE-basierter Drum-Sequencer mit Pattern-Generierung, Music-Theory-Features und integriertem Wavetable-Synthesizer.

## Features

### Drum Sequencer
- **8-Track Drum Sequencer** mit 64 Steps
- **Genre-basierte Pattern-Generierung** (Techno, House, Trap, DnB, Ambient, Garage)
- **Euclidean Rhythms** für organische Rhythmen
- **Melodie-Generator** mit Skalen, Arpeggios und chord Progressions
- **Music Theory Engine** für harmonisch korrekte Patterns
- **Swing & Reverb** für mehr Groove und Atmosphäre
- **Sample-Management** mit Kategorie-basiertem Laden

### Wavetable Synthesizer
- **3 Wavetable-Oszillatoren** mit Morphing zwischen Frames
- **Sub-Oszillator** mit verschiedenen Wellenformen
- **Wavetable-Import** - Lade eigene WAV-Dateien oder Serum-kompatible Wavetables
- **Modulationssystem** mit 4 LFOs, 3 Hüllenkurven und Modulation-Matrix
- **Filter-Sektion** mit Lowpass, Highpass, Bandpass, Notch
- **Preset-Management** mit Factory- und User-Presets
- **Echtzeit-Visualisierung** der Wavetable-Wellenform

### UI
- **Neon-inspiriertes UI** im Cyberpunk-Stil
- **Serum-inspiriertes Synth-Layout**

## Abhängigkeiten

- **JUCE 8.0.4** (wird automatisch via CMake FetchContent geladen)
- **melatonin_blur** (für UI-Effekte, wird automatisch geladen)
- **CMake 3.22+**
- **C++20 Compiler**

### Optional

- **super-strudel-desktop** - Für Strudel/TidalCycles-Integration (separates Repository)

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Projektstruktur

```
source/
├── Main.cpp              # Application Entry Point
├── MainComponent.h/cpp   # Haupt-GUI und Audio-Setup
├── AudioEngine.h/cpp     # Audio-Processing und Playback
├── TrackComponent.h/cpp  # UI für einen Track
├── TrackModel.h/cpp      # Datenmodell für Tracks
├── TrackAudioProcessor.h/cpp  # Audio-Processing pro Track
├── PatternGenerator.h/cpp # Genre-basierte Pattern-Generierung
├── RhythmExplorer.h/cpp  # UI für rhythmische Exploration
├── MusicTheory.h/cpp     # Skalen, Akkorde, Progressions
├── MelodyGenerator.h/cpp # Melodische Pattern-Generierung
├── MelodyPanel.h/cpp     # UI für Melodie-Workstation
├── WavetableSynth/       # Wavetable Synthesizer Engine
│   ├── WavetableData     # Wavetable-Daten und -Laden
│   ├── WavetableOscillator # Oszillator mit Unison/Morphing
│   ├── WavetableVoice    # Stimme mit Filter/Hüllenkurve
│   ├── WavetableSynth    # Synthesiser mit Voices
│   ├── WavetableEngine   # Standalone Engine
│   ├── WavetableParams   # Thread-sichere Parameter
│   └── WavetablePresetManager # Preset-Verwaltung
├── WavetableUI/          # Synthesizer UI
│   ├── WavetableDisplay  # Wavetable-Visualisierung
│   ├── OscillatorSection # Oszillator-Controls
│   ├── FilterSection     # Filter-Controls
│   ├── EnvelopeSection   # Hüllenkurven-Controls
│   ├── ModulationGrid    # Modulation-Matrix UI
│   └── WavetableSynthEditor # Haupt-Editor
└── Modulation/           # Modulationssystem
    ├── ModulationMatrix  # Routing-System
    ├── LFOModulator      # LFOs
    └── EnvelopeModulator # Hüllenkurven
```

## Step Modifiers

Jeder Step kann folgende Modifiers haben:
- **Gain** (g) - Lautstärke pro Step
- **Pitch** (p) - Pitch-Shift in Halbtönen
- **Pan** (n) - Stereo-Position
- **Ratchet** (r) - Schnelle Wiederholungen
- **Reverb** (v) - Reverb-Anteil
- **Delay** (d) - Delay-Anteil
- **Filter** (f) - Filter-Cutoff

## Lizenz

MIT
