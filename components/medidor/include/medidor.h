/* L2 medidor — periodo de consumo permitido (FR-MED-06, P32) y su evento (FR-MED-10, Q-11).
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. El conteo de pulsos y el tópico `medidor/set`
 * llegan en su propio PR (FR-MED-01…05, Fase 2). */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define MED_MIN_MAX 720            /* `min` admitido: 1…720 (FR-MED-06) */

typedef struct {
    bool hay;                      /* hay un periodo abierto; vive en RAM: un reinicio lo borra */
    uint32_t fin_s;                /* fin del periodo, reloj monótono en s */
} med_periodo_t;

typedef enum {
    MED_RECHAZADA,                 /* payload no válido: orden_rechazada (FR-ORD-02); el periodo no cambia */
    MED_SIN_CAMBIO,                /* aceptada, pero no mueve el fin (una orden nueva nunca acorta) */
    MED_EVENTO,                    /* abre el periodo o mueve su fin: evento `consumo_permitido` (FR-MED-10) */
} med_resultado_t;

/* `permitir_consumo` con `min` minutos en ahora_s: periodo hasta ahora + min + margen_min.
 * Con MED_EVENTO, *hasta_min = minutos desde ahora hasta el fin, incluido el margen (S-07). */
med_resultado_t med_permitir(med_periodo_t *p, uint32_t ahora_s, int min, int margen_min, int *hasta_min);

/* ¿ahora_s cae dentro del periodo permitido (con su margen)? */
bool med_dentro(const med_periodo_t *p, uint32_t ahora_s);
