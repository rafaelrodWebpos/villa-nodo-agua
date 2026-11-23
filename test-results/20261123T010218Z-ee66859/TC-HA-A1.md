# TC-HA-A1 — Home Assistant: entidades de §14

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (02:03:11.952 → 02:03:22.676 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado


**Resultado: SKIP** — not done: no hay Home Assistant de prueba (scripts/ha/ha_prueba.py up)
