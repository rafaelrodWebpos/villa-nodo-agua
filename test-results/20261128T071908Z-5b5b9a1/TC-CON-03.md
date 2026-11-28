# TC-CON-03 — Backoff de Wi-Fi con el AP apagado

- imagen: `0.0.0-d9239fa-bench` (variante bench, run CI 45202669354)

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (07:19:46.473 → 07:19:57.909 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Reset con el AP del banco apagado; observación de 10 reintentos (≈ 6 min)
- log: `TC-CON-03-serie.log` (93 líneas)

## Resultado

- líneas T09: 9, T10: 10; líneas viejas reenviadas descartadas (#89): 1
- intervalo 1: 1.0 s (esperado 1 s ± 10 %)
- intervalo 2: 4.0 s (esperado 2 s ± 10 %)
- intervalo 3: 8.0 s (esperado 4 s ± 10 %)
- intervalo 4: 16.0 s (esperado 8 s ± 10 %)
- intervalo 5: 32.0 s (esperado 16 s ± 10 %)
- intervalo 6: 60.0 s (esperado 32 s ± 10 %)
- intervalo 7: 60.0 s (esperado 60 s ± 10 %)
- intervalo 8: 60.0 s (esperado 60 s ± 10 %)
- intervalo 9: 60.0 s (esperado 60 s ± 10 %)

- wifi_down ausente estimado a [6795] ms, dentro del hueco del banco (4400, 8700) ms sin ninguna línea (upstream #89)

**Resultado: SKIP** — not done: solo 9 intervalos medidos; el wifi_down que falta (≈ 6795 ms) cae en el hueco de la captura del banco (4400, 8700) ms, sin ninguna línea en ese tramo (SensorsIot/Embedded-AI-Harness#89)
