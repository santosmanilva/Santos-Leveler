# Santos Leveler v1.0.1

**Voice Auto Level Rider · VST3 · Windows x64 · Open Source**

[![Build Windows VST3](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml/badge.svg)](https://github.com/santosmanilva/SANTOS-LEVELER/actions/workflows/build-windows-vst3.yml)
![Version](https://img.shields.io/badge/version-1.0.1-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-informational)
![Format](https://img.shields.io/badge/format-VST3-orange)
![JUCE](https://img.shields.io/badge/JUCE-8.0.12-4c8bf5)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C)
![License](https://img.shields.io/badge/license-AGPL--3.0-brightgreen)

[English documentation](README_EN.md)

## Descarga y manuales

**[⬇ Descargar Santos Leveler v1.0.1 — Windows x64 VST3](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.1/Santos-Leveler-v1.0.1-Windows-x64-VST3.zip)**

Última versión estable · Windows 10/11 x64 · VST3

- **[Manual de usuario — Español (PDF)](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.1/Santos-Leveler-v1.0.1-Manual-Usuario-ES.pdf)**
- **[User Manual — English (PDF)](https://github.com/santosmanilva/Santos-Leveler/releases/download/v1.0.1/Santos-Leveler-v1.0.1-User-Manual-EN.pdf)**
- [Ver la Release v1.0.1](https://github.com/santosmanilva/Santos-Leveler/releases/tag/v1.0.1)

![Santos Leveler v1.0.1](https://raw.githubusercontent.com/santosmanilva/Santos-Leveler/main/docs/santos-leveler-v1.0.0.jpg)

Santos Leveler es un procesador VST3 para voz diseñado para mantener un nivel más uniforme de forma automática, conservando naturalidad y ofreciendo control visual detallado del proceso. Está desarrollado en C++ con JUCE.

**Diseñado y desarrollado por Santos Leveler Project.**

> Santos Leveler es **software libre y de código abierto** bajo **GNU Affero General Public License v3.0 (AGPL-3.0-only)**. Puedes usar, estudiar, modificar y redistribuir el código conforme a los términos de AGPLv3. Consulta [LICENSE](LICENSE).

## Historia del proyecto

Los primeros prototipos de Santos Leveler se crearon en **[MNodes](https://marionietoworld.com/mnodes/)**, el entorno modular de audio desarrollado por **Mario Nieto**. MNodes permitió explorar y validar inicialmente la idea del nivelador automático de voz antes de reimplementar el proyecto como plugin VST3 nativo en C++/JUCE.

La versión actual no depende de MNodes en tiempo de ejecución.

Más información: [HISTORY.md](HISTORY.md).

## Compatibilidad

- Windows 10/11 x64
- VST3 de 64 bits
- Efecto de audio para pistas mono y estéreo
- Procesamiento estéreo enlazado
- Interfaz redimensionable
- Tamaño inicial: **1310 × 640**
- Tamaño máximo: **2625 × 1280**
- Sin versión Standalone

## Características principales

- **Voice Auto Level Rider** con Target, Gate, Speed, Detect, Lookahead, Hold y Release.
- Detector combinado **FAST/SLOW**.
- **Smart Gate** con histéresis.
- **Preserve Dynamics**.
- **Range Down / Range Up** hasta ±16 dB.
- **Down Strength / Up Strength**.
- **Intensity** global para Rider y Peak 2.
- **Peak 2 (en Rider, release adaptativo)**.
- Módulo **Dynamics** con compresor de voz feed-forward, estéreo enlazado y soft knee.
- **True Peak Limiter** (ITU-R BS.1770-5, release fijo 120 ms) con Ceiling ajustable de -9 a -1 dBTP.
- Medidores de pico para Input, Leveler Out y Final Out.
- Medición **True Peak dBTP** y **LUFS-M / LUFS-S / LUFS-I**.
- Gráfica **Live Response** con INPUT, RIDER, PEAK y LEVELER OUT activables individualmente.
- Memorias **A/B**.
- Presets de fábrica y presets de usuario `.slpreset`.
- **Bypass alineado en latencia**.

## Cadena de señal

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
Bypass alineado
  ↓
Peak / True Peak / LUFS
  ↓
OUTPUT
