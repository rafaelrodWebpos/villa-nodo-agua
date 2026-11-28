/* L2 medidor — medidor testigo de agua (FSD §6.7, H-96): pulsos (FR-MED-01), total (FR-MED-02),
 * guardado en NVS (FR-MED-03), caudal (FR-MED-04), consumo con la casa cerrada (FR-MED-05/09),
 * periodo permitido (FR-MED-06) y su evento (FR-MED-10), medidor_falla (FR-MED-08).
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. No toca la válvula ni la bomba (FR-MED-07). */
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

/* ---- FR-MED-01 (P29): un pulso por cierre ≥ 50 ms tras ≥ 50 ms abierto ---- */
#define MED_ANTIRREBOTE_MS 50u
typedef struct {
    bool inicio;                   /* ya hubo una muestra */
    bool estable;                  /* estado estable: true = cerrado */
    bool cand;                     /* último valor leído */
    uint32_t cand_desde;           /* desde cuándo se lee `cand` */
    uint32_t estable_desde;        /* desde cuándo vale `estable` */
} med_pulso_t;
/* Muestra del contacto (true = cerrado) en t_ms; true si cuenta un pulso */
bool med_muestra(med_pulso_t *d, uint32_t t_ms, bool cerrado);

/* ---- FR-MED-02: total acumulado, nunca baja ---- */
#define MED_L_POR_GAL 3.78541
typedef struct { uint32_t pulsos; double total_l; } med_total_t;
/* Suma un pulso con el galones_por_pulso de ese momento; devuelve los litros del pulso */
double med_sumar(med_total_t *t, double galones_por_pulso);

/* FR-MED-04: caudal en L/h entre dos lecturas del total separadas dt_s */
double med_caudal_lh(double litros, uint32_t dt_s);

/* ---- FR-MED-03 (P30): guardar como mucho cada 60 s y solo si cambió ---- */
#define MED_GUARDAR_S 60u
typedef struct { uint32_t pulsos; uint32_t t_s; bool hecho; } med_guardado_t;
/* ¿Toca escribir en NVS en t_s? Si devuelve true, la tarea escribe y llama a med_guardado */
bool med_toca_guardar(const med_guardado_t *g, uint32_t t_s, uint32_t pulsos);
void med_guardado(med_guardado_t *g, uint32_t t_s, uint32_t pulsos);

/* ---- FR-MED-05/09 (P32, P34): litros fuera de periodo, por minuto, en RAM ---- */
#define MED_MINUTOS 1440               /* 24 h */
typedef struct {
    float l[MED_MINUTOS];              /* litros fuera de periodo en cada minuto (anillo) */
    uint32_t min_actual;               /* minuto (t_s / 60) del último registro */
    bool inicio;
    uint32_t ultimo_pulso_s;           /* cualquier pulso, dentro o fuera de periodo */
    bool hay_pulso;
    bool avisado_hora;                 /* episodio de FR-MED-05 abierto */
    bool avisado_dia; uint32_t aviso_dia_s;
} med_consumo_t;
typedef struct {
    bool hora;  float l_hora;          /* publicar consumo_casa_cerrada motivo=hora */
    bool dia;   float l_dia;           /* publicar consumo_casa_cerrada motivo=diario */
} med_aviso_t;
/* Registra un pulso de `litros` en t_s; `fuera` = fuera de periodo permitido */
void med_consumo_pulso(med_consumo_t *c, uint32_t t_s, double litros, bool fuera);
/* Evalúa en t_s (cada ciclo) con los parámetros de §19 */
med_aviso_t med_consumo_evaluar(med_consumo_t *c, uint32_t t_s, bool casa_cerrada,
                                int ventana_min, int alerta_l, int diario_l);

/* ---- FR-MED-08 (P33): nivel que sube ≥ 10 cm sin pulsos ---- */
#define MED_FALLA_CM 10.0f
typedef struct { bool hay_base; float base_cm; bool avisado; } med_falla_t;
void med_falla_pulso(med_falla_t *f);
/* Con cada nivel válido; true si toca publicar medidor_falla (una vez, hasta el siguiente pulso) */
bool med_falla_nivel(med_falla_t *f, float nivel_cm);
