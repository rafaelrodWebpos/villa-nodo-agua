# TC-CON-07 — Ocupado mientras valida

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- red del envío: ficticia (`red-que-no-existe`), así que el DUT sigue en VALIDANDO hasta T06

- Precondiciones: grabar la imagen de la CI en SLOT2, P-NVS-ERASE y PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (00:36:12.337 → 00:36:24.065 UTC)
- Primer POST / válido (T04) y, enseguida, un segundo POST (T07)
- log: `TC-CON-07-serie.log` (26 líneas)

## Resultado

- 1.er POST: HTTP 200, probando=True
- 2.º POST: HTTP 200, ocupado=True
- líneas T04: 1, T07: 1, «wifi: intento 1»: 1
- T06 al final: sí

**Resultado: PASS** — el segundo envío en VALIDANDO respondió «ocupado», con su línea T07 y sin segunda prueba
