# Santos Leveler v1.0.0

**Voice Auto Level Rider · VST3 · Windows x64 · Open Source**

[![Build Windows VST3](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml/badge.svg)](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml)
![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-informational)
![Format](https://img.shields.io/badge/format-VST3-orange)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-4c8bf5)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C)
![License](https://img.shields.io/badge/license-AGPL--3.0-brightgreen)

[Documentación en español](README.md)

## Download and manuals

**[⬇ Download Santos Leveler v1.0.0 — Windows x64 VST3](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.0/Santos-Leveler-v1.0.0-Windows-x64-VST3.zip)**

Latest stable release · Windows 10/11 x64 · VST3

- **[User Manual — English (PDF)](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.0/Santos-Leveler-v1.0.0-User-Manual-EN.pdf)**
- **[Manual de usuario — Español (PDF)](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.0/Santos-Leveler-v1.0.0-Manual-Usuario-ES.pdf)**
- [View the v1.0.0 Release](https://github.com/santosmanilva/Santos-Leveler/releases/tag/v1.0.0)

![Santos Leveler v1.0.0](docs/santos-leveler-v1.0.0.jpg)

Santos Leveler is a VST3 voice processor designed to keep speech at a more consistent level automatically while preserving natural dynamics and providing detailed visual feedback. It is written in C++ with JUCE.

**Designed & Developed by Santos Leveler Project.**

> Santos Leveler is **free and open-source software** licensed under the **GNU Affero General Public License v3.0 (AGPL-3.0-only)**. You may use, study, modify and redistribute the code under the terms of AGPLv3. See [LICENSE](LICENSE).

## Project history

The first Santos Leveler prototypes were created in **[MNodes](https://marionietoworld.com/mnodes/)**, the modular audio environment developed by **Mario Nieto**. MNodes was used to explore and validate the automatic voice-leveling concept before the project was reimplemented as a native C++/JUCE VST3 plugin.

The current version does not depend on MNodes at runtime.

More information: [HISTORY.md](HISTORY.md).

## Compatibility

- Windows 10/11 x64
- 64-bit VST3 audio effect
- Mono and stereo tracks
- Stereo-linked processing
- Resizable GUI
- Initial size: **1310 × 640**
- Maximum size: **2625 × 1280**
- No Standalone build

## Main features

- **Voice Auto Level Rider** with Target, Gate, Speed, Detect, Lookahead, Hold and Release.
- Combined **FAST/SLOW detector**.
- **Smart Gate** with hysteresis.
- **Preserve Dynamics**.
- **Range Down / Range Up** up to ±16 dB.
- **Down Strength / Up Strength**.
- Global **Intensity** for Rider and Peak 2.
- **Peak 2 (in Rider, adaptive release)**.
- **Dynamics** section with feed-forward, stereo-linked soft-knee voice compressor.
- Stereo-linked **True Peak Limiter** (ITU-R BS.1770-5, fixed 120 ms release) with Ceiling from -9 to -1 dBTP.
- Peak meters for Input, Leveler Out and Final Out.
- **True Peak dBTP** and **LUFS-M / LUFS-S / LUFS-I** metering.
- **Live Response** graph with individually switchable INPUT, RIDER, PEAK and LEVELER OUT traces.
- **A/B** memories.
- Factory presets and user `.slpreset` files.
- **Latency-aligned bypass**.

## Signal path

```text
INPUT
  ↓
Voice Auto Level Rider
  ├─ FAST/SLOW detector
  ├─ Smart Gate
  ├─ Preserve Dynamics
  ├─ Range / Strength / Intensity
  └─ Peak 2 (adaptive release)
  ↓
LEVELER OUT trim
  ↓
Voice Compressor (stereo-linked, soft knee)
  ↓
True Peak Limiter (ITU-R BS.1770-5, 1 ms lookahead)
  ↓
Latency-aligned Bypass
  ↓
Peak / True Peak / LUFS
  ↓
OUTPUT
```

## v1.0.0 Default values

| Parameter | Default |
|---|---:|
| Gate | -40 dB |
| Target | -19 dB |
| Speed | 79 ms |
| Detect | 8 ms |
| Lookahead | 30 ms |
| Hold | 100 ms |
| Release | 100 ms |
| Peak | -8 dBFS |
| Range Down | -12 dB |
| Down Strength | 69 % |
| Range Up | +15 dB |
| Up Strength | 50 % |
| Leveler Out | 0 dB |
| Intensity | 100 % |
| Compressor | On |
| Comp Threshold | -20 dB |
| Comp Ratio | 3:1 |
| Comp Attack | 10 ms |
| Comp Release | 120 ms |
| Comp Makeup | +2 dB |
| Ceiling | -1 dBTP |
| Bypass | Off |

## Presets

Factory presets: **Default, Gentle, Natural, Broadcast and Tight**.

User presets use the `.slpreset` extension and are stored by default in:

```text
Documents\Santos Leveler Presets
```

## Metering

- **INPUT:** peak dBFS
- **LEVELER OUT:** peak dBFS before Dynamics
- **FINAL OUT:** peak dBFS after compressor, True Peak and final bypass
- **TRUE PEAK:** dBTP
- **LOUDNESS:** LUFS-M, LUFS-S and LUFS-I

Loudness metering is based on **ITU-R BS.1770-5** algorithms and EBU R128/Tech 3341 concepts. Santos Leveler is not presented as certified measurement equipment.

## Building on Windows

Requirements:

1. Visual Studio 2022 with **Desktop development with C++**.
2. CMake 3.22 or newer.
3. Git for Windows.

The project uses C++17 and fetches **JUCE 8.0.12** through CMake FetchContent.

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

The script builds `SantosLeveler_VST3` and `SantosLevelerDSPTests`, runs the DSP tests and prints the generated VST3 location.

## Installation

Copy the complete `Santos Leveler.vst3` folder to:

```text
C:\Program Files\Common Files\VST3
```

Then rescan VST3 plug-ins in your DAW.

## Licence

Santos Leveler is released under the **GNU Affero General Public License v3.0, version 3 only (`AGPL-3.0-only`)**.

The licence allows use, study, modification and redistribution under its copyleft terms. Redistributed and modified versions must comply with AGPLv3 obligations, including access to the corresponding source code where required.

Official licence text: https://www.gnu.org/licenses/agpl-3.0.html

See also:

- [LICENSE](LICENSE)
- [PUBLIC_SOURCE_NOTICE.md](PUBLIC_SOURCE_NOTICE.md)
- [THIRD_PARTY.md](THIRD_PARTY.md)

## Technical and liability notice

The software is provided without warranty under the terms of AGPLv3. Loudness, peak and True Peak functions are part of the audio-processing workflow and do not make Santos Leveler certified measurement equipment or guarantee compliance with any specific broadcast or delivery requirement.

Modified versions should be clearly identified as modified and must not be presented as official or endorsed Santos Leveler releases without authorisation.

## Contact

GitHub: `github.com/santosmanilva/Santos-Leveler`
