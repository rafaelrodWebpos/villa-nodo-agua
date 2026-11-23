# TC-LOG-04 — Sin log UDP en producción

- imagen: `0.0.0-d9239fa` (variante production, run CI 45202675792)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (17:53:37.009 → 17:53:46.741 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Captura del tráfico del DUT durante el arranque y 3 ciclos (imagen de producción)
- status online en el broker a 0.6 s del reset
- log: `TC-LOG-04-serie.log` (257 líneas)

## Flujos del DUT (captura de cabeceras)

- UDP → 192.168.4.1:53 (unicast): 24
- TCP → 192.168.4.1:1883 (unicast): 72
- UDP → 255.255.255.255:67 (broadcast): 2

**Resultado: PASS** — por UDP solo 53, 67; ningún datagrama al 5555
