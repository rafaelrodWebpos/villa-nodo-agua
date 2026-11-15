# JRN-01.3 — Conectar al broker y publicar status

- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/`

- Precondiciones: AP y broker del banco apagados
- Arrancar el broker, borrar los retenidos del nodo y suscribirse a sitio1/zona1/# y a su Discovery
- Resetear el DUT y levantar el AP desechable (canal 1)
- Suscriptor nuevo: status y Discovery deben estar retenidos
- log: `JRN-01.3-serie.log` (170 líneas)
- log: `JRN-01.3-mqtt.log` (42 líneas)

## Resultado

- primer mensaje: `sitio1/zona1/agua-7f9af8/status` qos=1 a +17.87 s del reset
- payload: `{"estado":"online","fw":"0.0.0-46989e0-bench","rssi":-30,"reset":"usb","uptime_s":4,"lecturas":0,"err_sensor":0,"ota":"ninguno","hora_valida":false}`
- mensajes del nodo en la ventana: 1
- status online: 1; Last Will offline: 0 (informativo: un segundo arranque daría 2 y 1)
- Discovery retenido: 20 de 20; faltan []; sobran []

**Resultado: PASS** — T11, status online con los 9 campos (fw=0.0.0-46989e0-bench, reset=usb) retenido; 20 configuraciones de Discovery retenidas, sin duplicados
