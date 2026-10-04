# SANTOS LEVELER — native VST3 project

This is a native C++/JUCE implementation of the working **SANTOS LEVELER v17** MNodes patch. Once compiled, the resulting VST3 does **not** require MNodes on the target computer.

## Current target

- Windows 10/11 x64
- VST3 effect
- Optional Standalone build for testing
- Mono and stereo tracks, stereo-linked detector
- 64-bit host process

## Parameters

| Parameter | Range | Default | Behaviour |
|---|---:|---:|---|
| Target | -36…-12 dB | -20 dB | Desired voice level |
| Gate | -70…-25 dB | -45 dB | Below this detector level (minus a fixed 3 dB hysteresis band), rider returns to unity rather than raising background noise |
| Attack | 2…250 ms | 12 ms | Gain response time while pulling gain DOWN (input louder than Target); 2 ms minimum prevents the unstable sub-2 ms region found during MNodes testing |
| Release | 2…250 ms | 45 ms | Gain response time while raising gain back UP (input quieter than Target) |
| Detect | 1…100 ms | 8 ms | RMS detector window |
| Range Down | -12…0 dB | -9 dB | Amount of downward rider correction |
| Range Up | 0…+12 dB | +9 dB | Amount of upward rider correction; 0 disables positive riding |
| Lookahead | 0…15 ms | 8 ms | Delays the audio path so the rider gain (computed from the live, undelayed signal) has a head start on transients before they reach the output. Reported to the host as plugin latency (PDC) |
| Ceiling | -12…0 dB | -0.3 dB | Safety limiter threshold applied after the rider and output trim; instant attack, 60 ms smoothed release |
| Output | -12…+12 dB | 0 dB | Final output trim |

The Range behaviour intentionally follows the working v17 patch: the positive and negative correction branches are each capped at 12 dB and then scaled by the selected Range value.

Attack and Release replace the single v1 "Speed" knob, split by direction — the same convention a compressor uses for gain reduction vs. gain recovery, applied here to the rider's downward and upward correction.

## History graph

The native UI draws all three histories in one large graph:

- **Blue:** input RMS level, -60…0 dBFS
- **Yellow:** actual smoothed rider gain, -12…+12 dB
- **Green:** output RMS level, -60…0 dBFS

When the host provides transport state, the graph **stops advancing when the DAW is stopped/paused** and resumes on Play. In Standalone mode, where there is no DAW transport, history runs continuously.

## Build on Windows

### Requirements

1. Visual Studio 2022 with **Desktop development with C++**
2. CMake 3.22 or newer
3. Git for Windows

JUCE is fetched automatically at configure time. The project is pinned to JUCE **8.0.12** for reproducible builds.

### One-command build

Open PowerShell in this folder and run:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

The script configures CMake, compiles the VST3 and Standalone targets, and runs the lightweight DSP tests.

The resulting VST3 will be inside:

```text
build\windows-x64\SantosLeveler_artefacts\Release\VST3\SANTOS LEVELER.vst3
```

The exact intermediate path can vary slightly with JUCE/CMake; the build script prints the actual location when it finishes.

### Install

Copy the complete `SANTOS LEVELER.vst3` bundle/folder to:

```text
C:\Program Files\Common Files\VST3
```

Administrator rights may be required. Then rescan VST3 plug-ins in the DAW.

An optional helper is included:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-windows.ps1
```

Run that from an elevated PowerShell after building.

## Build without installing anything locally: GitHub Actions

The included workflow:

```text
.github/workflows/build-windows-vst3.yml
```

builds the Windows x64 VST3 on GitHub's Windows runner and uploads `SANTOS-LEVELER-Windows-x64-VST3` as a downloadable workflow artifact.

Typical workflow:

1. Create a GitHub repository.
2. Upload the contents of this folder.
3. Open **Actions → Build Windows VST3 → Run workflow**.
4. When it finishes, download the VST3 artifact from the workflow run.

## DSP design

The audio engine is independent of JUCE and lives in `Source/LevelerEngine.h`.

Simplified path:

```text
Input ──┬─────────────────────────────────────────────► Lookahead delay line ─┐
        ↓                                                                     │
linked RMS detector (reads the LIVE, undelayed signal)                        │
        ↓                                                                     │
level in dB                                                                   │
        ↓                                                                     │
Target - Input                                                                │
        ↓                                                                     │
positive / negative correction branches                                      │
        ↓                                                                     │
Range Up / Range Down                                                         │
        ↓                                                                     │
Gate activity (with hysteresis)                                               │
        ↓                                                                     │
dB → linear gain                                                              │
        ↓                                                                     │
Attack / Release gain smoothing  ───────────────────────────────────────────► × gain
                                                                                ↓
                                                                        Output trim
                                                                                ↓
                                                                   Ceiling safety limiter
                                                                                ↓
                                                                             Output
```

The detector reads the signal before it enters the lookahead delay line, so the smoothed gain has `Lookahead` ms to react before it is applied to the matching (now delayed) audio. The control decision is refreshed at approximately 240 Hz, mirroring the working MNodes implementation, while gain interpolation and the limiter both run per sample.

## DSP tests

`Tests/LevelerDSPTests.cpp` is framework-independent and checks five essential behaviours:

- a quiet signal is raised toward Target with full Range Up;
- Range Up = 0 prevents positive gain;
- Gate prevents the rider from raising a below-gate signal;
- Output -6 dB produces approximately 6 dB attenuation;
- the Ceiling safety limiter never lets a loud, hard-riding signal exceed the configured ceiling, sample by sample, even with Lookahead disabled.

The tests can be built without JUCE directly with any C++17 compiler, and are also included in the CMake/CI build.

## Distribution and licensing

The plug-in source uses JUCE. JUCE is dual-licensed; before distributing a closed-source/commercial binary, verify that your intended distribution complies with the JUCE licence you hold. The VST3 SDK itself is distributed under the current Steinberg VST3 licensing terms used by the JUCE version selected by this project.

The target computer does not need JUCE or MNodes installed; they are development/build dependencies, not runtime plug-in dependencies.

## Next improvements

Implemented since v1: lookahead, separate Attack/Release, a hysteretic Gate, and a Ceiling safety limiter (see Parameters above). Good candidates for later revisions are:

- true hard Range clamp mode instead of the current proportional range scaling;
- live-updating host latency (PDC) if Lookahead is dragged mid-playback, instead of only on prepare/reload;
- an exposed/adjustable Gate hysteresis amount (currently a fixed 3 dB internally);
- meter ballistics (peak-hold), History graph time axis and Target/Gate reference lines, a bypass/A-B toggle, and factory presets — UI-side work, not yet started;
- macOS Universal VST3/AU builds.
