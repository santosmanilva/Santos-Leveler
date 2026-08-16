# Santos Leveler v1.0.0

**Voice Auto Level Rider · VST3 · Windows x64 · Freeware**

[English documentation](README_EN.md)

Santos Leveler es un procesador de dinámica para voz diseñado para mantener un nivel más uniforme de forma automática, conservando naturalidad y ofreciendo control visual detallado del proceso. Está desarrollado en C++ con JUCE y se distribuye como **freeware** para uso personal y profesional.

**Diseñado y desarrollado por Santos Leveler Project.**

> Santos Leveler es freeware, no software de código abierto. El permiso de uso del binario está definido en [LICENSE](LICENSE). El código fuente y los derechos asociados permanecen reservados salvo indicación expresa en dicha licencia.

## Compatibilidad

- Windows 10/11 x64
- Formato VST3 de 64 bits
- Efecto de audio, no instrumento
- Pistas mono y estéreo
- Procesamiento estéreo enlazado
- Interfaz redimensionable
- Tamaño inicial compacto: **1310 × 640**
- Tamaño máximo de interfaz: **2625 × 1280**
- No requiere MNodes ni JUCE instalados en el equipo de destino
- No incluye versión Standalone

El archivo `historical MNodes prototype` se conserva únicamente como referencia histórica del prototipo original. La versión VST3 actual es una implementación nativa e independiente.

## Características principales

- **Voice Auto Level Rider** con control Target, Gate, Speed, Detect, Lookahead, Hold y Release.
- **Detector FAST/SLOW** combinado para equilibrar respuesta rápida y estabilidad.
- **Smart Gate** con histéresis para evitar elevar ruido o ambiente cuando no hay voz útil.
- **Preserve Dynamics** para reducir el exceso de corrección descendente sobre dinámica vocal sostenida.
- Rangos independientes **Range Down / Range Up** de hasta ±16 dB.
- Controles **Down Strength / Up Strength** para suavizar la cantidad de corrección aplicada.
- **Intensity** global para escalar la acción del Rider y Peak 2 sin desactivar Output ni True Peak.
- **Peak 2**, etapa interna de control de picos con ataque rápido y release adaptativo.
- Módulo **Dynamics** con compresor de voz feed-forward, estéreo enlazado y soft knee.
- **True Peak Limiter** estéreo enlazado con Ceiling ajustable de -9 a -1 dBTP.
- Medidores de pico dBFS para Input, Leveler Out y Final Out.
- Medición **True Peak dBTP** y loudness **LUFS-M / LUFS-S / LUFS-I** sobre la salida final.
- Gráfica **Live Response** con trazas INPUT, RIDER, PEAK y LEVELER OUT activables individualmente con clic sobre su leyenda.
- Memorias **A/B** para comparar dos configuraciones de sonido.
- Presets de fábrica y presets de usuario `.slpreset`.
- **Bypass alineado en latencia**, manteniendo el motor activo para una transición limpia.
- Ventana About integrada con información de versión, autor, licencia y contacto.

## Cadena de señal

```text
INPUT
  ↓
Voice Auto Level Rider
  ├─ detector FAST/SLOW
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
Bypass alineado
  ↓
Medición final: Peak / True Peak / LUFS
  ↓
OUTPUT
```

## Parámetros y valores Default v1.0.0

| Parámetro | Rango | Default |
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

Los presets de fábrica incluidos son **Default, Gentle, Natural, Broadcast y Tight**. El preset Default coincide con los valores iniciales de una instancia nueva.

Los presets de usuario se guardan con extensión `.slpreset`, por defecto en:

```text
Documentos\Santos Leveler Presets
```

Los presets contienen los parámetros de sonido. El estado de Bypass y las memorias A/B no se incluyen. La visibilidad de las trazas de la gráfica es una preferencia visual guardada con el estado de la instancia/proyecto, no con los presets.

## Gráfica Live Response

La gráfica central permite activar o desactivar cada traza haciendo clic en su nombre:

- **INPUT** — cian
- **RIDER** — amarillo
- **PEAK** — magenta
- **LEVELER OUT** — verde

La traza PEAK representa reducción, por lo que visualmente parte de **0 dB en la zona superior** y desciende hasta -18 dB. Esto evita ocupar innecesariamente el centro de la gráfica cuando Peak 2 no está actuando.

