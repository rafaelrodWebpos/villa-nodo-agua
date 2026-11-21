# Nodo Agua — Manual de usuario

Cómo instalar y operar el nodo. Son procedimientos para personas, en presente.
El FSD dice qué hace el sistema; este manual, cómo hacer que lo haga.

**Estado: todavía no hay nada desplegable.** No existe firmware. Los capítulos
se llenan a medida que cada fase del FSD (§3) se completa.

## Contenido

1. Qué necesitas
2. Montaje (fuente de 12 V con batería, válvula, contactor de la bomba, sondas, caja IP65)
3. Primer flasheo
4. Aprovisionamiento por el portal
5. Ver los datos en Home Assistant
6. Puesta en marcha: calibrar la bomba y el nivel
7. Qué hacer ante una fuga (LED, zumbador, reapertura en persona)
8. Rearmar la bomba
9. Cambiar la batería
10. Actualizar por OTA
11. Solución de problemas y recuperación

## 2. Montaje — cableado del nodo (lado del dispositivo)

Qué cable y qué tubo necesita cada señal del nodo. Las rutas por la vivienda, las losas, las
zapatas y la etapa de obra **no van aquí**: el repositorio es público y describe el
dispositivo, no la vivienda (NFR-SEC-01). Están en la documentación privada del propietario.

| Señal (FSD) | Extremos | Cable | Hilos | Tubo |
|---|---|---|---|---|
| Nivel 4–20 mA (C-HW-27) | Transductor sumergido → gabinete | El del transductor, con tubo de venteo, sin empalmes | 2 + venteo | ¾", compartido con el flotador |
| Flotador de nivel mínimo (GPIO10) | Depósito → gabinete | De goma, 2 × 0,75 mm² | 2 | El del nivel |
| Presión de salida 0,5–4,5 V (C-HW-28) | Colector de salida → gabinete | Apantallado 3 × 0,34 mm² | 3 | ¾", compartido con la pinza y el permiso |
| Corriente de la bomba (C-HW-25) | Pinza en un conductor de fase, dentro de la caja del contactor → gabinete | Apantallado 2 × 0,34 mm² | 2 | El de la presión; nunca con la fuerza de la bomba |
| Permiso de la bomba, bobina de 12 V (C-HW-23) | Gabinete → contactor | 2 × 0,75 mm² | 2 | El de la presión |
| Válvula motorizada con finales (C-HW-21/22) | Gabinete → registro de la válvula | Motor 2 × 1,5 mm²; finales 3 × 0,5 mm² | 5 | ¾" |
| Sonda de fuga, lazo supervisado (C-HW-26), × 8 | Gabinete → caja baja junto al punto | Apantallado 2 × 0,34 mm², o un par de Cat6 | 2 | ½", uno por sonda, sin empalmes |
| Red presente (GPIO5) | Fuente de 12 V → nodo, dentro del gabinete | 2 × 0,34 mm² | 2 | — |
| Alimentación de la fuente de 12 V | Circuito respaldado por el inversor → gabinete | Según el electricista | — | Tubo eléctrico **separado** de la baja tensión |

Reglas: la baja tensión va siempre en tubo propio; las sondas, la válvula y los finales se
cablean sin empalmes (o con empalme en una caja seca y accesible); la entrada al depósito de
agua potable lleva un pasamuro estanco; y el venteo del transductor de nivel no se tapa.
