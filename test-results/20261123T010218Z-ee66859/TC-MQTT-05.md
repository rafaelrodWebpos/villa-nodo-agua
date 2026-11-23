# TC-MQTT-05 — Broker con autenticación

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (01:51:48.428 → 01:51:58.856 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Broker con autenticación que NO acepta las credenciales del DUT; 90 s
- con credenciales rechazadas: 7 intentos MQTT, T11: 0
- Broker con las credenciales del DUT; esperar T11
- log: `TC-MQTT-05-serie.log` (204 líneas)
- intervalos entre intentos (s): [1.0, 2.0, 4.0, 8.0, 16.0, 32.0]
- con las credenciales aceptadas: T11 sí

**Resultado: PASS** — rechazadas: 7 intentos con backoff [1.0, 2.0, 4.0, 8.0, 16.0, 32.0]; aceptadas: OPERATIVO
