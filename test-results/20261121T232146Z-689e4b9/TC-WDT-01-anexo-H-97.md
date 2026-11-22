# TC-WDT-01 — anexo: análisis del hueco de 240 s (decisión H-97)

El resultado original (`TC-WDT-01.md`, **FAIL** por un hueco entre status de 240 s que empieza
a las 2026-11-22 02:59:48 −0400) no se edita. Este anexo recoge el análisis y la decisión.

## Decisión del propietario (H-97, 2026-11-22)

Opción A: el hueco se acepta como **fallo de la infraestructura de prueba**, no del nodo.

## Evidencia

1. **La laptop perdió la red con el banco** de 03:00:29 a 03:03:34 (`TC-WDT-01-banco-transitorios.log`):
   19 errores `[Errno 101] Network is unreachable` en `serial/output`, `wifi/ap_status` y
   `mqtt/status`, y la recuperación a las 03:03:34. Fueron menos de 2 min sin respuesta en cada
   llamada, así que la prueba siguió. El suscriptor MQTT corre en la laptop y quedó sin red en el
   mismo tramo. Otros transitorios, aislados y recuperados al primer reintento: 00:42:12
   (`serial/output`), 05:19:43 y 14:18:31 (`wifi/ap_status`, `Connection reset by peer`).
2. **El nodo no se reinició ni tuvo task_wdt**: «reinicios del nodo: ninguno» en el resultado.
   La consola (`TC-WDT-01-serie.log`) sigue normal en ese tramo, y su única línea `rst:` es la
   0x15 (USB_UART_CHIP_RESET) del reset del banco al empezar la prueba.
3. **El nodo publicó durante el hueco**: al reconectar, el suscriptor recibió el status
   retenido con `uptime_s`=27600 (`TC-WDT-01-mqtt.log`, +27648 s, retain=1), publicado dentro del
   hueco. Después siguió el status en vivo con `uptime_s`=27660, sin salto.
4. **Los 5 cortes se recuperaron**: broker en 3,4 s (los tres) y Wi-Fi en 26,3 s y 26,1 s.

## Consecuencia

TC-WDT-01 sigue sin un resultado limpio. Pendiente, sin banco: que la prueba distinga una caída
del suscriptor de un ciclo sin publicar (status retenido y continuidad del uptime, o suscriptor
del lado de la Pi), y repetir la 24 h más adelante.
