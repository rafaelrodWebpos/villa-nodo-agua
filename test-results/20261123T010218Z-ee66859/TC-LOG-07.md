# TC-LOG-07 — Copia del log por UDP

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (01:55:13.192 → 01:55:22.846 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- AP del banco arriba (sin broker) y reset; observar 3 ciclos
- log: `TC-LOG-07-serie.log` (241 líneas)
- log: `TC-LOG-07-udp.log` (753 líneas)
- 93 líneas serie desde T08; sin copia 0; tarde 0

**Resultado: PASS** — 93 líneas con copia UDP en ≤ 1.0 s durante 3 ciclos
