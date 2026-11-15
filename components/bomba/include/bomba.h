/* L2 bomba — núcleo puro de bomba y cisterna (FSD §6). Sin ESP-IDF; lo compila tests/host. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* FR-NIV-01: nivel = (I − 4 mA) / 16 mA × rango_cm, acotado a [0, rango_cm]; % sobre
 * util_cm, acotado a [0, 100]. FR-NIV-02 (P18): I < 3,8 mA o > 20,5 mA → falla (devuelve
 * false y no escribe cm ni pct). */
bool bom_nivel(float ma, int rango_cm, int util_cm, float *cm, float *pct);

/* FR-PRE-01 (P9): presión = (V − 0,5) / 4,0 × 10 bar; V < 0,3 o > 4,7 → falla (false). */
bool bom_presion(float v, float *bar);

/* FR-CIC-01/02 (P7): arranques en una ventana móvil y un aviso por ventana */
#define CIC_MAX_ARRANQUES 30          /* ciclado_max ≤ 30 (§19) */
typedef struct {
    uint32_t t[CIC_MAX_ARRANQUES];    /* instantes de los arranques en la ventana, en s */
    int n;
    bool avisado;                     /* ya hubo aviso en esta ventana */
    uint32_t aviso_hasta;             /* fin de la ventana del último aviso */
} cic_t;

/* Registra un arranque en t_s (monótono). Devuelve true si toca publicar ciclado_anormal
 * (≥ max arranques en los últimos ventana_s), como mucho una vez por ventana; *n queda
 * con los arranques en la ventana. */
bool cic_arranque(cic_t *c, uint32_t t_s, uint32_t ventana_s, int max, int *n);
