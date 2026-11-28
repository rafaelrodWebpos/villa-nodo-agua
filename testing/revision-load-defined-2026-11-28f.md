# Revisión independiente de la puerta «Load defined» (6.ª, sobre el diff) — 2026-11-28

- **Commit revisado:** `b78bd29` (FSD 0.8.9). **Alcance** (propietario): solo `git diff 6c9bd57 HEAD` del FSD y la puerta. **Veredicto: OPEN** — sin hallazgos críticos ni altos.
- **Método:** revisor nuevo, sin memoria ni las revisiones anteriores; solo la puerta, el repo y el FSD del piloto (`ddf8016`, solo lectura).
- **Regla del propietario (2026-11-28):** sin críticos ni altos, la puerta pasa a OPEN y los medios y bajos quedan registrados para cerrarlos en el firmware (o por decisión del propietario: U-02, U-03).

## Comprobaciones
Las mecánicas pasan: 113 IDs, todos con contrato. El cambio de V15 solo quita caminos de reapertura automática (V14, V23, V27, V29, V30); `bloqueo_fuga` se conserva; ningún MQTT provoca V15. El reparto de clima:FR-CON-* coincide con clima §3. Sin datos de la vivienda.

## Hallazgos abiertos (para el firmware)
| ID | Sev. | FSD | Qué falla | Arreglo propuesto |
|---|---|---|---|---|
| U-01 | media | l.638 | La fila de banco nueva de FR-VAL-10 parte de un ejercicio, que solo existe en la Fase 2 (SNTP u orden `ejercicio`) | Marcarla Fase 2, o una fila de Fase 1 desde FALLA por otro motivo |
| U-02 | media | l.639; V22 l.564 | La fila host no ordena red y palanca; V22 (de nivel, sin guarda de palanca) cierra con el motor tras la palanca y choca con FR-VAL-10 (choque previo) | «red presente antes de mover la palanca»; decisión: guarda de palanca en V22 o excepción en FR-VAL-10 |
| U-03 | media | l.549 (V15) | Un final tardío del motor tras V07 cuenta como palanca: un ejercicio lento deja la villa cerrada hasta una orden | Aceptación escrita en §5.3, o umbral de N s tras parar el motor |
| U-04 | baja | l.549 | «solo una orden abre» olvida V10 y V16 | «solo V08, V10 o V16 abren» |
| U-05 | baja | l.764 | La fila nueva de FR-BOM-03 prohíbe `calibrada`=false sin «en la Fase 2»; «como antes» vago | «en la Fase 2, además»; observar en el portal y repitiendo l.763 |
| U-06 | baja | l.1250; §21.1 | «No vacío» no impide `i_seco_a` apenas mayor que `i_marcha_a` | Separación mínima, o decir que solo evita vaciarlo sin querer |
| U-07 | baja | l.370 | Salida de la Fase 2 cita FR-MED-04…09 | FR-MED-04…10 |
| U-08 | baja | l.549, 1080 | Tras la palanca con fuga, `cerrada_por`=`ninguno` con `bloqueo_fuga`=true; HA pierde la causa | Anotarlo como comportamiento esperado |
