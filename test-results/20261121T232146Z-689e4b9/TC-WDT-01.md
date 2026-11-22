# TC-WDT-01 — Estabilidad 24 h

- imagen: `0.0.0-3e3ffe7-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (23:22:21.177 → 23:22:30.598 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- ventana: 86400 s; cortes de 300 s: broker a t=7200 s, wifi a t=21600 s, broker a t=36000 s, wifi a t=50400 s, broker a t=64800 s

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Ventana de 24 h 0 min con 5 cortes; inicio 2026-11-21 19:23:48 -0400
- inicio de la ventana: 2026-11-21 19:23:48 -0400; fin previsto: 2026-11-22 19:23:48 -0400

- Corte de broker 5 min (t=7209 s, 2026-11-21 21:23:57 -0400)
- Corte de wifi 5 min (t=21602 s, 2026-11-22 01:23:50 -0400)
- Corte de broker 5 min (t=36000 s, 2026-11-22 05:23:48 -0400)
- Corte de wifi 5 min (t=50419 s, 2026-11-22 09:24:07 -0400)
- Corte de broker 5 min (t=64807 s, 2026-11-22 13:23:55 -0400)
- log: `TC-WDT-01-serie.log` (13347 líneas)
- log: `TC-WDT-01-mqtt.log` (1572 líneas)
- log: `TC-WDT-01-banco.log` (0 líneas)
- fin de la ventana: 2026-11-22 19:23:48 -0400 (24.00 h)

- errores transitorios del banco (reintentos 5/10/20 s; no cuentan para el nodo): 21 intentos fallidos, 4 recuperaciones

- log: `TC-WDT-01-banco-transitorios.log` (25 líneas)

## Resultado

- status en vivo: 1419 (1419 en la ventana); primero: uptime_s=10, reset=usb; último: uptime_s=86460, reset=usb
- corte de broker a t=7209 s: primer status en vivo a 3.4 s de devolverlo
- corte de wifi a t=21602 s: primer status en vivo a 26.3 s de devolverlo
- corte de broker a t=36000 s: primer status en vivo a 3.4 s de devolverlo
- corte de wifi a t=50419 s: primer status en vivo a 26.1 s de devolverlo
- corte de broker a t=64807 s: primer status en vivo a 3.4 s de devolverlo
- reinicios del nodo: ninguno
- reinicios del banco (reset=usb/jtag, no cuentan; #83/#86): ninguno
- reset que cambia sin reinicio: ninguno
- consola: rst: [('0x15', 'USB_UART_CHIP_RESET')]; líneas con task_wdt: 0
- huecos entre status fuera de cortes y reinicios del banco: [(1795330788.7, 240.0)]

**Resultado: FAIL** — AssertionError: huecos entre status (ciclos sin publicar) fuera de los cortes: [(1795330788.7, 240.0)]
assert not [(1795330788.7, 240.0)]
