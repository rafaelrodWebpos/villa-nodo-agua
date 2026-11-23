# TC-CON-A5 — Reasociación con el broker caído 25 min

- imagen: `0.0.0-673bbe8-bench` (variante bench, run CI 45156827479)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (12:40:13.145 → 12:40:24.195 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Parar el broker 25 min con el AP arriba
- Arrancar el broker
- log: `TC-CON-A5-serie.log` (637 líneas)
- log: `TC-CON-A5-mqtt.log` (45 líneas)

## Resultado

- reasociaciones desde T15: [600.0, 1206.0] s (esperadas 600 y 1200 s ± 30 s)
- broker de vuelta: T11 sí; status online a +14.5 s (límite 75 s)

**Resultado: PASS** — reasociaciones a [600.0, 1206.0] s de T15, sin reinicio; status online a 14.5 s
