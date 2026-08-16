# Dependencias de terceros / Third-party dependencies

## Español

### JUCE

El proyecto está configurado para descargar **JUCE 8.0.12** desde el repositorio oficial `juce-framework/JUCE` durante la fase de configuración de CMake mediante FetchContent.

JUCE no se incluye como copia independiente dentro de este repositorio. El binario compilado incorpora código generado a partir de JUCE y su distribución debe cumplir la licencia de JUCE aplicable al desarrollador/distribuidor.

JUCE 8 se ofrece bajo un modelo de doble licencia: la licencia JUCE y AGPLv3. Para distribuir software cerrado que contenga JUCE deben cumplirse los términos de la licencia JUCE correspondiente, salvo que la distribución se realice válidamente bajo AGPLv3.

Referencias oficiales:

- JUCE 8 EULA: https://juce.com/legal/juce-8-licence/
- Información y FAQ de licencias: https://juce.com/get-juce/
- Repositorio oficial: https://github.com/juce-framework/JUCE

### VST3

Santos Leveler utiliza el soporte VST3 proporcionado a través de JUCE. Desde VST SDK 3.8, la interfaz y los componentes principales del SDK VST3 se distribuyen bajo licencia MIT; determinados componentes auxiliares pueden tener sus propias licencias.

Referencias oficiales:

- VST3 Developer Portal: https://steinbergmedia.github.io/vst3_dev_portal/
- VST3 Licensing: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST%2B3%2BLicensing/Index.html

Esta información se ofrece únicamente como referencia técnica. Cada distribuidor es responsable de comprobar y cumplir las licencias vigentes que resulten aplicables a su forma de uso y distribución.

---

## English

### JUCE

This project is configured to fetch **JUCE 8.0.12** from the official `juce-framework/JUCE` repository during the CMake configure step using FetchContent.

JUCE is not stored as a separate bundled copy in this repository. The compiled binary incorporates code built from JUCE, and distribution must comply with the JUCE licence applicable to the developer/distributor.

JUCE 8 is dual-licensed under the JUCE licence and AGPLv3. Distribution of closed-source software containing JUCE must comply with the applicable JUCE licence terms unless the software is validly distributed under AGPLv3.

Official references:

- JUCE 8 EULA: https://juce.com/legal/juce-8-licence/
- Licensing information and FAQ: https://juce.com/get-juce/
- Official repository: https://github.com/juce-framework/JUCE

### VST3

Santos Leveler uses VST3 support provided through JUCE. Since VST SDK 3.8, the VST3 interface and main SDK components are distributed under the MIT licence; some auxiliary components may have their own licences.

Official references:

- VST3 Developer Portal: https://steinbergmedia.github.io/vst3_dev_portal/
- VST3 Licensing: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST%2B3%2BLicensing/Index.html

This information is provided for technical reference only. Each distributor is responsible for reviewing and complying with the current licence terms applicable to their use and distribution model.
