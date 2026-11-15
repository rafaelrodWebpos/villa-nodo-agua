/* L2 fugas — núcleo puro de la protección de fugas (FSD §5). Sin ESP-IDF; lo compila tests/host. */
#pragma once
#include <stdbool.h>

#define FUG_MUESTRAS_CONFIRMAR 4   /* FR-FUG-01 (P2): 4 muestras seguidas en mojada, cada 250 ms */

/* Contador de muestras seguidas en «mojada» de una sonda */
typedef struct { int seguidas; } fug_sonda_t;

/* FR-FUG-01: añade una muestra (mojada o no) y devuelve si la fuga queda confirmada.
 * Cualquier muestra que no sea «mojada» reinicia la cuenta. */
bool fug_muestra(fug_sonda_t *s, bool mojada);

/* FR-SON-01 (P4, FSD 0.4.1): clase de un lazo según su fracción de la referencia; cada
 * banda incluye su borde superior: ≤ 0,05 corto; (0,05–0,40] mojada; (0,40–0,60] seca;
 * (0,60–0,90] indeterminada; > 0,90 abierta. */
typedef enum { SON_CORTO, SON_MOJADA, SON_SECA, SON_INDETERMINADA, SON_ABIERTA } son_clase_t;
son_clase_t son_clasificar(float fraccion);
const char *son_nombre(son_clase_t c);
