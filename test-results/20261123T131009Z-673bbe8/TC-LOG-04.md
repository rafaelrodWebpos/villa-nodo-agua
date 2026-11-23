# TC-LOG-04 — Sin log UDP en producción

- imagen: `0.0.0-673bbe8` (variante production, run CI 45156832063)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (13:10:47.722 → 13:10:58.128 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Captura del tráfico del DUT durante el arranque y 3 ciclos (imagen de producción)
- log: `TC-LOG-04-serie.log` (282 líneas)

**Resultado: FAIL** — AssertionError: sin T11
assert None
 +  where None = esperar('con:\\ CONECTANDO_BROKER\\ \\->\\ OPERATIVO\\ \\(mqtt_up\\)', 150)
 +    where esperar = <benchlib.Registro object at 0x7a40b2503800>.esperar
 +    and   'con:\\ CONECTANDO_BROKER\\ \\->\\ OPERATIVO\\ \\(mqtt_up\\)' = <function escape at 0x7a40b2e29580>('con: CONECTANDO_BROKER -> OPERATIVO (mqtt_up)')
 +      where <function escape at 0x7a40b2e29580> = re.escape
