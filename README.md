# NeonSakuraStudio

Ein JUCE-basierter 4-Spur Step-Sequencer mit TidalCycles/Strudel-ähnlichen Modifiern für kreatives Pattern-Design.

![NeonSakuraStudio](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

## Features

### Kernfunktionen
- **4-Spur Sequencer** mit je 16 Steps pro Bank
- **4 Pattern-Banks** (A, B, C, D) für bis zu 64 Steps pro Track
- **Steuerelemente** pro Track:
  - Volume (Lautstärke)
  - Pitch (Transposition ±12 Halbtöne)
  - Attack (Anstiegszeit 0.0-2.0s)
  - Decay (Abklingzeit 0.01-2.0s)
  - Cutoff (Low-Pass Filter 20Hz-20kHz)

### TidalCycles/Strudel Modifier-System

Right-Klick auf jeden Step-Button öffnet das Modifier-Kontextmenü:

#### Speed (*)
- `*2`, `*3`, `*4` - Ratcheting innerhalb eines Steps
- Beispiele: `*2` = 2 Trigger pro Step, `*4` = 4 Trigger pro Step
- Effekt: Schnelle, rhythmische Trills

#### Slow (/)
- `/2`, `/3`, `/4` - Trigger nur jeden N-ten Loop
- Beispiel: `/2` = Trigger nur jeden zweiten Loop durch
- Effekt: Sparse, minimale Patterns

#### Elongate (@)
- `@2`, `@3`, `@4` - Verlängert den Sound
- Der Decay-Parameter wird multipliziert für längeren Sustain
- Effekt: Länger anhaltende Klänge

#### Replicate (!)
- `!2`, `!3`, `!4` - Auto-Repeat auf nächsten Steps
- Beispiel: `!2` = Trigger wird auf dem nächsten Step wiederholt
- Effekt: Cascading Trigger-Effekte

### Zusätzliche Features
- **Sample-Suche**: Automatische Erkennung von SuperDirt-Sample-Ordnern
- **Manueller Ordnerwahl**: Auswahl beliebiger Sample-Ordner
- **Sample-Navigation**: `<` und `>` Buttons zum Durchsuchen der Samples pro Kategorie
- **Variable Loop-Länge**: 16, 32, 48 oder 64 Steps
- **BPM-Slider**: 60-200 BPM mit Echtzeit-Update
- **DSP Low-Pass Filter**: Pro Track mit dynamischer Cutoff-Steuerung

## Installation

### Voraussetzungen
- Windows 10/11
- Visual Studio 2022 oder neuer
- CMake 3.22 oder neuer
- C++20 Compiler

### Kompilierung

```bash
# Repository klonen
git clone https://github.com/DEIN-USERNAME/NeonSakuraStudio.git
cd NeonSakuraStudio

# Build-Verzeichnis erstellen
mkdir build
cd build

# CMake konfigurieren
cmake ..

# Kompilieren (Release)
cmake --build . --config Release

# Oder mit Visual Studio öffnen
cmake --open .
```

### Ausführen

Die kompilierte EXE befindet sich unter:
```
build/NeonSakuraStudio_artefacts/Release/NeonSakuraStudio.exe
```

## Bedienung

### Grundlegendes Sequencing

1. **Sample auswählen**: Wähle eine Kategorie aus dem Dropdown (z.B. "bd" für Bass Drum)
2. **Steps aktivieren**: Klicke auf die Step-Buttons (1-16) um Steps zu aktivieren/deaktivieren
3. **Playback starten**: Klicke auf "Play" oder drücke Space
4. **BPM anpassen**: Nutze den BPM-Slider oder gib einen Wert ein

### Modifier anwenden

1. **Right-Klick** auf einen aktiven Step-Button
2. Wähle einen Modifier aus dem Kontextmenü:
   - **Speed (*)**: Schnelle Ratchet-Effekte
   - **Slow (/)**: Sparse Patterns
   - **Elongate (@)**: Längere Sounds
   - **Replicate (!)**: Auto-Repeat
3. Der Button zeigt den Modifier und Wert an (z.B. "*3", "/2", "@4", "!2")

### Pattern-Banks

- Klicke auf **A, B, C, D** um zwischen Pattern-Banken zu wechseln
- Jede Bank speichert 16 unabhängige Steps
- Die Loop-Länge (16-64) gilt über alle Banks hinweg

### Sound-Shaping

- **Volume**: Gesamtlautstärke des Tracks
- **Pitch**: Transposition des Samples
- **Attack**: Anstiegszeit des Sounds (kürzere Attack = schärferer Klang)
- **Decay**: Abklingzeit des Sounds (längerer Decay = längerer Sustain)
- **Cutoff**: Low-Pass Filter für warme, weiche Klänge

## Sample-Ordner

### Automatische Erkennung

NeonSakuraStudio sucht automatisch in folgenden Ordnern nach Samples:
- `Dirt-Samples-master/` im Projektverzeichnis
- `Dirt-Samples/` im Projektverzeichnis
- `samples/` im Projektverzeichnis
- Ebenfalls im Eltern- und Großeltern-Verzeichnis

### Manualer Import

1. Klicke auf **"Set SuperDirt Folder"**
2. Navigiere zu deinem Sample-Ordner
3. Die Kategorie-Namen erscheinen automatisch im Dropdown

### SuperDirt Struktur

Der Ordner sollte folgendes Format haben:
```
Dirt-Samples/
├── bd/          # Bass Drums
├── sn/          # Snares
├── hh/          # Hi-Hats
├── cp/          # Cowbells
├── lt/          # Low Toms
├── ht/          # High Toms
├── perc/        # Percussion
└── ...
```

## Tastaturkürzel

| Funktion | Tastaturkürzel |
|----------|----------------|
| Play/Pause | Space |
| Stop | Esc |
| Sample vorher | < |
| Sample nächst | > |

## Tasten

| Tastatur | Funktion |
|----------|----------|
| 1-16 | Step-Buttons (nur für Tests) |
| A-D | Bank-Auswahl (nur für Tests) |

## Technische Details

### Architektur
- **JUCE 8.0.4** Framework für Audio und GUI
- **C++20** Standard
- **CMake** Build-System
- **MSVC** Compiler (Visual Studio 2022)

### Audio-Engine
- Sample-accurate Timing für präzises Ratcheting
- Separater MIDI-Buffer pro Track zur Vermeidung von Cross-Talk
- DSP Low-Pass Filter pro Track mit dynamischer Koeffizientenberechnung
- Bis zu 8 Synthesizer-Stimmen pro Track für schnelle Ratchet-Effekte

### Modifier-Implementierung

#### Speed (*) Ratcheting
```cpp
// Beispiel: *3 = 3 Trigger pro Step
samplesPerRatchetStep = samplesPerStep / 3;
// Trigger bei: 0, 1/3, 2/3 des Steps
```

#### Slow (/) Cycle
```cpp
// Beispiel: /2 = Trigger nur jeden zweiten Loop
if ((globalLoopCounter - 1) % 2 == 0)
    // Trigger
```

#### Elongate (@) Envelope
```cpp
// Der Decay-Parameter wird beim Sample-Laden verwendet
samplerSound = new SamplerSound(..., attackTime, decayTime * elongateFactor, ...);
```

#### Replicate (!) Auto-Repeat
```cpp
// Beispiel: !2 = Trigger auf nächstem Step wiederholen
for (int repStep = 1; repStep <= 1; ++repStep)
{
    // Trigger bei calculatedCurrentStep + repStep
}
```

## Bekannte Probleme

- Modifier-Effekte können bei sehr hohen BPM-Raten (180+) Timing-Probleme haben
- Sample-Ladevorgänge sind nicht thread-safe bei gleichzeitigen Category-Änderungen

## Zukünftige Features

- [ ] MIDI-In/Out für externe Geräte
- [ ] Pattern-Export als MIDI-Dateien
- [ ] Pattern-Save/Load mit Presets
- [ ] Swing-Parameter für groovige Rhythmen
- [ ] Random-Modifier (? in TidalCycles)
- [ ] Global-Effect-Chain (Delay, Reverb, Distortion)

## Lizenz

MIT License - siehe LICENSE-Datei für Details.

## Credits

- Gebaut mit [JUCE](https://juce.com/) - Jules' Utility Class Extensions
- Inspiriert von [TidalCycles](https://tidalcycles.org/)
- Sample-Kompatibilität mit [SuperDirt](https://github.com/musikinformatik/SuperDirt)

## Beiträge

Beiträge sind willkommen! Bitte erstelle einen Pull Request oder öffne ein Issue für Bugs und Feature-Requests.

## Kontakt

Für Fragen und Support bitte ein Issue auf GitHub erstellen.