Cuando el host proporciona estado de transporte, el historial deja de avanzar al detener/pausar el DAW y continúa al volver a reproducir.

## Medición

Los medidores visibles de nivel utilizan lectura de pico, no RMS:

- **INPUT:** pico dBFS
- **LEVELER OUT:** pico dBFS antes del módulo Dynamics
- **FINAL OUT:** pico dBFS después de compresor, True Peak y bypass final
- **TRUE PEAK:** dBTP mediante sobremuestreo
- **LOUDNESS:** LUFS-M, LUFS-S y LUFS-I sobre la salida final

El medidor de loudness está basado en los algoritmos de **ITU-R BS.1770-5** y conceptos de medición EBU R128/Tech 3341. No se presenta como un medidor EBU Mode certificado.

## Diseño DSP

El motor de nivelado vive principalmente en `Source/LevelerEngine.h` y trabaja con decisión de control a aproximadamente **240 Hz**, mientras la ganancia se interpola por muestra.

El True Peak Limiter se encuentra en `Source/TruePeakLimiter.h`. Emplea interpolación sobremuestreada de 4 fases, 12 taps por fase, enlace estéreo y 1 ms de lookahead. El Ceiling es independiente de Intensity.

El compresor de voz está implementado en `Source/VoiceCompressor.h` y trabaja antes del True Peak Limiter.

La medición de loudness y True Peak de salida se encuentra en `Source/LoudnessMeter.h`.

## Compilar en Windows

### Requisitos

1. Visual Studio 2022 con **Desarrollo para el escritorio con C++**.
2. CMake 3.22 o superior.
3. Git for Windows.

El proyecto usa C++17 y descarga automáticamente **JUCE 8.0.12** durante la configuración mediante CMake FetchContent.

### Compilación con script

Desde PowerShell en la carpeta del proyecto:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-windows.ps1
```

El script configura el proyecto, compila **SantosLeveler_VST3** y **SantosLevelerDSPTests**, ejecuta los tests DSP y muestra la ubicación del bundle VST3 generado.

### Instalación local

Copiar la carpeta completa `Santos Leveler.vst3` a:

```text
C:\Program Files\Common Files\VST3
```

También puede utilizarse, desde PowerShell con los permisos necesarios:

```powershell
powershell -ExecutionPolicy Bypass -File .\install-windows.ps1
```

Después debe realizarse un rescan de plugins VST3 en el DAW.

## GitHub Actions

El workflow `.github/workflows/build-windows-vst3.yml` compila en **Windows Server 2022**, ejecuta los tests DSP y genera el artefacto:

```text
Santos-Leveler-v1.0.0-Windows-x64-VST3
```

El paquete incluye el bundle VST3 y `LICENSE`.

## Tests DSP

`Tests/LevelerDSPTests.cpp` comprueba, entre otros puntos:

- corrección ascendente y descendente del Rider;
- Strength y rangos extendidos ±16 dB;
- Intensity al 0 %;
- comportamiento de Gate y Output trim;
- bypass funcional del compresor y reducción con COMP activo;
- latencia y Ceiling del True Peak Limiter a -1 y -3 dBTP;
- LUFS-M, LUFS-S, LUFS-I y True Peak del medidor interno;
- reset de Integrated y True Peak máximo.

En la candidata final v1.0.0 la compilación Release y `SantosLevelerDSPTests.exe` se validaron correctamente con código de salida 0.

## Licencia

Santos Leveler se distribuye como **freeware**. El binario puede utilizarse gratuitamente para fines personales y profesionales conforme a [LICENSE](LICENSE). No es software open source y la licencia no concede permiso general para vender, modificar o redistribuir el software.

Se incluye una traducción al español en el propio archivo `LICENSE` a título informativo; la versión inglesa es la que gobierna en caso de discrepancia.

El proyecto utiliza dependencias de terceros. Consulte [THIRD_PARTY.md](THIRD_PARTY.md) para información sobre JUCE y VST3.

## Contacto

**Santos Leveler Project**  
GitHub: `github.com/santosmanilva/SANTOS-LEVELER`  
Email: `github.com/santosmanilva/Santos-Leveler/issues`
