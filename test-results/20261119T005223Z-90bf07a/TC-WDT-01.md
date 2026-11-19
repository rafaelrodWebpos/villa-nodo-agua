# TC-WDT-01 — Estabilidad 24 h

- imagen: `0.0.0-3e3ffe7-bench` (variante bench, run CI None)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (00:53:01.985 → 00:53:13.293 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- ventana: 86400 s; cortes de 300 s: broker a t=7200 s, wifi a t=21600 s, broker a t=36000 s, wifi a t=50400 s, broker a t=64800 s

- Resetear el DUT y levantar el AP desechable (canal 1); esperar T11
- Ventana de 24 h 0 min con 5 cortes; inicio 2026-11-18 20:54:35 -0400
- inicio de la ventana: 2026-11-18 20:54:35 -0400; fin previsto: 2026-11-19 20:54:35 -0400

- Corte de broker 5 min (t=7207 s, 2026-11-18 22:54:43 -0400)
- Corte de wifi 5 min (t=21623 s, 2026-11-19 02:54:58 -0400)
- Corte de broker 5 min (t=36011 s, 2026-11-19 06:54:47 -0400)
- log: `TC-WDT-01-serie.log` (5917 líneas)
- log: `TC-WDT-01-mqtt.log` (689 líneas)
- log: `TC-WDT-01-banco.log` (0 líneas)
- fin de la ventana: 2026-11-19 07:05:49 -0400 (10.19 h)


**Resultado: SKIP** — not done: el broker del banco se apagó sin la prueba a las 2026-11-19 07:05:49 -0400 (otro uso del banco); no es fallo del firmware
