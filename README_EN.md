# Santos Leveler v1.0.0

**Voice Auto Level Rider · VST3 · Windows x64 · Freeware**

[![Build Windows VST3](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml/badge.svg)](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml)
![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-informational)
![Format](https://img.shields.io/badge/format-VST3-orange)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-4c8bf5)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C)
![License](https://img.shields.io/badge/license-Freeware-brightgreen)

[Documentación en español](README.md)

![Santos Leveler v1.0.0](docs/santos-leveler-v1.0.0.webp)

Santos Leveler is a voice dynamics processor designed to keep speech at a more consistent level automatically while preserving natural dynamics and providing detailed visual feedback. It is written in C++ with JUCE and distributed as **freeware** for personal and professional use.

**Designed & Developed by Santos Leveler Project.**

> Santos Leveler is freeware, not open-source software. Permission to use the compiled binary is defined in [LICENSE](LICENSE). Source code and associated rights remain reserved unless expressly granted by that licence.

## Compatibility

- Windows 10/11 x64
- 64-bit VST3 effect
- Mono and stereo tracks
- Stereo-linked processing
- Resizable GUI
- Default compact size: **1310 × 640**
- Maximum GUI size: **2625 × 1280**
- No MNodes or JUCE installation required on the target computer
- No Standalone build

`historical MNodes prototype` is kept only as a historical reference to the original prototype. The current VST3 is a native, independent implementation.

## Main features

- **Voice Auto Level Rider** with Target, Gate, Speed, Detect, Lookahead, Hold and Release controls.
- Combined **FAST/SLOW detector** for fast response with stable level decisions.
- **Smart Gate** with hysteresis to avoid lifting background noise when useful voice is absent.
- **Preserve Dynamics** logic to reduce excessive downward correction on sustained vocal dynamics.
- Independent **Range Down / Range Up** up to ±16 dB.
- **Down Strength / Up Strength** controls for softer correction behaviour.
- Global **Intensity** scaling for Rider and Peak 2 while keeping Output and True Peak active.
- **Peak 2** internal peak-control stage with fast attack and adaptive release.
- **Dynamics** section with feed-forward, stereo-linked, soft-knee voice compressor.
- Stereo-linked **True Peak Limiter** with adjustable Ceiling from -9 to -1 dBTP.
- dBFS peak meters for Input, Leveler Out and Final Out.
- **True Peak dBTP** plus **LUFS-M / LUFS-S / LUFS-I** metering on the final output.
- **Live Response** graph with individually clickable INPUT, RIDER, PEAK and LEVELER OUT traces.
- **A/B** memories for comparing two sound configurations.
- Factory presets and user `.slpreset` files.
- **Latency-aligned bypass** for clean switching.
- Integrated About window with version, author, licence and contact information.

## Signal path

```text
INPUT
  ↓
Voice Auto Level Rider
  ├─ FAST/SLOW detector
  ├─ Smart Gate
  ├─ Preserve Dynamics
  ├─ Range / Strength / Intensity
  └─ Peak 2
  ↓
LEVELER OUT trim
  ↓
Voice Compressor
  ↓
True Peak Limiter
  ↓
Latency-aligned Bypass
  ↓
Final Peak / True Peak / LUFS metering
  ↓
OUTPUT
```

## Parameters and v1.0.0 Default values

| Parameter | Range | Default |
|---|---:|---:|
| Target | -36…-6 dB | -19 dB |
| Gate | -70…-25 dB | -40 dB |
| Speed | 2…250 ms | 79 ms |
| Detect | 1…100 ms | 8 ms |
| Lookahead | 0…100 ms | 30 ms |
| Hold | 0…1000 ms | 100 ms |
| Release | 50…3000 ms | 100 ms |
| Peak | -18…-1 dBFS | -8 dBFS |
| Range Down | -16…0 dB | -12 dB |
| Down Strength | 0…100 % | 69 % |
| Range Up | 0…+16 dB | +15 dB |
| Up Strength | 0…100 % | 50 % |
| Leveler Out | -12…+12 dB | 0 dB |
| Intensity | 0…100 % | 100 % |
| Compressor | Off / On | On |
| Comp Threshold | -36…0 dB | -20 dB |
| Comp Ratio | 1:1…10:1 | 3:1 |
| Comp Attack | 0.5…100 ms | 10 ms |
| Comp Release | 20…1000 ms | 120 ms |
| Comp Makeup | 0…+12 dB | +2 dB |
| Ceiling | -9…-1 dBTP | -1 dBTP |
| Bypass | Off / On | Off |

## Presets

Factory presets: **Default, Gentle, Natural, Broadcast and Tight**. The Default preset matches the initial values of a new plugin instance.

User presets use the `.slpreset` extension and are stored by default in:

```text
Documents\Santos Leveler Presets
```

Sound presets do not include Bypass or A/B memories. Graph trace visibility is a visual preference saved with the plugin instance/project state rather than with sound presets.

## Live Response graph

Click each legend item to show or hide its trace:

- **INPUT** — cyan
- **RIDER** — yellow
- **PEAK** — magenta
- **LEVELER OUT** — green

The PEAK trace represents gain reduction, so it is displayed from **0 dB at the top** down to -18 dB. This keeps the centre of the graph cleaner when Peak 2 is inactive.

When host transport information is available, history stops advancing while the DAW is stopped/paused and resumes on playback.

## Metering

Visible level meters use peak readings rather than RMS:

- **INPUT:** peak dBFS
- **LEVELER OUT:** peak dBFS before Dynamics
- **FINAL OUT:** peak dBFS after compressor, True Peak and final bypass
- **TRUE PEAK:** oversampled dBTP
- **LOUDNESS:** LUFS-M, LUFS-S and LUFS-I on the final output

Loudness metering is based on **ITU-R BS.1770-5** algorithms and EBU R128/Tech 3341 metering concepts. It is not presented as a formally certified EBU Mode meter.

## DSP design

The leveler engine is mainly implemented in `Source/LevelerEngine.h`. Control decisions are refreshed at approximately **240 Hz**, while gain interpolation runs per sample.

The True Peak Limiter in `Source/TruePeakLimiter.h` uses 4-phase oversampled interpolation with 12 taps per phase, stereo linking and 1 ms lookahead. Ceiling operates independently from Intensity.

The voice compressor is implemented in `Source/VoiceCompressor.h` and runs before the True Peak Limiter.

Final loudness and True Peak metering is implemented in `Source/LoudnessMeter.h`.

## Building on Windows

### Requirements

1. Visual Studio 2022 with **Desktop development with C++**.
2. CMake 3.22 or newer.
3. Git for Windows.

The project uses C++17 and fetches **JUCE 8.0.12** automatically during CMake configuration.

### Build script

Run from PowerShell in the repository folder:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

The script configures the project, builds **SantosLeveler_VST3** and **SantosLevelerDSPTests**, runs the DSP tests and prints the generated VST3 bundle location.

### Local installation

Copy the complete `Santos Leveler.vst3` folder to:

```text
C:\Program Files\Common Files\VST3
```

Or run with the required permissions:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-windows.ps1
```

Then rescan VST3 plug-ins in your DAW.

## GitHub Actions

`.github/workflows/build-windows-vst3.yml` builds on **Windows Server 2022**, runs the DSP tests and uploads:

```text
Santos-Leveler-v1.0.0-Windows-x64-VST3
```

The package contains the VST3 bundle and `LICENSE`.

## DSP tests

`Tests/LevelerDSPTests.cpp` covers Rider up/down correction, Strength, extended ±16 dB ranges, Intensity at 0 %, Gate and Output trim, compressor bypass/reduction, True Peak latency and ceilings at -1/-3 dBTP, loudness M/S/I, True Peak metering, and meter reset behaviour.

The final v1.0.0 candidate was validated with a successful Release build and `SantosLevelerDSPTests.exe` returning exit code 0.

## Licence

Santos Leveler is distributed as **freeware**. The compiled binary may be used free of charge for personal and professional purposes under [LICENSE](LICENSE). It is not open-source software, and the licence does not grant general permission to sell, modify or redistribute the software.

A Spanish convenience translation is included in `LICENSE`; the English licence text governs if there is any discrepancy.

Third-party dependency information is available in [THIRD_PARTY.md](THIRD_PARTY.md).

## Contact

**Santos Leveler Project**  
GitHub: `github.com/santosmanilva/SANTOS-LEVELER`  
Email: `github.com/santosmanilva/Santos-Leveler/issues`
