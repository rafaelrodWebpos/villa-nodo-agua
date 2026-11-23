# TC-CON-06 — Nunca vuelve al portal sin Wi-Fi

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (00:20:11.773 → 00:20:22.466 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Reset con el AP del banco apagado; 15 min de observación y 5 escaneos
- log: `TC-CON-06-serie.log` (145 líneas)

## Resultado

- líneas con: 53 (destino distinto de CONECTANDO_WIFI: 7)
- última línea del DUT a los 892 s de su arranque
- escaneo a t=185 s: ningún agua-*
- escaneo a t=365 s: ningún agua-*
- escaneo a t=545 s: ningún agua-*
- escaneo a t=725 s: ningún agua-*
- escaneo a t=905 s: ningún agua-*

**Resultado: FAIL** — AssertionError: líneas con: hacia otro estado: ['W (123365) nodo: con: ip perdida (wifi_down)', 'W (243365) nodo: con: ip perdida (wifi_down)', 'W (363365) nodo: con: ip perdida (wifi_down)']
assert not ['W (123365) nodo: con: ip perdida (wifi_down)', 'W (243365) nodo: con: ip perdida (wifi_down)', 'W (363365) nodo: con...dida (wifi_down)', 'W (603365) nodo: con: ip perdida (wifi_down)', 'W (723365) nodo: con: ip perdida (wifi_down)', ...]
