# Revisión independiente de la puerta «Load defined» (4.ª) — 2026-11-28

- **Commit revisado:** `9508843` (FSD 0.8.7). **Veredicto: SHUT** por Q-01 (alta).
- **Método:** revisor nuevo, sin memoria ni las revisiones anteriores; solo `testing/gates/01-load-defined.md`, el repo y el FSD heredado del piloto (`ddf8016`, solo lectura). Incluye la revisión de seguridad expresa de FR-LOC-06.
- Los hallazgos son datos: Claude comprobó Q-01 contra el FSD (l.750 espera relé en bajo al salir `flotador_falla`; l.729–730 y el contrato host l.749 dan PERMITIDA a los 10 min + `t_rearme_min`). Fue una omisión al aplicar Q-04 de la 3.ª revisión. Decide el propietario.

## Comprobaciones
Las 5 mecánicas pasan: 111 IDs, todos con contrato. Los requisitos 1–7 se cumplen (el 5 con la reserva Q-03). Contratos al azar montables (FR-VAL-05, FR-BOM-04, FR-SON-05, FR-MED-01), salvo FR-BOM-10 banco (Q-01). Sin palabras vagas en requisitos. Sin datos de la vivienda. Por MQTT no se encontró forma de abrir con fuga, saltarse `g_ciego`/`g_bateria`, salir de BLOQUEADA ni anular el corte por seco o nivel crítico. FR-LOC-06 borra solo Wi-Fi, MQTT, `zona` y `sitio`, y está separada de FR-LOC-03.

## Hallazgos
| ID | Sev. | FSD | Qué falla | Arreglo propuesto |
|---|---|---|---|---|
| Q-01 | alta | l.750 vs 729–730, 749 | El contrato de banco de FR-BOM-10 exige relé en bajo al salir `flotador_falla`; B05 da PERMITIDA a los 10 min + `t_rearme_min`. Un firmware correcto falla la fila | «A9 `eve: flotador_falla` a los 10 min; relé en bajo a los 10 min + `t_rearme_min` ± 10 s»; prohibido «relé bajo antes; relé alto después» |
| Q-02 | media | l.917 | El contrato de FR-LOC-06 dice «un seco simulado sigue cortando (B01)», pero parte de CORTADA(seco) y no dice cómo se llega a PERMITIDA; según el rearme puede tocar B15 o B07 pone intentos a 0 | Fijar la secuencia (esperar B05, seco a más de 10 min, B01 e `intentos`=3) o «B01 o B15 según P20» |
| Q-03 | media | l.326–327 | La salida de la Fase 1 pide en verde «clima:FR-CON-* heredados» con el broker apagado; FR-CON-03/04/05 publican por MQTT | Listar por ID: Fase 1 = FR-CON-01, fila Wi-Fi de FR-CON-02, FR-CON-06 sin broker, FR-CON-07; Fase 2 = FR-CON-03/04/05 |
| Q-04 | baja | l.674–675, 685 | B14 y B16 se solapan (BLOQUEADA, `rearmar`, nivel bajo): motivo ambiguo | Fijar la prioridad |
| Q-05 | baja | l.595 vs 525, 552 | FR-VAL-07 pide `fuga_activa` en cualquier estado; la tabla solo desde CERRADA | Acotar a CERRADA o añadir filas |
| Q-06 | baja | l.926–928, 661, 676 | P20 necesita el instante del último rearme y FR-PER-01 no lo guarda; tras un reinicio un seco cae en B01 en vez de B15 | Guardarlo en FR-PER-01, tratado como al arrancar |
| Q-07 | baja | l.433, 889 | El umbral de 10 s no tiene fila en §4.4; D16 solo dice 3 s | Fila de decisión (R-08) o enmendar D16 |
| Q-08 | baja | l.1301 | Con FR-POR-08, cada reinicio sin aprovisionar reabre el AP 10 min más | Decirlo en §21.1 |
| Q-09 | baja | l.1215, 700–702 | No se define si el portal puede vaciar `i_seco_a` ni el criterio de seco sin calibrar | El portal no admite vaciarlo; aclarar el criterio |
| Q-10 | baja | l.887–888 | Una pulsación larga ejecuta una fila de válvula y otra de bomba; no dice qué patrón gana si una se rechaza | Explicitarlo |
| Q-11 | baja | l.817–821, 1289 | `medidor/set` puede alargar sin límite el periodo y silenciar `consumo_casa_cerrada` con credenciales del broker | Riesgo aceptado en §21.1 o evento al abrir un periodo |
| Q-12 | baja | l.530, 551, 553 | Final «cerrada» del ejercicio sin `g_seco`: ninguna de V14/V27/V29 aplica y queda `cerrada_por`=`ejercicio` | Reevaluar V14 mientras `cerrada_por`=`ejercicio` |
