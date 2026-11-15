# Agenda de depuración — Nodo Agua

Partes **del proyecto** que nunca se han visto funcionar aquí (respuesta del
propietario a la pregunta H1 del harness, 2026-11-14). `/commission` la quema
en orden. El banco no está en la lista: el proyecto depende de su calidad y no
la prueba.

**Condiciones del propietario:**
- La placa del SLOT2 es **solo de banco**: nunca se instala y nunca se le queman
  eFuses (C-SEC-A3).
- No se usa el banco mientras el piloto (`villa-nodo-clima-piloto`) tenga pruebas
  en curso. La coordinación está en `testing/test-plan.yaml` → `bench_coordination`.
- Los puntos 2–5 dependen de la compra de fin de mes
  (`compras-nodo-agua-banco.md` del propietario). Mientras tanto se avanza con lo
  que no depende de ella.

| # | Parte | No probada porque | Se prueba con | Depende de la compra | Estado |
|---|---|---|---|---|---|
| 1 | Placa DUT: segunda Waveshare ESP32-S3-DEV-KIT-N8R8 del SLOT2 | Nunca se ha usado como DUT de este proyecto; estuvo `absent` el 2026-11-01 | `chip-info` (chip, 8 MB, MAC) + grabar el esqueleto de la CI y ver `Init complete` y `NODO-BENCH-IMAGE` (JRN-01.0) | no | **hecho 2026-11-14**: chip v0.2, 8 MB, MAC en `dut.identity`, eFuses en 0; CI 43668818077 → flash → `NODO-BENCH-IMAGE` + `Init complete` (`commissioning.forward_path`) |
| 2 | Montaje de E/S del nodo (3 ADS1115 en 0x48/0x49/0x4A, puente H, relé, LED, zumbador, botón) | No se ha armado nunca | Línea `i2c: scan … ack=0x48,0x49,0x4a` (A7) + firmware de prueba que conmuta cada salida, vista por `bench-salidas-dut` | sí | bloqueado por la compra |
| 3 | Válvula real (sentido de giro, finales, tiempo de carrera) | Pieza nueva | Abrir y cerrar con IN1/IN2 y ver GPIO6/7; medir el tiempo de carrera | sí | bloqueado por la compra |
| 4 | Cableado de los simuladores CR-TB-16…22 hasta los pines del DUT | El fork del banco es del banco; el cableado hacia este DUT es nuestro | Cada estímulo del banco aparece en la línea de log correspondiente (A3, A4, `val:`, `bom:`) | sí | bloqueado por la compra |
| 5 | Sensores reales (transductor 4–20 mA con lazo a 24 V, presión 0,5–4,5 V, pinza, flotador, sondas mojadas con agua de la villa) | Piezas nuevas; P4 supone R(mojada) < 20 kΩ | Multímetro + líneas A3/A4 con estímulos reales (agua, aire comprimido, carga conocida) | sí | bloqueado por la compra |
| 6 | HA de prueba con las entidades de este nodo | Es el HA del piloto, nunca visto con este nodo | Discovery de §14 visible en HA (JRN) | no | **hecho 2026-11-15**: HA 2026.10.4 de prueba (`scripts/ha/ha_prueba.py`) descubre el nodo con sus 20 entidades (TC-HA-A1) |
