# TC-CON-06 — Nunca vuelve al portal sin Wi-Fi

- imagen: `0.0.0-673bbe8-bench` (variante bench, run CI 45156827479)

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (12:17:45.337 → 12:17:56.200 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Reset con el AP del banco apagado; 15 min de observación y 5 escaneos
- log: `TC-CON-06-serie.log` (132 líneas)

## Resultado

- líneas con: 39 (destino distinto de CONECTANDO_WIFI: 0)
- última línea del DUT a los 901 s de su arranque
- escaneo a t=185 s: ningún agua-*
- escaneo a t=365 s: ningún agua-*
- escaneo a t=545 s: ningún agua-*
- escaneo a t=725 s: ningún agua-*
- escaneo a t=905 s: ningún agua-*

**Resultado: PASS** — 39 líneas con:, todas hacia CONECTANDO_WIFI; el AP del portal no apareció en 5 escaneos
