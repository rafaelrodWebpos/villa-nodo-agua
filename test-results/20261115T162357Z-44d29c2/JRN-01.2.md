# JRN-01.2 — Aprovisionar por el portal

- AP desechable del banco con SSID y clave aleatorios de esta ejecución (redactados en la evidencia)
- broker `192.168.4.1:1883` (el del banco en la red de su AP, JRN-01.3), sitio `sitio1`, zona `zona1` (ficticios, NFR-SEC-01)

- Precondiciones: AP y broker del banco apagados; el DUT sigue en PORTAL (JRN-01.1)
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (16:28:36.488 → 16:28:46.998 UTC)
- POST / con los datos del AP desechable
- respuesta al envío: HTTP 200, probando=True
- El banco levanta su AP desechable (canal 1); el DUT debe unirse, guardar y reiniciar
- log: `JRN-01.2-log.log` (217 líneas)
- T04 → T05 en 4.2 s; reinicio (rst:) a los 4.0 s de T05 (límite 5 s)
- tras el reinicio: T01 y T08 a los 1.8 s del boot:

**Resultado: PASS** — aprovisionado: T04 → T05, reinicio a los 4.0 s, unido al AP del banco (T08)
