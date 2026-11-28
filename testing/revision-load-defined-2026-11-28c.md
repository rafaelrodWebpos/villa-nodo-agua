# Revisión independiente de la puerta «Load defined» (3.ª) — 2026-11-28

- **Commit revisado:** `2751360` (FSD 0.8.6). **Veredicto: SHUT** por Q-01 (alta).
- **Método:** revisor nuevo, sin memoria ni las revisiones anteriores; solo `testing/gates/01-load-defined.md`, el repo y el FSD heredado del piloto (`ddf8016`, solo lectura). Incluye la revisión de seguridad expresa de FR-LOC-06.
- Los hallazgos son datos: Claude comprobó Q-01 contra el FSD (l.879–881: los parámetros del portal vuelven a su valor por defecto; l.1201: `i_seco_a` por defecto «sin configurar»; l.730: sin `i_seco_a` no hay corte por corriente). Decide el propietario.

## Comprobaciones
Las 5 mecánicas pasan: 111 IDs, todos con contrato. Los requisitos 1–7 se cumplen. Contratos al azar montables (FR-FUG-02, FR-BOM-03, FR-MED-03, FR-CON-A2; FR-LOC-06 con la salvedad Q-06). Orden de fases correcto. Sin palabras vagas en requisitos. Sin datos de la vivienda. Por MQTT no se encontró forma de saltarse fuga, seco, nivel crítico ni BLOQUEADA.

## Hallazgos
| ID | Sev. | FSD | Qué falla | Arreglo propuesto |
|---|---|---|---|---|
| Q-01 | alta | l.879, 1201, 730, 1281–1286 | FR-LOC-06 devuelve `i_seco_a` a «sin configurar» (se apaga el corte por seco) y `rango`/`altura` a 500/200 (B03 actúa a otra altura real; posible `flotador_falla` falso). Se dispara con la misma botonera que la pulsación larga. §21.1 no acepta perder la calibración; el contrato no lo comprueba | Conservar `i_marcha_a`, `i_seco_a`, `rango_transductor_cm` y `altura_util_cm` (el portal los muestra rellenos), o evento + LED «sin calibrar» y riesgo aceptado expreso; contrato: «calibración igual tras aprovisionar; un seco simulado sigue cortando» |
| Q-02 | media | l.879, 1283–1285; clima FR-POR-01/08 | Tras la pulsación, el portal es un AP abierto 10 min: cualquiera al alcance puede aprovisionar antes (sondas, calibración, otro broker; con otro broker el aviso FR-VIG-03 no llega) | Reconocer la carrera en §21.1; confirmación local para parámetros de seguridad, o dejarlos fuera del borrado |
| Q-03 | media | l.879, 904; V24 | `sonda_activa`→true hace que las sondas no cableadas lean `abierta`; con `casa_cerrada`=true, V24 cierra la válvula al arrancar, sin aviso ni contrato | Conservar `sonda_activa`, o documentarlo y añadir contrato |
| Q-04 | media | l.713, 739–740, 661, 665 | FR-BOM-10: el contrato espera PERMITIDA a los 10 min, pero B05 daría ~20 min; el flotador en falla se trata «con agua» en B02/B05/B07/B09 y no en B08/B12/B16 | Fijar desde cuándo cuenta B05; ampliar o excluir expresamente B08/B12/B13/B16 |
| Q-05 | baja | l.877, 879 | FR-LOC-06 no dice si actúa a los 10 s pulsado o al soltar, ni qué pasa si se ignora con la válvula en movimiento | Fijar el momento; «ignorada: ni A10 ni acción de FR-LOC-03» |
| Q-06 | baja | l.904, 865 | `intentos_24h` y `cerrada_por` se observan en la Fase 1 sin línea A que los muestre | Añadirlos a A1/A2/A5, o mover la observación a la Fase 2 |
| Q-07 | baja | l.543–545 | Línea en blanco entre V27 y V28 parte la tabla | Quitarla |
| Q-08 | baja | l.538, 540 | V22 y V24 no salen de FALLA (la fuga sí, V20) | Añadir FALLA como origen con reintento, o anotar que se ignora a propósito |
