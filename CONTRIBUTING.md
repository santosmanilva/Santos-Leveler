# Contribuir / Contributing

## Español

¡Gracias por tu interés en Santos Leveler! Este proyecto es software libre bajo **GNU AGPL v3.0 (AGPL-3.0-only)** y las colaboraciones son bienvenidas.

### Informar de un error

1. Mira en [Issues](https://github.com/santosmanilva/Santos-Leveler/issues) si ya está comunicado.
2. Si no, abre un Issue nuevo con la plantilla **Error / Bug report** e incluye:
   - la versión de Santos Leveler (cuadro **About**, botón «i»);
   - el DAW y su versión, y la versión de Windows;
   - la frecuencia de muestreo y el tamaño de buffer;
   - los pasos para reproducirlo y, si ayuda, el preset o una captura.

### Proponer una mejora

Abre un Issue con la plantilla **Propuesta / Feature request** y explica qué problema resuelve. Es mejor hablarlo antes de escribir código.

### Enviar cambios (Pull Request)

1. Haz un fork y crea una rama a partir de `main`.
2. Compila y ejecuta los tests DSP: `powershell -ExecutionPolicy Bypass -File .\build-windows.ps1` (requisitos en el [README](README.md)).
3. Los tests deben pasar. Si cambias el procesado de audio, añade un test y explica en el Pull Request cómo lo mediste.
4. Mantén el estilo del código existente y describe el cambio con claridad.
5. Al enviar tu colaboración aceptas que se publique bajo la misma licencia, **AGPL-3.0-only**.

Los cambios de interfaz deben mantener la identidad visual definida en `Source/UI/Theme.h`.

## English

Thank you for your interest in Santos Leveler! This project is free software under the **GNU AGPL v3.0 (AGPL-3.0-only)** and contributions are welcome.

### Reporting a bug

1. Check the [Issues](https://github.com/santosmanilva/Santos-Leveler/issues) in case it is already reported.
2. If not, open a new Issue with the **Bug report** template and include:
   - the Santos Leveler version (**About** box, the "i" button);
   - the DAW and its version, and the Windows version;
   - the sample rate and buffer size;
   - the steps to reproduce it and, if it helps, the preset or a screenshot.

### Suggesting an improvement

Open an Issue with the **Feature request** template and explain which problem it solves. It is better to discuss it before writing code.

### Sending changes (Pull Request)

1. Fork the repository and create a branch from `main`.
2. Build and run the DSP tests: `powershell -ExecutionPolicy Bypass -File .\build-windows.ps1` (requirements in the [README](README_EN.md)).
3. The tests must pass. If you change the audio processing, add a test and explain in the Pull Request how you measured it.
4. Follow the existing code style and describe the change clearly.
5. By submitting your contribution you agree that it is published under the same licence, **AGPL-3.0-only**.

Interface changes should keep the visual identity defined in `Source/UI/Theme.h`.
