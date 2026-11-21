# TC-WDT-01 — Estabilidad 24 h

- imagen: `0.0.0-3e3ffe7-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (16:30:29.300 → 16:30:40.384 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- ventana: 86400 s; cortes de 300 s: broker a t=7200 s, wifi a t=21600 s, broker a t=36000 s, wifi a t=50400 s, broker a t=64800 s

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Ventana de 24 h 0 min con 5 cortes; inicio 2026-11-20 12:31:58 -0400
- inicio de la ventana: 2026-11-20 12:31:58 -0400; fin previsto: 2026-11-21 12:31:58 -0400

- Corte de broker 5 min (t=7209 s, 2026-11-20 14:32:07 -0400)
- Corte de wifi 5 min (t=21625 s, 2026-11-20 18:32:23 -0400)
- Corte de broker 5 min (t=36014 s, 2026-11-20 22:32:13 -0400)
- Corte de wifi 5 min (t=50403 s, 2026-11-21 02:32:02 -0400)

**Resultado: not done** — el banco cortó la conexión (`Connection reset by peer`) en GET /api/wifi/ap_status a las 2026-11-21 05:26:17 −0400, a las 16 h 54 min de la ventana, con 4 de 5 cortes hechos. Es un fallo de comunicación con el banco, no del firmware.

## Corrección (2026-11-21)

- La prueba registró el resultado como FAIL porque una llamada de vigilancia al banco lanzó la excepción sin reintentos. Se corrige a **not done** (decisión del propietario, 2026-11-21).
- Resultado original: `FAIL — CommandTimeout: GET /api/wifi/ap_status: [Errno 104] Connection reset by peer`.
- La excepción cortó la prueba antes de guardar la consola y los mensajes MQTT, así que esta evidencia no permite decir si el nodo se reinició en la ventana. Los pasos muestran que el banco hizo los 4 cortes (broker 14:32, Wi-Fi 18:32, broker 22:32 y Wi-Fi 02:32).
- Desde el commit siguiente, las llamadas de vigilancia toleran errores transitorios: 3 reintentos tras 5, 10 y 20 s, y not done solo si el banco no responde durante más de 2 min seguidos.


## Estado del SLOT2 tras el desenchufe del 2026-11-21 (lectura de consola, 19:21)

- Hacia las 15:20 el propietario desenchufó por error la placa del SLOT2 y la volvió a enchufar, así que el uptime ya no dice nada del intento 3.
- Una sola lectura pasiva del buffer de consola del SLOT2 (`GET /api/serial/output`, 500 líneas; sin tocar SLOT1), 2026-11-21 19:21: última línea `I (14750989) nodo: con: CONECTANDO_WIFI -> CONECTANDO_WIFI (wifi_down)` a las 19:21:03. Uptime 14 751 s → **arranque a las 15:15:12**, coherente con el desenchufe.
- De 17:13 a 19:21, ciclo normal de reintentos Wi-Fi sin AP (backoff de 60 s), con la marca de ms creciendo sin saltos: **sin reinicios después de ese arranque**, sin pánico ni watchdog. Las líneas del arranque ya no están en el buffer.
