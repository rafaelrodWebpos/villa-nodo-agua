# TC-LOG-08 — Log UDP solo por unicast al receptor

- imagen: `0.0.0-d9239fa-bench` (variante bench, run CI 45202669354)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (17:48:32.319 → 17:48:42.143 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Captura del tráfico del DUT durante el arranque y 3 ciclos
- status online en el broker a 1.0 s del reset
- log: `TC-LOG-08-serie.log` (265 líneas)

## Flujos del DUT (captura de cabeceras)

- UDP → 192.168.4.1:53 (unicast): 23
- TCP → 192.168.4.1:1883 (unicast): 73
- UDP → 192.168.4.1:5555 (unicast): 52
- UDP → 255.255.255.255:67 (broadcast): 2

**Resultado: PASS** — 52 datagramas de log, todos unicast a 192.168.4.1:5555
