# JRN-01.1 — Portal sin configuración

- slot: SLOT2; id esperado (FR-ID-01, MAC de dut.identity): `agua-7f9af8`

- Precondiciones: AP del banco apagado (el escaneo comparte la radio)
- Reset y escaneo hasta ver agua-7f9af8 (≤ 10 s)

## Escaneos (solo SSID agua-*)

- t=5.6 s: [('agua-7f9af8', 'OPEN')]

- log: `JRN-01.1-boot.log` (239 líneas)
- `agua-7f9af8` visto a los 5.6 s (cota superior desde la petición de reset), auth OPEN
- T02 en el reloj del DUT: boot: → con: en 430 ms (límite 10000 ms)
- FR-ID-01: reinicio 2 de 3
- log: `JRN-01.1-boot2.log` (119 líneas)
- FR-ID-01: reinicio 3 de 3
- log: `JRN-01.1-boot3.log` (119 líneas)
- ids en 3 arranques: ['agua-7f9af8', 'agua-7f9af8', 'agua-7f9af8']

**Resultado: PASS** — PORTAL en 5.6 s, agua-7f9af8 abierto, id estable en 3 arranques
