# TC-TIME-08 — Reinicio sin SNTP: hora no válida

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (00:10:08.830 → 00:10:19.344 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Resetear el DUT y levantar el AP desechable (canal 1, con salida a Internet); esperar T11
- Bloquear SNTP y enviar «reiniciar» por la consola (USB-Serial/JTAG)
- log: `TC-TIME-08-serie.log` (171 líneas)
- boot tras «reiniciar»: reset=sw
- hora_valida en los status tras el reinicio: [False, False, False]
- «sntp: sincronizado» tras el reinicio: 0

**Resultado: PASS** — reset=sw; hora_valida = false en 3 status sin SNTP
