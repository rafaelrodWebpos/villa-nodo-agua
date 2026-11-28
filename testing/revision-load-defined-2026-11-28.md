# Revisión independiente de la puerta «Load defined» — 2026-11-28

- **Commit revisado:** `e69c003` (FSD 0.8.4). **Veredicto: SHUT.**
- **Método:** revisor nuevo, sin memoria ni relato del autor, solo con `testing/gates/01-load-defined.md` y el repo (`auditoria-seguridad.md`, «Quién»). Solo lectura.
- Los hallazgos son datos: Claude comprobó G-01, G-04 y G-05 contra el FSD. Decide el propietario.

## Comprobaciones mecánicas
Planos presentes; sin `(propuesto`, `(assumed)` ni `{{…}}` sueltos; 109 IDs, todos con contrato.

## Requisitos 1–7
Cumplen 1, 3, 4, 6 y 7; el 5 cumple con reservas (G-08, G-09). **El 2 no cumple:** la tabla de §6.2 (B01–B11) no tiene la columna «Límite».

## Hallazgos
| ID | Sev. | FSD | Qué falla | Arreglo propuesto |
|---|---|---|---|---|
| G-01 | crítica | FR-CFG-A3, §19 `sonda_activa`, FR-SEC-A1 | Con las sondas secas, ocho `config/set` `sonda_activa[n]`=false apagan el cierre por fuga, sin evento ni aviso; FR-SEC-A1 solo cubre la «sonda activa mojada» | `sonda_activa` solo por el portal o con el botón local, o un mínimo de sondas activas; evento y aviso; ampliar FR-SEC-A1 |
| G-02 | alta | §19 `i_marcha_a`, `i_seco_a` | Los rangos permiten anular el corte en seco (`i_marcha_a`=10 A, o una ventana vacía) | Restricciones cruzadas o solo por el portal; caso de fuzz en seco en FR-SEC-A1 |
| G-03 | alta | §19 `rango_transductor_cm`/`altura_util_cm`, FR-BOM-10 | Un nivel falso por configuración provoca `flotador_falla`: se ignora el flotador y la bomba queda PERMITIDA con la cisterna vacía (viola FR-BOM-08) | Que `flotador_falla` no salte por un cambio de configuración, o esos parámetros solo por el portal |
| G-04 | alta | P20, B07, FR-BOM-07 | P20 no tiene guarda en §6.2; B07 pone `intentos_24h`=0 con un `rearmar` remoto, así que con `rearmar` repetido la bomba nunca se bloquea | Guarda P20 en B01 → BLOQUEADA; un `rearmar` remoto no pone el contador a cero, o de BLOQUEADA solo se sale en local |
| G-05 | alta (req. 2) | §6.2 | B01–B11 sin la columna «Límite» | Añadirla |
| G-06 | media | FR-BOM-03 | No dice qué detecta mientras `i_seco_a` no está configurado (así sale de fábrica) | Decirlo y llevarlo al contrato |
| G-07 | media | V08, V14, V22 | Con batería crítica y sin red, `abrir` o el ejercicio reabren y V22 vuelve a cerrar: un bucle que gasta batería y motor | Guarda «no (batería crítica y red ausente)» en V08, V10 y V14 |
| G-08 | baja | §3.1 | La Fase 1 entrega A1–A7 pero sale con A1–A8; FR-OBS-A1 incluye §13 (MQTT) | Igualar y separar el caso de §13 para la Fase 2 |
| G-09 | baja | FR-VAL-10, FR-BOM-05 | Observación por evento que puede ser MQTT; «configuración de prueba» sin definir | Observar por A1 o GPIO; constante de build del banco |
| G-10 | baja | V04/V18, §6.2 | Fila ABRIENDO × fuga repetida; §6.2 sin regla de completitud | Quitar la repetida y escribir la regla |
