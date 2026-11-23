# TC-LOG-08 — Log UDP solo por unicast al receptor

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)
- broker del banco (abierto); tópicos bajo `sitio1/zona1/agua-7f9af8/` y `homeassistant/+/agua-7f9af8/`

- Precondiciones: grabar la imagen de la CI en SLOT2
- Precondiciones: P-NVS-ERASE y reset; el DUT debe arrancar en PORTAL
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (01:59:29.870 → 01:59:40.249 UTC)
- Precondiciones: POST / con los datos de un AP desechable nuevo
- precondición: DUT configurado contra un AP desechable nuevo (redactado); AP del banco apagado

- Captura del tráfico del DUT durante el arranque y 3 ciclos

**Resultado: FAIL** — AssertionError: sin T11
assert None
 +  where None = esperar('con:\\ CONECTANDO_BROKER\\ \\->\\ OPERATIVO\\ \\(mqtt_up\\)', 150)
 +    where esperar = <benchlib.Registro object at 0x70227e9a6630>.esperar
 +    and   'con:\\ CONECTANDO_BROKER\\ \\->\\ OPERATIVO\\ \\(mqtt_up\\)' = <function escape at 0x70227fac9580>('con: CONECTANDO_BROKER -> OPERATIVO (mqtt_up)')
 +      where <function escape at 0x70227fac9580> = re.escape
