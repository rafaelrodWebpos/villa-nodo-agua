# TC-CON-03 — Backoff de Wi-Fi con el AP apagado

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (01:02:57.045 → 01:03:07.292 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Reset con el AP del banco apagado; observación de 10 reintentos (≈ 6 min)
- log: `TC-CON-03-serie.log` (98 líneas)

## Resultado

- líneas T09: 11, T10: 10
- intervalo 1: 1.0 s (esperado 1 s ± 10 %)
- intervalo 2: 4.0 s (esperado 2 s ± 10 %)
- intervalo 3: 8.0 s (esperado 4 s ± 10 %)
- intervalo 4: 16.0 s (esperado 8 s ± 10 %)
- intervalo 5: 32.0 s (esperado 16 s ± 10 %)
- intervalo 6: 60.0 s (esperado 32 s ± 10 %)
- intervalo 7: 17.56 s (esperado 60 s ± 10 %)
- intervalo 8: 60.0 s (esperado 60 s ± 10 %)
- intervalo 9: 60.0 s (esperado 60 s ± 10 %)
- intervalo 10: 22.41 s (esperado 60 s ± 10 %)

**Resultado: FAIL** — AssertionError: intervalo 2: 4.0 s fuera de 2 s ± 10 %
assert 2.0 <= (2 * 0.1)
 +  where 2.0 = abs((4.0 - 2))
