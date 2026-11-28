# Revisión independiente de la puerta «Load defined» (5.ª, sobre el diff) — 2026-11-28

- **Commit revisado:** `6c9bd57` (FSD 0.8.8). **Alcance** (propietario): solo `git diff 5b5b9a1 HEAD` del FSD y la puerta. **Veredicto: SHUT** por S-01 y S-02 (altas).
- **Método:** revisor nuevo, sin memoria ni las revisiones anteriores; solo la puerta, el repo y el FSD del piloto (`ddf8016`, solo lectura).
- Los hallazgos son datos: Claude comprobó S-01 (V15 no cambia `cerrada_por`; V30, l.564, abre con `cerrada_por`=`ejercicio` y final «cerrada», contra FR-VAL-10, l.612) y S-02 (el contrato de clima FR-CON-01 exige OPERATIVO y un caso con el broker parado; clima §3 lo cierra en la Fase 2, l.382). Decide el propietario.

## Comprobaciones
Las mecánicas pasan: 113 IDs, todos con contrato. FR-LOC-06, los riesgos nuevos de §21.1, las prioridades B14/B16 y B16/B07–B13, el flotador en falla, FR-MED-10/FR-VIG-04 y Q-09 se encontraron bien. Sin datos de la vivienda en lo añadido.

## Hallazgos
| ID | Sev. | FSD | Qué falla | Arreglo propuesto |
|---|---|---|---|---|
| S-01 | alta | l.564 (V30) vs l.612 (FR-VAL-10) | Ejercicio en FALLA, cierre con la palanca (V15 → CERRADA, sigue `cerrada_por`=`ejercicio`): V30 abre con `g_seco`, mueve el motor tras un movimiento manual y da agua a una tubería en reparación; `cerrar` en CERRADA se ignora | V15 pone `cerrada_por`=`ninguno` al llegar a CERRADA, o V30 con la guarda de palanca de V24; contrato bench: IN2=0 durante 60 s |
| S-02 | alta | l.335 (§3.1) | FR-CON-01 en la Fase 1 «con el broker apagado», pero su contrato heredado exige OPERATIVO, un caso con el broker parado y `uptime_s` por MQTT; clima §3 lo cierra en la Fase 2. La fila host de FR-CON-02 sin fase | FR-CON-01 a la Fase 2 (o variante de Fase 1 solo con el caso A); fila host de FR-CON-02 a la Fase 1 |
| S-03 | media | l.715–716, 1238 | «El portal no admite vaciar `i_seco_a`» sin contrato ni regla en §19 | Contrato bench (vacío → error del campo, NVS sin cambio) y «no vacío si ya hay valor» en el rango |
| S-04 | media | l.763 | Contrato host de FR-BOM-10 sin la secuencia de `rearmar`/`permitir` | (a) B05 a 10 + `t_rearme_min`; (b) `cortar`, `permitir`; (c) `cortar`, `rearmar`; (d) transductor en falla |
| S-05 | baja | l.556, 558 | «Un reintento por episodio» sin definir el episodio | «hasta que la condición deje de cumplirse o la válvula salga de FALLA por otra fila» |
| S-06 | baja | l.764 | Prohibido «relé bajo antes de 10 + `t_rearme_min`» choca con la tolerancia ± 10 s; A9 sin tolerancia | «antes de 10 min + `t_rearme_min` − 10 s»; A9 ± 10 s |
| S-07 | baja | l.867, 1081 | El contrato de FR-MED-10 no comprueba `hasta_min` ni si incluye el margen; el campo no está en la tabla de interfaces | `hasta_min` = min + margen; documentarlo en `evento` |
| S-08 | baja | l.1327 | Con otro broker se pierden también FR-VIG-02 y FR-VIG-04, no solo FR-VIG-03 | Nombrarlos en el riesgo aceptado |
