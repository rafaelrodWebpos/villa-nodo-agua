# TC-WDT-02 — Watchdog de tareas (wdt-test)

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (00:08:22.522 → 00:08:33.741 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Enviar «wdt-test» por la consola (USB-Serial/JTAG)
- log: `TC-WDT-02-serie.log` (133 líneas)
- de «bloqueando ciclo» a la línea boot: 30.2 s; reset=task_wdt

**Resultado: PASS** — reset=task_wdt a 30.2 s de «bloqueando ciclo»
