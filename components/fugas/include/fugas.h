/* L2 fugas — núcleo puro de la protección de fugas (FSD §5). Sin ESP-IDF; lo compila tests/host. */
#pragma once
#include <stdbool.h>

#define FUG_MUESTRAS_CONFIRMAR 4   /* FR-FUG-01 (P2): 4 muestras seguidas en mojada, cada 250 ms */

/* Contador de muestras seguidas en «mojada» de una sonda */
typedef struct { int seguidas; } fug_sonda_t;

/* FR-FUG-01: añade una muestra (mojada o no) y devuelve si la fuga queda confirmada.
 * Cualquier muestra que no sea «mojada» reinicia la cuenta. */
bool fug_muestra(fug_sonda_t *s, bool mojada);
