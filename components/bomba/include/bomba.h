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

/* ---- Permiso de la bomba (FSD §6.1–§6.2, B01–B16) ------------------------------------ */
typedef enum { BOM_PERMITIDA, BOM_CORTADA, BOM_BLOQUEADA } bom_estado_t;
typedef enum { BMOT_NINGUNO, BMOT_SECO, BMOT_NIVEL, BMOT_FLOTADOR, BMOT_ORDEN } bom_motivo_t;
typedef enum {
    BEV_SECO,               /* trabajo en seco declarado (FR-BOM-03, sec_muestra) */
    BEV_FLOTADOR_SIN_AGUA,  /* el flotador pasa a «sin agua» */
    BEV_NIVEL,              /* lectura de nivel (B03 si es válida y < nivel_critico_pct) */
    BEV_CORTAR,             /* orden `cortar` */
    BEV_REARME_CUMPLIDO,    /* condición de rearme cumplida (FR-BOM-05, rea_tick) */
    BEV_REARMAR,            /* orden `rearmar` (B08, B12–B14, B16) */
    BEV_PERMITIR,           /* orden `permitir` */
    BEV_PULSACION,          /* pulsación larga (B07, B08, B16; FR-LOC-03) */
    BEV_N_EVENTOS
} bom_evento_t;

typedef struct {
    bom_evento_t evento;
    bool flotador_agua;
    bool nivel_valido;      /* transductor sin falla (FR-NIV-02) */
    float nivel_pct;
    uint32_t t_s;           /* reloj monótono, s */
} bom_entrada_t;

#define BOM_MAX_CORTES 8    /* intentos_max ≤ 5 (§19) */
typedef struct {
    bom_estado_t estado;
    bom_motivo_t motivo;            /* solo en CORTADA */
    uint32_t cortes[BOM_MAX_CORTES];/* instantes de los cortes por seco (B01), para las 24 h móviles */
    int n_cortes;
    int intentos_max;               /* P6 */
    int nivel_critico_pct;          /* P6 */
    bool hay_rearme;                /* hubo un rearme (B05, B07, B12) y rearme_t vale (P20) */
    uint32_t rearme_t;              /* instante del último rearme, s */
} bom_t;

#define BOM_P20_S 600u              /* P20: un seco a 10 min o menos de un rearme bloquea (B15) */

typedef enum { BRECH_NINGUNO, BRECH_SIN_AGUA, BRECH_NIVEL_BAJO, BRECH_BLOQUEADA } bom_rechazo_t;
typedef struct {
    int fila;                       /* 1…16 = B01…B16 (FSD 0.8.9); 0 = par ignorado */
    bom_rechazo_t rechazo;
    bool evento_bloqueada;          /* B06, B13, B15: publicar `bomba_bloqueada` */
} bom_accion_t;

void bom_iniciar(bom_t *b, int intentos_max, int nivel_critico_pct);
bom_accion_t bom_evento(bom_t *b, const bom_entrada_t *e);
/* intentos_24h: cortes por seco en las últimas 24 h hasta t_s */
int bom_intentos_24h(bom_t *b, uint32_t t_s);
/* P20 / FR-PER-01 (Q-06): hay un rearme de hace 10 min o menos en t_s */
static inline bool bom_rearme_reciente(const bom_t *b, uint32_t t_s)
{ return b->hay_rearme && t_s - b->rearme_t <= BOM_P20_S; }
/* FR-BOM-01: relé energizado (GPIO14 alto) salvo en PERMITIDA */
static inline bool bom_rele(const bom_t *b) { return b->estado != BOM_PERMITIDA; }

/* FR-BOM-03 (P6): trabajo en seco = I ≥ i_marcha y I < i_seco y P < p_min durante más de
 * t_seco_s seguidos. Con i_seco ≤ 0 (sin configurar) la detección está inactiva (P6). */
typedef struct { bool en_curso; uint32_t desde_ms; } sec_t;
bool sec_muestra(sec_t *s, uint32_t t_ms, float i_a, float p_bar,
                 float i_marcha, float i_seco, float p_min, uint32_t t_seco_s);

/* FR-BOM-05 (P6): nivel válido ≥ nivel_rearme_pct y flotador con agua, sin interrupción,
 * durante t_rearme_s. Devuelve true una vez al cumplirse y vuelve a contar desde cero. */
typedef struct { bool en_curso; uint32_t desde_s; } rea_t;
bool rea_tick(rea_t *r, uint32_t t_s, bool nivel_valido, float nivel_pct, bool flotador_agua,
              int nivel_rearme_pct, uint32_t t_rearme_s);
