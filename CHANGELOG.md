# Changelog

Las versiones anteriores a 1.0.3 están descritas en las [Releases](https://github.com/santosmanilva/Santos-Leveler/releases).
Earlier versions are described in the [Releases](https://github.com/santosmanilva/Santos-Leveler/releases).

## [1.0.3] · 2026-10-05

### Español

Versión de mejora del procesado y de la interfaz. **Los presets existentes pueden sonar distinto** (ver «Al actualizar»).

**Cambios que se oyen**

- **Compresor.** Attack y Release ahora coinciden con los controles (con Attack en 10 ms la respuesta real era de unos 45 ms) y la reducción sigue la curva de Threshold y Ratio. El enlace estéreo usa el canal más fuerte: una voz solo en un canal se comprime igual que centrada. El resultado es que **comprime más y más rápido que en 1.0.2**.
- **Peak 2.** Ahora usa el lookahead. Antes reaccionaba tarde y, con 30 ms de lookahead, un golpe repentino de voz se pasaba unos 4 dB del umbral.
- **True Peak Limiter.** La bajada de ganancia es una rampa en lugar de un salto de una muestra, y la detección usa sobremuestreo 8x. Con sibilantes limitadas ~1 dB la salida llegaba a −0,5 dBTP con el techo en −1 dBTP; ahora respeta el techo con una tolerancia de unos 0,07 dB en el peor caso medido. El lookahead pasa de 1 a 2 ms (**+1 ms de latencia**) y el margen de seguridad de 0,01 a 0,1 dB.

**Interfaz**

- Rediseño completo: placa de hardware con módulos hundidos, mandos con anillo de LEDs, dos VU analógicos, pads luminosos, navegador de presets y lecturas LUFS de siete segmentos. Acento violeta.
- La gráfica Live Response se separa en un panel de niveles (con líneas de Target y Peak) y un carril de ganancia en dB.
- El panel Output muestra pico, true peak y la reducción del compresor y del limitador.
- Los deslizadores muestran su ranura completa y se restauran a su valor por defecto con doble clic.
- Tamaño mínimo 655 × 320, máximo 2620 × 1280, proporción fija.
- Se muestra la latencia real que informa el plugin.

**Correcciones**

- La gráfica Live Response podía quedarse esperando y bloquear la interfaz si el DAW reiniciaba el plugin con el transporte parado.
- El bypass del DAW dejaba pasar el audio sin el retardo del lookahead (unos 31 ms por delante). Ahora usa el bypass alineado en latencia del plugin.
- El LUFS integrado reservaba memoria sin límite durante la sesión; ahora usa un histograma de tamaño fijo y el mismo resultado (diferencia de 0,003 LU).
- El medidor de true peak ya no puede leer por debajo del pico de muestra en contenido de alta frecuencia.
- Los tests se compilaban en Release con los `assert` desactivados y no comprobaban nada; ahora siempre comprueban, y hay un test nuevo por cada corrección.

**Al actualizar**

- Escucha tus presets (Default, Gentle, Natural, Broadcast y Tight) y ajusta Threshold, Ratio y Makeup si hace falta. Los valores de los presets no se han cambiado.
- Si tienes automatizado el bypass del DAW sobre este plugin en sesiones antiguas, esa automatización puede perderse porque ahora controla el parámetro Bypass del plugin.
- La latencia sube 1 ms. Los DAW que compensan latencia lo hacen solos.
- Los manuales en PDF de v1.0.0 describen la interfaz anterior.

### English

Processing and interface update. **Existing presets may sound different** (see "Upgrade notes").

**Audible changes**

- **Compressor.** Attack and Release now match their controls (with Attack at 10 ms the real response was about 45 ms) and the reduction follows the Threshold and Ratio curve. Stereo link uses the louder channel: a voice on one channel is compressed like a centred one. The result is that it **compresses more and faster than 1.0.2**.
- **Peak 2.** It now uses the lookahead. It used to react late and, with 30 ms of lookahead, a sudden voice burst exceeded the threshold by about 4 dB.
- **True Peak Limiter.** Gain reduction is a ramp instead of a one-sample step, and detection uses 8x oversampling. With sibilants limited by ~1 dB the output reached −0.5 dBTP at a −1 dBTP ceiling; it now holds the ceiling to within about 0.07 dB in the worst measured case. Lookahead goes from 1 to 2 ms (**+1 ms of latency**) and the safety margin from 0.01 to 0.1 dB.

**Interface**

- Full redesign: hardware faceplate with recessed modules, LED-ring knobs, two analogue VU meters, glowing pads, a preset navigator and seven-segment LUFS read-outs. Violet accent.
- The Live Response graph is split into a level pane (with Target and Peak lines) and a gain lane in dB.
- The Output panel shows peak, true peak and compressor and limiter gain reduction.
- Sliders show their full slot and reset to their default with a double click.
- Minimum size 655 × 320, maximum 2620 × 1280, fixed aspect ratio.
- The latency reported by the plugin is shown on screen.

**Fixes**

- The Live Response graph could wait forever and freeze the UI if the host restarted the plugin while the transport was stopped.
- The host bypass let audio through without the lookahead delay (about 31 ms early). It now uses the plugin's latency-aligned bypass.
- Integrated LUFS allocated unbounded memory during a session; it now uses a fixed-size histogram with the same result (0.003 LU difference).
- The true-peak meter can no longer read below the sample peak on high-frequency content.
- Tests were built in Release with `assert` disabled and checked nothing; they now always check, with a new test for each fix.

**Upgrade notes**

- Listen to your presets (Default, Gentle, Natural, Broadcast and Tight) and adjust Threshold, Ratio and Makeup if needed. Preset values have not been changed.
- If old sessions automate the host bypass on this plugin, that automation may be lost because it now drives the plugin's Bypass parameter.
- Latency increases by 1 ms. Hosts with latency compensation handle it automatically.
- The v1.0.0 PDF manuals describe the previous interface.
