# Santos Leveler v1.0.1

**Voice Auto Level Rider · VST3 · Windows x64 · Open Source**

[![Build Windows VST3](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml/badge.svg)](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml)
![Version](https://img.shields.io/badge/version-1.0.1-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-informational)
![Format](https://img.shields.io/badge/format-VST3-orange)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-4c8bf5)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C)
![License](https://img.shields.io/badge/license-AGPL--3.0-brightgreen)

[Documentación en Español](README.md)

## Download and Manuals

**[⬇ Download Santos Leveler v1.0.1 — Windows x64 VST3](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.1/Santos-Leveler-v1.0.1-Windows-x64-VST3.zip)**

Latest stable release · Windows 10/11 x64 · VST3

- **[User Manual — English (PDF)](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.1/Santos-Leveler-v1.0.1-User-Manual-EN.pdf)**
- **[Manual de usuario — Español (PDF)](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.1/Santos-Leveler-v1.0.1-Manual-Usuario-ES.pdf)**
- [View Release v1.0.1](https://github.com/santosmanilva/Santos-Leveler/releases/tag/v1.0.1)

![Santos Leveler v1.0.1](https://raw.githubusercontent.com/santosmanilva/Santos-Leveler/main/docs/santos-leveler-v1.0.0.jpg)

Santos Leveler is a VST3 voice processor designed to maintain a more consistent level automatically, preserving naturalness while offering detailed visual control over the process. Built in C++ with JUCE.

**Designed and developed by Santos Leveler Project.**

> Santos Leveler is **free and open-source software** under the **GNU Affero General Public License v3.0 (AGPL-3.0-only)**. You may use, study, modify, and redistribute the code under the terms of the AGPLv3. See [LICENSE](LICENSE).

## Project History

The first prototypes of Santos Leveler were created in **[MNodes](https://marionietoworld.com/mnodes/)**, the modular audio environment developed by **Mario Nieto**. MNodes allowed for initial exploration and validation of the auto voice leveler idea before the project was re-implemented as a native C++/JUCE VST3 plugin.

The current version does not rely on MNodes at runtime.

More information: [HISTORY.md](HISTORY.md).

## Compatibility

- Windows 10/11 x64
- 64-bit VST3
- Audio effect for mono and stereo tracks
- Stereo-linked processing
- Resizable interface
- Initial size: **1310 × 640**
- Maximum size: **2625 × 1280**
- No Standalone version

## Main Features

- **Voice Auto Level Rider** with Target, Gate, Speed, Detect, Lookahead, Hold, and Release.
- Combined **FAST/SLOW** detector.
- **Smart Gate** with hysteresis.
- **Preserve Dynamics**.
- **Range Down / Range Up** up to ±16 dB.
- **Down Strength / Up Strength**.
- Global **Intensity** for Rider and Peak 2.
- **Peak 2 (within Rider, adaptive release)**.
- **Dynamics** module with feed-forward voice compressor, stereo-linked, soft knee.
- **True Peak Limiter** (ITU-R BS.1770-5, fixed 120 ms release) with adjustable Ceiling from -9 to -1 dBTP.
- Peak meters for Input, Leveler Out, and Final Out.
- **True Peak dBTP** and **LUFS-M / LUFS-S / LUFS-I** metering.
- **Live Response** graph with individually toggleable INPUT, RIDER, PEAK, and LEVELER OUT.
- **A/B** memory states.
- Factory presets and user presets `.slpreset`.
- **Latency-compensated bypass**.

## Signal Chain

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
Latency-compensated bypass
  ↓
Peak / True Peak / LUFS
  ↓
OUTPUT
