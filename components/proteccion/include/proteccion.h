/* L2 proteccion — la tarea de protección sin hardware (FSD §5–§9, §12): une los núcleos
 * probados (fugas, valvula, bomba, energia, interfaz) con las lecturas de los drivers.
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. El adaptador lo llama cada 10 ms con
 * las entradas crudas y aplica las salidas; no hace llamadas de red (§2.3). */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "bomba.h"
#include "energia.h"
#include "es.h"
#include "fugas.h"
#include "interfaz.h"
#include "valvula.h"

#define PROT_N_SONDAS 8
#define PROT_PERIODO_MS 10u            /* paso de la tarea (FR-VAL-05: ≤ 100 ms con el antirrebote) */
#define PROT_FALLA_SONDA_MS 5000u      /* FR-SON-02 */
#define PROT_FALLA_VREF_MS 5000u       /* FR-SON-04 (FSD 0.7.1) */
#define PROT_CIEGO_MIN_SONDAS 2        /* FR-SON-05: N = 2 (H-93) */
#define PROT_FLOTADOR_FALLA_S 600u     /* FR-BOM-10: 10 min */

/* Valores de fábrica de §19 (la configuración por MQTT, §16, es de la Fase 2) */
typedef struct {
    bool sonda_activa[PROT_N_SONDAS];
    uint32_t t_carrera_max_s;
    int ejercicio_dias, ejercicio_hora;
    int nivel_critico_pct, nivel_rearme_pct, intentos_max;
    uint32_t t_rearme_s;
    int rango_transductor_cm, altura_util_cm;
    float i_marcha_a, i_seco_a, p_min_bar;   /* i_seco_a ≤ 0: sin configurar */
    uint32_t t_seco_s;
    int ciclado_max;
    uint32_t ciclado_ventana_s;
    bool zumbador_activo;
    uint32_t zumbador_min;
    bool casa_cerrada;                       /* FR-SON-05 (P22): de fábrica true */
} prot_config_t;
void prot_config_fabrica(prot_config_t *c);

/* Lo que se guarda en NVS (FR-PER-01) */
typedef struct {
    bool bloqueo_fuga;
    val_cerrada_por_t cerrada_por;
    bom_estado_t bomba;
    bom_motivo_t motivo;
    int n_cortes;          /* cortes por seco de las últimas 24 h */
    bool rearme_reciente;  /* rearme de hace 10 min o menos (P20; FR-PER-01, Q-06) */
} prot_persistente_t;

typedef struct {
    uint32_t t_ms;          /* reloj monótono en ms (las diferencias aguantan el desborde) */
    uint32_t t_s;           /* reloj monótono en s */
    /* digitales crudas, ya con su polaridad resuelta (true = activo) */
    bool fin_abierta, fin_cerrada, flotador_agua, red_presente, boton;
    /* medidas analógicas: solo cuando hay_medidas (cada 250 ms) */
    bool hay_medidas;
    bool ads_ok[ES_N_ADS];
    float v_sonda[PROT_N_SONDAS];   /* V en el ADS */
    float v_nivel, v_presion, v_bat, v_ref;   /* v_ref: A3 del 0x4A (FSD 0.6.0) */
    bool hay_corriente;
    float i_rms;
    /* hora (FR-EJE-01) y conectividad (LED, prioridad 4) */
    bool hora_valida;
    int hora_local;
    uint32_t ahora_s;
    bool conectado;
} prot_entrada_t;

