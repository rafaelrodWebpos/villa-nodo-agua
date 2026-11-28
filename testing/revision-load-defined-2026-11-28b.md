# Revisión independiente de la puerta «Load defined» (2.ª) — 2026-11-28

- **Commit revisado:** `505e807` (FSD 0.8.5). **Veredicto: SHUT** por R-01 (alta).
- **Método:** revisor nuevo, sin memoria ni la revisión anterior; solo `testing/gates/01-load-defined.md`, el repo y el FSD heredado del piloto (`ddf8016`, solo lectura).
- Los hallazgos son datos: Claude comprobó R-01, R-02 y R-08 contra el FSD. Decide el propietario.

## Comprobaciones
Las mecánicas pasan: 110 IDs, todos con contrato. Los requisitos 1–7 se cumplen, aunque el 2 nombra rangos de filas desfasados (R-12). No se encontraron datos de la vivienda.

## Hallazgos
| ID | Sev. | FSD | Qué falla | Arreglo propuesto |
|---|---|---|---|---|
| R-01 | alta | B12, B07 frente a B11, B03 | Un `rearmar` remoto (B12), o la pulsación larga (B07), pone PERMITIDA desde CORTADA(nivel/orden) con el nivel bajo el crítico; esto se salta B11 y el corte por nivel. B03 no dice si es de nivel o de flanco. FR-SEC-A1 no cubre el nivel crítico | Rechazar con `nivel_bajo` en B12/B07 con transductor válido y nivel < crítico; declarar B03 de nivel; ampliar FR-SEC-A1 |
| R-02 | media | FR-ENE-05 frente a V23 | El requisito omite las guardas `g_seco` y no `g_ciego` que pone V23 | Copiarlas al requisito y al contrato |
| R-03 | media | V15 | En DESCONOCIDA o FALLA se ignora un cambio de final; `abrir` en DESCONOCIDA se ignora sin rechazo; no dice si un final es flanco o nivel | Extender V15; definirlo; fila de rechazo |
| R-04 | media | Completitud de §5.3/§6.2 frente a FR-ORD-01 | Una orden ignorada queda sin respuesta | Una orden ignorada publica su `…/state` o un rechazo `estado` |
| R-05 | media | FR-VAL-04, FR-SON-02, FR-IO-06, FR-BOM-10, FR-BOM-03 | Contratos de la Fase 1 que mezclan la observación local con un evento MQTT | «En la Fase 2, además…»; líneas A para `falla_sonda`, `falla_adc` y `flotador_falla` |
| R-06 | media | §3.2, FR-HER-02 | FR-HER-02 no aparece en ningún criterio de salida | Citarlo en la Fase 2 |
| R-07 | media | FR-ORD-03 | Un `retain` en `medidor/set` o `config/set` se vuelve a aplicar al reconectar | Extender FR-ORD-03, con contrato |
| R-08 | media | §19, FR-MED-02, FR-PER-01 | Volver al portal exige borrar toda la NVS por USB (piloto: el botón BOOT no se aceptó), y eso borra los bloqueos, los intentos y el total del medidor | Reentrada al portal que conserve esas claves, o aceptar la pérdida de forma explícita |
| R-09 | baja | V14, V27 | Con `g_ciego` durante el ejercicio, queda `cerrada_por`=`ejercicio` para siempre | Fila: `cerrada_por`=`sondas` |
| R-10 | baja | FR-LOC-03 | Faltan V16, V25 y V26 | Añadirlas |
| R-11 | baja | FR-EJE-04 | `posponer_ejercicio` puede aplazar sin límite | Tope o aviso |
| R-12 | baja | puerta:12, FSD §10 | Rangos de filas desfasados (V01–V27/V03b, B01–B15, T26 en el piloto) | Actualizar la puerta y §10 |
