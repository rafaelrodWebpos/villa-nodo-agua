/* L1 portal — parámetros del nodo en el portal (FSD §19, interfaz `portal`; §15, FR-CFG-A3,
 * FR-BOM-03/Q-09, FR-LOC-06). Núcleo puro. */
#pragma once
#include <stdbool.h>
#include "portal_form.h"

typedef struct {
    bool sonda_activa[8];
    int rango_cm, altura_cm;           /* rango_transductor_cm, altura_util_cm */
    float i_marcha_a, i_seco_a;        /* i_seco_a ≤ 0: sin configurar */
} portal_nodo_t;

void portal_nodo_fabrica(portal_nodo_t *p);          /* §19: todas activas, 500, 200, 0,5, sin configurar */
/* ¿Trae el envío los parámetros del nodo? (campo oculto `nodo`; un envío sin él los deja como están) */
bool portal_nodo_presente(const char *const v[PORTAL_N_CAMPOS]);
/* Valida los del nodo contra el valor actual y las sondas mojadas; escribe los errores por campo
 * y, si no hay ninguno, el resultado en *out. Devuelve el número de errores. */
int portal_nodo_validar(const char *const v[PORTAL_N_CAMPOS], const portal_nodo_t *actual, const bool mojada[8],
                        const char *error[PORTAL_N_CAMPOS], portal_nodo_t *out);
/* Rellena v[] con el valor actual (para que el portal lo muestre, FR-LOC-06); txt guarda los números */
void portal_nodo_valores(const portal_nodo_t *p, char txt[4][16], const char *v[PORTAL_N_CAMPOS]);
/* ¿Es un portal_nodo_t leído de la NVS válido? (si no, se usan los de fábrica) */
bool portal_nodo_valido(const portal_nodo_t *p);