typedef enum {
    PEV_VAL,             /* a = fila, b = desde, c = hacia, d = evento (línea A1) */
    PEV_BOM,             /* a = fila, b = desde, c = hacia, d = evento (línea A2) */
    PEV_SON,             /* a = sonda 1…8, b = clase o -1 si el ADS falla (línea A3) */
    PEV_FUGA,            /* a = sonda */
    PEV_FALLA_SONDA,     /* a = sonda, b = clase o -1 */
    PEV_FALLA_VALVULA,
    PEV_MOVIMIENTO_MANUAL,
    PEV_BOMBA_BLOQUEADA,
    PEV_CICLADO,         /* a = arranques, b = válvula abierta */
    PEV_BATERIA,         /* a = estado (ene_estado_t) */
    PEV_FALLA_ADC,       /* a = índice del ADS (0…2) */
    PEV_RECHAZO,         /* a = 1 válvula (fuga_activa) / 2 bomba (sin_agua) / 3 válvula (sondas_en_falla, V25) / 4 bomba (nivel_bajo, B16) por pulsación larga */
    PEV_FLOTADOR_FALLA,  /* a = 1 declarada (FR-BOM-10), 0 terminada */
    PEV_VREF,            /* a = 1 se usa la referencia nominal, 0 vuelve la medida */
    PEV_FALLA_VREF,      /* a = 1 referencia en falla 5 s (evento falla_vref, FR-SON-04), 0 terminada */
} prot_ev_tipo_t;

typedef struct { prot_ev_tipo_t tipo; int a, b, c, d; } prot_evento_t;

#define PROT_MAX_EVENTOS 24
typedef struct {
    bool in1, in2, rele, led_verde, led_rojo, zumbador;
    prot_evento_t ev[PROT_MAX_EVENTOS];
    int n_ev;
    bool persistir;          /* cambió algo de prot_persistente_t: guardarlo ya */
} prot_salida_t;

typedef struct {
    prot_config_t cfg;
    val_t val;
    bom_t bom;
    es_motor_t motor;
    bool arrancado, t0_fijado;
    uint32_t t0_ms;
    uint32_t t_s;                       /* último reloj monótono visto, s (P20) */
    es_antirrebote_t ar_abierta, ar_cerrada, ar_flotador, ar_red, ar_boton;
    es_boton_t boton;
    uint8_t finales;
    bool flotador_agua, red_presente;   /* flotador_agua: el efectivo (con FR-BOM-10) */
    bool flotador_real, flotador_falla, flot_en_curso;
    uint32_t flot_desde_s;
    bool vref_nominal;
    float vref;
    bool vref_falla;                    /* FR-SON-04: nominal durante PROT_FALLA_VREF_MS */
    uint32_t vref_nominal_desde_ms;
    /* sondas */
    fug_sonda_t fug[PROT_N_SONDAS];
    int clase[PROT_N_SONDAS];           /* son_clase_t o -1 (ADS en falla) */
    bool mojada[PROT_N_SONDAS], confirmada[PROT_N_SONDAS];
    bool en_falla[PROT_N_SONDAS], falla_avisada[PROT_N_SONDAS];
    uint32_t falla_desde_ms[PROT_N_SONDAS];
    bool ads_ok[ES_N_ADS];
    /* medidas */
    bool nivel_valido, presion_valida;
    float nivel_cm, nivel_pct, presion_bar, vbat, i_rms;
    ene_estado_t bateria;
    bool bateria_conocida;
    ene_reap_t reap;
    sec_t seco;
    rea_t rearme;
    cic_t cic;
    bool marcha, marcha_cand;
    uint32_t marcha_cambio_ms;
    /* ejercicio y zumbador */
    uint32_t ultimo_mov_s;
    bool fuga_activa;
    uint32_t fuga_desde_ms;
    bool silenciado;
    uint32_t rechazo_hasta_ms, prio2_desde_ms;
    int prio_anterior;
    prot_persistente_t guardado;
} prot_t;

/* Arranque: relé en bajo hasta aquí (FR-BOM-02); restaura los valores de NVS (FR-PER-02) */
void prot_iniciar(prot_t *p, const prot_config_t *cfg, const prot_persistente_t *nvs, uint32_t t_s);
/* Un paso de 10 ms */
void prot_paso(prot_t *p, const prot_entrada_t *e, prot_salida_t *s);
prot_persistente_t prot_persistente(const prot_t *p);
/* ¿Alguna sonda activa en `mojada`? (FR-OTA-A1, para la Fase 3) */
bool prot_sonda_mojada(const prot_t *p);
/* g_ciego (§5.2, FR-SON-05): casa_cerrada y (≥ 2 sondas activas en falla o referencia en falla) */
bool prot_g_ciego(const prot_t *p);
