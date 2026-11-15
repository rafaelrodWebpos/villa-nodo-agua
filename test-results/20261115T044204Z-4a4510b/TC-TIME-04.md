# TC-TIME-04 — Resincronización fallida sin perder la hora

- imagen: `0.0.0-46989e0-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (04:44:57.512 → 04:45:07.538 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Resetear el DUT y levantar el AP desechable (canal 1, con salida a Internet); esperar T11
- Esperar la sincronización
- Bloquear SNTP 2 h
- Desbloquear SNTP
- log: `TC-TIME-04-serie.log` (129 líneas)
- log: `TC-TIME-04-mqtt.log` (142 líneas)

## Resultado

- sincronización inicial: `I (9690) nodo: sntp: sincronizado ts=1794717936`
- bloqueo: True (rules_ok True); desbloqueo: True
- en las 2 h: 120 status, 0 con hora_valida ≠ true
- «sntp: inicio» 6: ['I (3615760) nodo: sntp: inicio pool.ntp.org', 'I (3625760) nodo: sntp: inicio pool.ntp.org', 'I (3635760) nodo: sntp: inicio pool.ntp.org', 'I (7245810) nodo: sntp: inicio pool.ntp.org']
- «sntp: fallo» 2: ['W (3645760) nodo: sntp: fallo', 'W (7275810) nodo: sntp: fallo']
- sincronizaciones durante el bloqueo: 0; arranques: 0

**Resultado: PASS** — 120 status con hora_valida = true en 2 h de bloqueo; 2 «sntp: fallo»
