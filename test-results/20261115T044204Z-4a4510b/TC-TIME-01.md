# TC-TIME-01 — Hora por SNTP

- imagen: `0.0.0-46989e0-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (04:42:43.053 → 04:42:54.012 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Resetear el DUT y levantar el AP desechable (canal 1, con salida a Internet); esperar T11
- Esperar la sincronización y un status del ciclo posterior (≤ 140 s)
- log: `TC-TIME-01-serie.log` (168 líneas)
- log: `TC-TIME-01-mqtt.log` (22 líneas)

## Resultado

- salida del AP: True (rules_ok True)
- wifi_up: `I (5260) nodo: con: CONECTANDO_WIFI -> CONECTANDO_BROKER (wifi_up)`
- sincronización: `I (10730) nodo: sntp: sincronizado ts=1794717804` → 5.47 s desde wifi_up (≤ 30 s)
- primer status posterior: uptime_s=60, hora_valida=True

**Resultado: PASS** — sincroniza a 5.47 s de wifi_up; hora_valida true en el status siguiente
