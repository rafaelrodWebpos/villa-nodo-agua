# TC-HA-A1 — Home Assistant: entidades de §14

- imagen: `0.0.0-46989e0-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (04:32:21.155 → 04:32:31.942 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Home Assistant de prueba 2026.10.4 en la laptop (instancia nueva)

- Reset y AP arriba; esperar T11 y el dispositivo en Home Assistant
- Reinicio 1 de 3; esperar T11
- Reinicio 2 de 3; esperar T11
- Reinicio 3 de 3; esperar T11
- log: `TC-HA-A1-serie.log` (750 líneas)

## Resultado

- conexión: 1 dispositivo(s), 20 entidades — ["no disponibles (status online no visto): ['sensor.agua_sitio1_zona1_nivel_de_la_cisterna', 'sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2', 'sensor.agua_sitio1_zona1_presion']"]
- reinicio 1: 1 dispositivo(s), 20 entidades — ["no disponibles (status online no visto): ['sensor.agua_sitio1_zona1_nivel_de_la_cisterna', 'sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2', 'sensor.agua_sitio1_zona1_presion']"]
- reinicio 2: 1 dispositivo(s), 20 entidades — ["no disponibles (status online no visto): ['sensor.agua_sitio1_zona1_nivel_de_la_cisterna', 'sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2', 'sensor.agua_sitio1_zona1_presion']"]
- reinicio 3: 1 dispositivo(s), 20 entidades — ["no disponibles (status online no visto): ['sensor.agua_sitio1_zona1_nivel_de_la_cisterna', 'sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2', 'sensor.agua_sitio1_zona1_presion']"]

## Entidades (último reinicio)

- `valve.agua_sitio1_zona1_valvula_general` = unknown
- `switch.agua_sitio1_zona1_permiso_de_la_bomba` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_1` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_2` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_3` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_4` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_5` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_6` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_7` = unknown
- `binary_sensor.agua_sitio1_zona1_fuga_8` = unknown
- `binary_sensor.agua_sitio1_zona1_flotador` = unknown
- `binary_sensor.agua_sitio1_zona1_red_electrica` = unknown
- `sensor.agua_sitio1_zona1_nivel_de_la_cisterna` = unavailable
- `sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2` = unavailable
- `sensor.agua_sitio1_zona1_presion` = unavailable
- `sensor.agua_sitio1_zona1_corriente_de_la_bomba` = unavailable
- `sensor.agua_sitio1_zona1_tension_de_la_bateria` = unavailable
- `button.agua_sitio1_zona1_rearmar_la_bomba` = unknown
- `button.agua_sitio1_zona1_ejercitar_la_valvula` = unknown
- `event.agua_sitio1_zona1_evento` = unknown

**Resultado: FAIL** — AssertionError: conexión: ["no disponibles (status online no visto): ['sensor.agua_sitio1_zona1_nivel_de_la_cisterna', 'sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2', 'sensor.agua_sitio1_zona1_presion']"] (FR-HA-A1)
assert not ["no disponibles (status online no visto): ['sensor.agua_sitio1_zona1_nivel_de_la_cisterna', 'sensor.agua_sitio1_zona1_nivel_de_la_cisterna_2', 'sensor.agua_sitio1_zona1_presion']"]
