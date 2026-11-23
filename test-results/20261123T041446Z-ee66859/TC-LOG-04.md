# TC-LOG-04 — Sin log UDP en producción

- imagen: `0.0.0-ee66859` (variante production, run CI 45085044807)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (04:15:23.954 → 04:15:34.963 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Captura del tráfico del DUT durante el arranque y 3 ciclos (imagen de producción)

**Resultado: FAIL** — AssertionError: sin T11
assert None
 +  where None = esperar('con:\\ CONECTANDO_BROKER\\ \\->\\ OPERATIVO\\ \\(mqtt_up\\)', 150)
 +    where esperar = <benchlib.Registro object at 0x729bd49a0f20>.esperar
 +    and   'con:\\ CONECTANDO_BROKER\\ \\->\\ OPERATIVO\\ \\(mqtt_up\\)' = <function escape at 0x729bd525d580>('con: CONECTANDO_BROKER -> OPERATIVO (mqtt_up)')
 +      where <function escape at 0x729bd525d580> = re.escape
