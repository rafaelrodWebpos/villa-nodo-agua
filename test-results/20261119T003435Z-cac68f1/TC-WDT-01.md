# TC-WDT-01 — Estabilidad (corrida de humo, 15 min)

- imagen: `0.0.0-3e3ffe7-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (00:35:14.237 → 00:35:25.420 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- **corrida de humo** (NODO_WDT_HUMO=1): no cuenta como resultado de TC-WDT-01

- ventana: 900 s; cortes de 300 s: broker a t=300 s

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Ventana de 0 h 15 min con 1 cortes; inicio 2026-11-19 00:36:49 +0000
- inicio de la ventana: 2026-11-19 00:36:49 +0000; fin previsto: 2026-11-19 00:51:49 +0000

- Corte de broker 5 min (t=300 s, 2026-11-19 00:41:49 +0000)
- log: `TC-WDT-01-serie.log` (442 líneas)
- log: `TC-WDT-01-mqtt.log` (57 líneas)
- log: `TC-WDT-01-banco.log` (0 líneas)
- fin de la ventana: 2026-11-19 00:51:49 +0000 (0.25 h)


## Resultado

- status en vivo: 13 (13 en la ventana); primero: uptime_s=5, reset=usb; último: uptime_s=960, reset=usb
- corte de broker a t=300 s: primer status en vivo a 3.4 s de devolverlo
- reinicios del nodo: ninguno
- reinicios del banco (reset=usb/jtag, no cuentan; #83/#86): ninguno
- reset que cambia sin reinicio: ninguno
- consola: rst: ninguno; líneas con task_wdt: 0
- huecos entre status fuera de cortes y reinicios del banco: ninguno

**Resultado: PASS** — 0.25 h (2026-11-19 00:36:49 +0000 → 2026-11-19 00:51:49 +0000), 1 cortes: 0 reinicios del nodo, 0 del banco, reset=usb
