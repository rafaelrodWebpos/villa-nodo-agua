# TC-LOG-08 — Log UDP solo por unicast al receptor

- imagen: `0.0.0-46989e0-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (03:56:46.948 → 03:56:56.958 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Captura del tráfico del DUT durante el arranque y 3 ciclos

## Flujos del DUT (captura de cabeceras)

- UDP → 192.168.4.1:53 (unicast): 24
- TCP → 192.168.4.1:1883 (unicast): 91
- UDP → 192.168.4.1:5555 (unicast): 53
- UDP → 255.255.255.255:67 (broadcast): 6

**Resultado: PASS** — 53 datagramas de log, todos unicast a 192.168.4.1:5555
