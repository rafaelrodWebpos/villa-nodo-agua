# TC-WDT-01 — Estabilidad 24 h

- imagen: `0.0.0-d9239fa-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (18:22:03.674 → 18:22:14.495 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- ventana: 86400 s; cortes de 300 s: broker a t=7200 s, wifi a t=21600 s, broker a t=36000 s, wifi a t=50400 s, broker a t=64800 s

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Ventana de 24 h 0 min con 5 cortes; inicio 2026-11-23 14:23:29 -0400
- inicio de la ventana: 2026-11-23 14:23:29 -0400; fin previsto: 2026-11-24 14:23:29 -0400

- Corte de broker 5 min (t=7210 s, 2026-11-23 16:23:38 -0400)
- Corte de wifi 5 min (t=21627 s, 2026-11-23 20:23:56 -0400)
- Corte de broker 5 min (t=36022 s, 2026-11-24 00:23:50 -0400)
- Corte de wifi 5 min (t=50410 s, 2026-11-24 04:23:38 -0400)
- Corte de broker 5 min (t=64827 s, 2026-11-24 08:23:56 -0400)
- log: `TC-WDT-01-serie.log` (13351 líneas)
- log: `TC-WDT-01-mqtt.log` (1553 líneas)
- log: `TC-WDT-01-banco.log` (0 líneas)
- fin de la ventana: 2026-11-24 14:23:29 -0400 (24.00 h)

- errores transitorios del banco (reintentos 5/10/20 s; no cuentan para el nodo): 1 intentos fallidos, 1 recuperaciones

- log: `TC-WDT-01-banco-transitorios.log` (2 líneas)

## Resultado

- status en vivo: 1422 (1422 en la ventana); primero: uptime_s=10, reset=usb; último: uptime_s=86460, reset=usb
- corte de broker a t=7210 s: primer status en vivo a 3.4 s de devolverlo
- corte de wifi a t=21627 s: primer status en vivo a 26.1 s de devolverlo
- corte de broker a t=36022 s: primer status en vivo a 3.4 s de devolverlo
- corte de wifi a t=50410 s: primer status en vivo a 26.3 s de devolverlo
- corte de broker a t=64827 s: primer status en vivo a 3.6 s de devolverlo
- reinicios del nodo: ninguno
- reinicios del banco (reset=usb/jtag, no cuentan; #83/#86): ninguno
- reset que cambia sin reinicio: ninguno
- consola: rst: [('0x15', 'USB_UART_CHIP_RESET')]; líneas con task_wdt: 0
- huecos entre status fuera de cortes y reinicios del banco: ninguno

**Resultado: PASS** — 24.00 h (2026-11-23 14:23:29 -0400 → 2026-11-24 14:23:29 -0400), 5 cortes: 0 reinicios del nodo, 0 del banco, reset=usb
