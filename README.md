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
| Gate | -70…-25 dB | -45 dB | Below this detector level, rider returns to unity rather than raising background noise |
| Speed | 2…250 ms | 15 ms | Gain response time; 2 ms minimum prevents the unstable sub-2 ms region found during MNodes testing |
| Detect | 1…100 ms | 8 ms | RMS detector window |
| Range Down | -12…0 dB | -9 dB | Amount of downward rider correction |
| Range Up | 0…+12 dB | +9 dB | Amount of upward rider correction; 0 disables positive riding |
| Output | -12…+12 dB | 0 dB | Final output trim |

The Range behaviour intentionally follows the working v17 patch: the positive and negative correction branches are each capped at 12 dB and then scaled by the selected Range value.

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
Input
  ↓
linked RMS detector
  ↓
level in dB
  ↓
Target - Input
  ↓
positive / negative correction branches
  ↓
Range Up / Range Down
  ↓
Gate activity
  ↓
dB → linear gain
  ↓
2 ms minimum gain smoothing
  ↓
Output trim
  ↓
Output
```

The detector control decision is refreshed at approximately 240 Hz, mirroring the working MNodes implementation, while gain interpolation runs per sample.

## DSP tests

`Tests/LevelerDSPTests.cpp` is framework-independent and checks four essential behaviours:

- a quiet signal is raised toward Target with full Range Up;
- Range Up = 0 prevents positive gain;
- Gate prevents the rider from raising a below-gate signal;
- Output -6 dB produces approximately 6 dB attenuation.

The tests can be built without JUCE directly with any C++17 compiler, and are also included in the CMake/CI build.

## Distribution and licensing

The plug-in source uses JUCE. JUCE is dual-licensed; before distributing a closed-source/commercial binary, verify that your intended distribution complies with the JUCE licence you hold. The VST3 SDK itself is distributed under the current Steinberg VST3 licensing terms used by the JUCE version selected by this project.

The target computer does not need JUCE or MNodes installed; they are development/build dependencies, not runtime plug-in dependencies.

## Next improvements

The current native implementation intentionally follows the proven v17 behaviour. Good candidates for later revisions are:

- 5–10 ms lookahead to reduce initial overshoot without sub-2 ms gain modulation;
- separate attack/release behaviour;
- soft/hysteretic Gate transition;
- true hard Range clamp mode instead of v17 proportional range scaling;
- optional safety ceiling/limiter;
- macOS Universal VST3/AU builds.
