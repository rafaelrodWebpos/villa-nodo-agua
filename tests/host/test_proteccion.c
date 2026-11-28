/* TC-PROT-A1 · la protección completa sobre una planta simulada (FSD §5–§9, §12):
 * válvula con 3 s de carrera y sus finales, 8 lazos, 3 ADS1115, flotador, red, batería,
 * presión, pinza y botón. El núcleo corre cada 10 ms como en el firmware; las medidas
 * llegan cada 250 ms y la corriente cada segundo. Uso: test_proteccion <escenario> */
#include <stdio.h>
#include <string.h>
#include "proteccion.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- Planta ---- */
enum { SECA, MOJADA, ABIERTA, CORTO, LIMITE };   /* LIMITE: 0,42, seca por poco */
typedef struct {
    double pos;                 /* 0 = cerrada, 1 = abierta */
    double carrera_ms;
    int sonda[8];
    bool ads_ok[3];
    float nivel_pct_real;       /* sobre altura útil de fábrica: 200 cm de 500 */
    float presion_bar, vbat, i_a;
    float vref_real, v_a3;      /* referencia real de los lazos y lo que lee A3 */
    bool transductor_ok;
    bool flotador_agua, red, boton;
    bool hora_valida; int hora_local; uint32_t ahora_s;
} planta_t;

static const float F_SONDA[] = { 0.5f, 0.18f, 1.0f, 0.0f, 0.42f };   /* fracción del lazo (C-HW-26) */

typedef struct {
    prot_t p;
    planta_t pl;
    uint32_t t;                 /* ms desde el arranque */
    prot_salida_t s;
    bool ambos;                 /* alguna vez IN1 e IN2 a la vez */
    int n_persistir;
    prot_evento_t log[512];
    uint32_t t_log[512];
    int n_log;
    uint32_t motor_movido_ms;   /* primera vez con el motor en marcha (o 0) */
    int orden;                  /* orden para el próximo paso (PORD_*) */
} sim_t;

static void sim_iniciar(sim_t *x, const prot_persistente_t *nvs, double pos)
{
    memset(x, 0, sizeof *x);
    prot_config_t c;
    prot_config_fabrica(&c);
    prot_iniciar(&x->p, &c, nvs, 0);
    x->pl = (planta_t){ .pos = pos, .carrera_ms = 3000, .ads_ok = { true, true, true }, .nivel_pct_real = 50,
                        .presion_bar = 3.0f, .vbat = 13.2f, .flotador_agua = true, .red = true,
                        .vref_real = 3.3f, .v_a3 = 3.3f, .transductor_ok = true };
}

static void paso(sim_t *x)
{
    planta_t *pl = &x->pl;
    prot_entrada_t e = { .t_ms = x->t, .t_s = x->t / 1000,
                         .fin_abierta = pl->pos >= 1.0, .fin_cerrada = pl->pos <= 0.0,
                         .flotador_agua = pl->flotador_agua, .red_presente = pl->red, .boton = pl->boton,
                         .conectado = true, .hora_valida = pl->hora_valida, .hora_local = pl->hora_local, .ahora_s = pl->ahora_s,
                         .orden = x->orden };
    x->orden = PORD_NINGUNA;
    if (x->t % 250 == 0) {
        e.hay_medidas = true;
        for (int k = 0; k < 3; k++) e.ads_ok[k] = pl->ads_ok[k];
        for (int i = 0; i < 8; i++) e.v_sonda[i] = F_SONDA[pl->sonda[i]] * pl->vref_real;
        e.v_ref = pl->v_a3;
        float cm = pl->nivel_pct_real / 100.0f * 200.0f;                /* altura útil 200 cm */
        e.v_nivel = pl->transductor_ok ? (4.0f + cm / 500.0f * 16.0f) * 150.0f / 1000.0f : 0.3f;   /* 2 mA si falla */
        e.v_presion = (0.5f + pl->presion_bar / 10.0f * 4.0f) * 2.0f / 3.0f;
        e.v_bat = pl->vbat / 6.0f;
    }
    if (x->t % 1000 == 0) { e.hay_corriente = true; e.i_rms = pl->i_a; }
    prot_paso(&x->p, &e, &x->s);
    x->ambos |= x->s.in1 && x->s.in2;
    x->n_persistir += x->s.persistir;
    for (int i = 0; i < x->s.n_ev && x->n_log < 512; i++) { x->t_log[x->n_log] = x->t; x->log[x->n_log++] = x->s.ev[i]; }
    if ((x->s.in1 || x->s.in2) && !x->motor_movido_ms) x->motor_movido_ms = x->t ? x->t : 1;
    /* la válvula se mueve con el motor */
    double d = 10.0 / pl->carrera_ms;
    if (x->s.in1) pl->pos = pl->pos - d < 0 ? 0 : pl->pos - d;
    if (x->s.in2) pl->pos = pl->pos + d > 1 ? 1 : pl->pos + d;
    x->t += 10;
}

static void correr(sim_t *x, uint32_t hasta_ms) { while (x->t < hasta_ms) paso(x); }

/* Una orden en el paso siguiente; devuelve la salida de ese paso (FR-ORD-01) */
static prot_salida_t ordenar(sim_t *x, int o) { x->orden = o; paso(x); return x->s; }

/* primer instante ≥ desde de un evento de ese tipo (y a, si a ≥ 0); UINT32_MAX si no hay */
static uint32_t cuando(const sim_t *x, prot_ev_tipo_t t, int a, uint32_t desde)
{
    for (int i = 0; i < x->n_log; i++)
        if (x->log[i].tipo == t && (a < 0 || x->log[i].a == a) && x->t_log[i] >= desde) return x->t_log[i];
    return UINT32_MAX;
}
static int cuantos(const sim_t *x, prot_ev_tipo_t t, int a)
{
    int n = 0;
    for (int i = 0; i < x->n_log; i++) n += x->log[i].tipo == t && (a < 0 || x->log[i].a == a);
    return n;
}

static const prot_persistente_t LIMPIO = { false, VCP_NINGUNO, BOM_PERMITIDA, BMOT_NINGUNO, 0, false };

/* Arranque en ABIERTA, todo seco: V01 sin mover el motor (FR-VAL-06); relé en bajo */
static void arranque(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 10000);
    OK(x.p.val.estado == VAL_ABIERTA && cuantos(&x, PEV_VAL, 1) == 1, "estado %s", val_nombre_estado(x.p.val.estado));
    OK(!x.motor_movido_ms && !x.s.rele && x.n_persistir == 0, "motor %u, relé %d, escrituras %d", x.motor_movido_ms, x.s.rele, x.n_persistir);
    OK(x.s.led_verde && !x.s.led_rojo && !x.s.zumbador, "LED verde fijo (prioridad 5)");
    for (int i = 1; i <= 8; i++) OK(cuantos(&x, PEV_SON, i) == 1, "línea A3 inicial de la sonda %d", i);
}

/* Fuga en la sonda 3: confirmación (FR-FUG-01), IN1 ≤ 500 ms (FR-FUG-02), motor parado
 * ≤ 100 ms tras el final (FR-VAL-05), bloqueo guardado (FR-PER-01), bomba igual (FR-FUG-03),
 * LED rojo fijo y zumbador que una pulsación corta silencia (FR-LOC-01/02) */
static void fuga(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 5000);
    x.pl.sonda[2] = MOJADA;
    uint32_t t0 = x.t;
    uint32_t t_in1 = 0, t_final = 0, t_parado = 0;
    while (x.t < 15000) {
        paso(&x);
        if (x.s.in1 && !t_in1) t_in1 = x.t;
        if (x.pl.pos <= 0 && !t_final) t_final = x.t;
        if (t_final && !x.s.in1 && !t_parado) t_parado = x.t;
    }
    uint32_t t_fuga = cuando(&x, PEV_FUGA, 3, t0);
    OK(t_fuga != UINT32_MAX && t_fuga - t0 <= 1000 + 250, "fuga confirmada a los %u ms (4 muestras de 250 ms)", t_fuga - t0);
    OK(t_in1 && t_in1 - t_fuga <= 500, "IN1 a los %u ms de la confirmación", t_in1 - t_fuga);
    OK(t_parado && t_parado - t_final <= 100, "motor parado a los %u ms del final", t_parado - t_final);
    OK(x.p.val.estado == VAL_CERRADA && x.p.val.bloqueo_fuga && x.p.val.cerrada_por == VCP_FUGA, "estado final");
    OK(x.n_persistir >= 1 && x.p.guardado.bloqueo_fuga, "bloqueo guardado en NVS");
    OK(!x.s.rele && x.p.bom.estado == BOM_PERMITIDA, "la fuga cambió el permiso de la bomba");
    OK(x.s.led_rojo && !x.s.led_verde, "LED rojo fijo");
    int z = 0; for (int i = 0; i < 200; i++) { paso(&x); z += x.s.zumbador; }
    OK(z > 50 && z < 150, "zumbador a 1 Hz (%d de 200 pasos)", z);
    x.pl.boton = true; correr(&x, x.t + 500); x.pl.boton = false; correr(&x, x.t + 200);
    z = 0; for (int i = 0; i < 200; i++) { paso(&x); z += x.s.zumbador; }
    OK(z == 0 && x.p.val.estado == VAL_CERRADA, "pulsación corta: zumbador %d, válvula %s", z, val_nombre_estado(x.p.val.estado));
    OK(!x.ambos, "IN1 e IN2 a la vez");
}

/* FR-PER-03: sonda 4 mojada desde antes del arranque, válvula abierta → CERRANDO ≤ 1,5 s */
static void arranque_mojado(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    x.pl.sonda[3] = MOJADA;
    correr(&x, 3000);
    uint32_t t = UINT32_MAX;
    for (int i = 0; i < x.n_log; i++) if (x.log[i].tipo == PEV_VAL && x.log[i].c == VAL_CERRANDO) { t = x.t_log[i]; break; }
    OK(t <= 1500, "CERRANDO a los %u ms del arranque", t);
}

/* FR-PER-02: NVS con bloqueo_fuga y BLOQUEADA → relé alto desde el primer paso, sin motor, sin rearme */
static void restaurar(void)
{
    static sim_t x;
    prot_persistente_t nvs = { true, VCP_FUGA, BOM_BLOQUEADA, BMOT_NINGUNO, 3, false };
    sim_iniciar(&x, &nvs, 0.0);
    paso(&x);
    OK(x.s.rele, "relé en bajo en el primer paso");
    correr(&x, 30 * 60 * 1000);
    OK(x.s.rele && x.p.bom.estado == BOM_BLOQUEADA && x.p.val.bloqueo_fuga && x.p.val.estado == VAL_CERRADA && !x.motor_movido_ms,
       "tras 30 min: relé %d, bomba %d, bloqueo %d, válvula %s", x.s.rele, x.p.bom.estado, x.p.val.bloqueo_fuga, val_nombre_estado(x.p.val.estado));
    OK(x.n_persistir == 0, "escrituras de NVS sin cambios: %d", x.n_persistir);
}

/* FR-PER-01/02 (Q-06): el último rearme se guarda y, tras un reinicio, cuenta como ocurrido al
 * arrancar: un seco a los 3 min del arranque bloquea (B15) */
static void rearme_reinicio(void)
{
    prot_config_t cfg; prot_config_fabrica(&cfg);
    prot_t p;
    prot_persistente_t nvs = { false, VCP_NINGUNO, BOM_PERMITIDA, BMOT_NINGUNO, 1, true };
    prot_iniciar(&p, &cfg, &nvs, 0);
    OK(prot_persistente(&p).rearme_reciente, "rearme reciente no restaurado");
    bom_entrada_t s1 = { BEV_SECO, true, true, 50, 180 };
    bom_accion_t a = bom_evento(&p.bom, &s1);
    OK(a.fila == 15 && p.bom.estado == BOM_BLOQUEADA, "seco a los 3 min del arranque: B%02d, estado %d", a.fila, p.bom.estado);
    nvs.rearme_reciente = false;
    prot_iniciar(&p, &cfg, &nvs, 0);
    a = bom_evento(&p.bom, &s1);
    OK(a.fila == 1, "sin rearme guardado: B%02d (esperada B01)", a.fila);
    /* el rearme deja de ser reciente a los 10 min: un cambio que se guarda (FR-PER-01) */
    p.bom.hay_rearme = true; p.bom.rearme_t = 1000; p.t_s = 1000 + BOM_P20_S;
    OK(prot_persistente(&p).rearme_reciente, "rearme a los 600 s ya no es reciente");
    p.t_s = 1000 + BOM_P20_S + 1;
    OK(!prot_persistente(&p).rearme_reciente, "rearme a los 601 s sigue reciente");
}

/* FR-IO-06: el ADS 0x49 deja de responder → falla_adc, sondas 5–8 en falla ≤ 2 s, sin cierre */
static void ads_caido(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    x.p.cfg.casa_cerrada = false;                       /* casa ocupada: solo aviso (FR-SON-05) */
    correr(&x, 5000);
    x.pl.ads_ok[1] = false;
    uint32_t t0 = x.t;
    correr(&x, 20000);
    OK(cuantos(&x, PEV_FALLA_ADC, 1) == 1, "falla_adc 0x49: %d", cuantos(&x, PEV_FALLA_ADC, 1));
    for (int s = 5; s <= 8; s++) OK(cuando(&x, PEV_FALLA_SONDA, s, t0) - t0 <= 2000, "sonda %d en falla a los %u ms", s, cuando(&x, PEV_FALLA_SONDA, s, t0) - t0);
    for (int s = 1; s <= 4; s++) OK(cuando(&x, PEV_FALLA_SONDA, s, t0) == UINT32_MAX, "sonda %d en falla", s);
    OK(x.p.val.estado == VAL_ABIERTA && !x.motor_movido_ms, "cierre por el ADS caído");
    OK(x.s.led_rojo || cuantos(&x, PEV_FALLA_SONDA, -1) == 4, "LED de falla");
    /* Con la casa cerrada, las 4 sondas del ADS caído dejan el nodo ciego: V24 */
    static sim_t y; sim_iniciar(&y, &LIMPIO, 1.0);
    correr(&y, 5000);
    y.pl.ads_ok[1] = false;
    t0 = y.t;
    correr(&y, t0 + 10000);
    uint32_t tc = y.motor_movido_ms;
    OK(tc && tc - t0 <= 2500 && y.p.val.estado == VAL_CERRADA && y.p.val.cerrada_por == VCP_SONDAS && !y.p.val.bloqueo_fuga,
       "casa cerrada + ADS caído: V24 a los %u ms, estado %s", tc - t0, val_nombre_estado(y.p.val.estado));
}

/* FR-SON-05 (FSD 0.7.1, H-93): casa cerrada y ≥ 2 sondas en falla → V24; V25 rechaza la
 * reapertura mientras dura; sin reapertura automática; con la casa ocupada, solo aviso */
static void ciego(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 3000);
    x.pl.sonda[1] = ABIERTA;
    correr(&x, x.t + 2000);
    x.pl.sonda[6] = ABIERTA;                            /* la segunda, 2 s después */
    uint32_t t2 = x.t;
    correr(&x, t2 + 10000);                             /* 5 s de falla + 500 ms + 3 s de carrera */
    uint32_t f2 = cuando(&x, PEV_FALLA_SONDA, 7, t2);
    OK(f2 != UINT32_MAX && x.motor_movido_ms >= f2 && x.motor_movido_ms - f2 <= 500,
       "V24 a %u ms de la segunda falla", x.motor_movido_ms - f2);
    OK(x.p.val.estado == VAL_CERRADA && x.p.val.cerrada_por == VCP_SONDAS && !x.p.val.bloqueo_fuga, "cerrada por sondas: %s cp=%d",
       val_nombre_estado(x.p.val.estado), x.p.val.cerrada_por);
    OK(!x.ambos, "IN1 e IN2 a la vez");
    x.pl.boton = true; correr(&x, x.t + 3500); x.pl.boton = false; correr(&x, x.t + 500);
    OK(cuantos(&x, PEV_RECHAZO, 3) == 1 && x.p.val.estado == VAL_CERRADA, "V25: pulsación rechazada (%d)", cuantos(&x, PEV_RECHAZO, 3));
    x.pl.sonda[1] = SECA; x.pl.sonda[6] = SECA;
    correr(&x, x.t + 10000);
    OK(x.p.val.estado == VAL_CERRADA, "reapertura automática al quitarse la falla");
    x.pl.boton = true; correr(&x, x.t + 3500); x.pl.boton = false; correr(&x, x.t + 5000);
    OK(x.p.val.estado == VAL_ABIERTA && x.p.val.cerrada_por == VCP_NINGUNO, "reapertura en persona (V10): %s",
       val_nombre_estado(x.p.val.estado));

    /* Una sola sonda en falla no cierra */
    static sim_t u; sim_iniciar(&u, &LIMPIO, 1.0);
    correr(&u, 3000); u.pl.sonda[2] = CORTO; correr(&u, u.t + 10000);
    OK(!u.motor_movido_ms && cuantos(&u, PEV_FALLA_SONDA, 3) == 1, "cierre con una sola sonda en falla");

    /* Casa ocupada: las mismas fallas solo avisan */
    static sim_t o; sim_iniciar(&o, &LIMPIO, 1.0);
    o.p.cfg.casa_cerrada = false;
    correr(&o, 3000); o.pl.sonda[1] = ABIERTA; o.pl.sonda[6] = ABIERTA; correr(&o, o.t + 10000);
    OK(!o.motor_movido_ms && cuantos(&o, PEV_FALLA_SONDA, -1) == 2, "casa ocupada: movimiento o avisos (%d)",
       cuantos(&o, PEV_FALLA_SONDA, -1));
}

/* FR-SON-04 (FSD 0.7.1): referencia en falla 5 s → un falla_vref y LED de prioridad 2; con la
 * casa cerrada, V24; al volver, sin falla_vref repetidos */
static void falla_vref(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 3000);
    x.pl.v_a3 = 2.5f;                                    /* fuera de 3,0–3,6 V */
    uint32_t t0 = x.t;
    correr(&x, t0 + 4500);
    OK(cuantos(&x, PEV_FALLA_VREF, 1) == 0 && !x.motor_movido_ms, "falla_vref antes de 5 s");
    correr(&x, t0 + 7000);
    uint32_t tf = cuando(&x, PEV_FALLA_VREF, 1, t0);
    OK(cuantos(&x, PEV_FALLA_VREF, 1) == 1 && tf - t0 >= 5000 && tf - t0 <= 5250, "falla_vref a los %u ms", tf - t0);
    OK(x.motor_movido_ms && x.motor_movido_ms - tf <= 500 && x.p.val.cerrada_por == VCP_SONDAS, "V24 por la referencia");
    x.pl.v_a3 = 3.3f; correr(&x, x.t + 2000);
    x.pl.v_a3 = 2.5f; correr(&x, x.t + 3000);           /* < 5 s: no hay otro evento */
    OK(cuantos(&x, PEV_FALLA_VREF, 1) == 1 && cuantos(&x, PEV_FALLA_VREF, 0) == 1, "eventos repetidos sin cambio");

    static sim_t o; sim_iniciar(&o, &LIMPIO, 1.0);
    o.p.cfg.casa_cerrada = false;
    correr(&o, 3000); o.pl.v_a3 = 2.5f; correr(&o, o.t + 8000);
    OK(cuantos(&o, PEV_FALLA_VREF, 1) == 1 && !o.motor_movido_ms && o.p.val.estado == VAL_ABIERTA, "casa ocupada: solo aviso");
    /* §8.2: prioridad 2 (rojo 1 Hz) con la válvula abierta y sin otra falla */
    bool rojo = false, verde = false;
    for (int i = 0; i < 200; i++) { paso(&o); rojo |= o.s.led_rojo; verde |= o.s.led_verde; }
    OK(rojo && !verde, "LED de prioridad 2 con la referencia en falla");
}

/* FR-SON-02/03: lazo 6 abierto 6 s → un falla_sonda sin mover la válvula; sonda 8 inactiva en corto → nada */
static void sondas(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    x.p.cfg.sonda_activa[7] = false;
    x.pl.sonda[7] = CORTO;
    correr(&x, 3000);
    x.pl.sonda[5] = ABIERTA;
    uint32_t t0 = x.t;
    correr(&x, t0 + 6000);
    uint32_t t = cuando(&x, PEV_FALLA_SONDA, 6, t0);
    OK(cuantos(&x, PEV_FALLA_SONDA, 6) == 1 && t - t0 >= 5000 && t - t0 <= 5250, "falla_sonda 6 a los %u ms (%d)", t - t0, cuantos(&x, PEV_FALLA_SONDA, 6));
    OK(cuantos(&x, PEV_FALLA_SONDA, 8) == 0 && cuantos(&x, PEV_SON, 8) == 0 && cuantos(&x, PEV_FUGA, -1) == 0, "eventos de la sonda inactiva");
    OK(!x.motor_movido_ms, "movimiento por una falla de sonda");
    x.pl.sonda[5] = SECA; correr(&x, x.t + 2000); x.pl.sonda[5] = ABIERTA; correr(&x, x.t + 4000);
    OK(cuantos(&x, PEV_FALLA_SONDA, 6) == 1, "falla de menos de 5 s avisada");
}

/* FR-BOM-03: 0,8 A con i_seco 1,0 y 0,2 bar → CORTADA(seco) a los 10 ± 1 s; con 2 bar, nunca */
static void seco(void)
{
    for (int k = 0; k < 2; k++) {
        static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
        x.p.cfg.i_seco_a = 1.0f;
        correr(&x, 3000);
        x.pl.i_a = 0.8f; x.pl.presion_bar = k ? 2.0f : 0.2f;
        uint32_t t0 = x.t, t_rele = 0;
        while (x.t < t0 + 15000) { paso(&x); if (x.s.rele && !t_rele) t_rele = x.t; }
        if (k == 0) OK(t_rele && t_rele - t0 >= 9000 && t_rele - t0 <= 12000 && x.p.bom.motivo == BMOT_SECO, "corte por seco a los %u ms", t_rele - t0);
        else OK(!t_rele, "corte con presión alta");
    }
    static sim_t y; sim_iniciar(&y, &LIMPIO, 1.0);           /* sin calibrar: nunca */
    y.pl.i_a = 0.8f; y.pl.presion_bar = 0.2f;
    correr(&y, 30000);
    OK(!y.s.rele, "corte sin i_seco_a");
}

/* FR-BOM-04: transductor en 80 % y flotador sin agua → relé ≤ 1 s */
static void flotador(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    x.pl.nivel_pct_real = 80;
    correr(&x, 3000);
    x.pl.flotador_agua = false;
    uint32_t t0 = x.t, t_rele = 0;
    while (x.t < t0 + 3000) { paso(&x); if (x.s.rele && !t_rele) t_rele = x.t; }
    OK(t_rele && t_rele - t0 <= 1000 && x.p.bom.motivo == BMOT_FLOTADOR, "relé a los %u ms", t_rele - t0);
    /* pulsación larga sin agua: rechazo (B08) con el patrón; con agua: B07 */
    x.pl.boton = true; correr(&x, x.t + 3200); x.pl.boton = false; correr(&x, x.t + 100);
    OK(cuantos(&x, PEV_RECHAZO, 2) == 1 && x.s.rele, "rechazo sin_agua");
    x.pl.flotador_agua = true; correr(&x, x.t + 500);
    x.pl.boton = true; correr(&x, x.t + 3200); x.pl.boton = false; correr(&x, x.t + 100);
    OK(!x.s.rele && x.p.bom.estado == BOM_PERMITIDA, "rearme con pulsación larga");
}

/* FR-ENE-04/05: crítica sin red → cierre con cerrada_por = bateria; red 5 min y 12,8 V → reapertura */
static void bateria(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 3000);
    x.pl.vbat = 11.9f; x.pl.red = false;
    correr(&x, x.t + 6000);
    OK(x.p.val.estado == VAL_CERRADA && x.p.val.cerrada_por == VCP_BATERIA && cuantos(&x, PEV_BATERIA, ENE_CRITICA) == 1,
       "cierre por batería: %s, cerrada_por %d", val_nombre_estado(x.p.val.estado), x.p.val.cerrada_por);
    x.pl.vbat = 13.0f; x.pl.red = true;
    uint32_t t0 = x.t;
    correr(&x, t0 + 4 * 60 * 1000);
    OK(x.p.val.estado == VAL_CERRADA, "reapertura antes de 5 min");
    correr(&x, t0 + 6 * 60 * 1000);
    OK(x.p.val.estado == VAL_ABIERTA && x.p.val.cerrada_por == VCP_NINGUNO, "reapertura tras 5 min: %s", val_nombre_estado(x.p.val.estado));
}

/* FR-LOC-03 + V10: válvula cerrada por fuga ya seca; pulsación larga de 3,5 s abre, una de 2,5 s no */
static void boton(void)
{
    static sim_t x;
    prot_persistente_t nvs = { true, VCP_FUGA, BOM_PERMITIDA, BMOT_NINGUNO, 0, false };
    sim_iniciar(&x, &nvs, 0.0);
    correr(&x, 2000);
    x.pl.boton = true; correr(&x, x.t + 2500); x.pl.boton = false; correr(&x, x.t + 1000);
    OK(x.p.val.estado == VAL_CERRADA && !x.motor_movido_ms, "acción con 2,5 s");
    x.pl.boton = true; correr(&x, x.t + 3500); x.pl.boton = false; correr(&x, x.t + 5000);
    OK(x.p.val.estado == VAL_ABIERTA && !x.p.val.bloqueo_fuga && x.n_persistir >= 1, "3,5 s: %s", val_nombre_estado(x.p.val.estado));
}

/* FR-LOC-06: pulsación de 11 s → PEV_PORTAL al soltar, sin acción de la pulsación larga; con la
 * válvula en CERRANDO se ignora. A11: al arrancar y en cada cambio */
static void portal(void)
{
    static sim_t x;
    prot_persistente_t nvs = { true, VCP_FUGA, BOM_CORTADA, BMOT_SECO, 2, false };
    sim_iniciar(&x, &nvs, 0.0);
    correr(&x, 2000);
    OK(cuantos(&x, PEV_ESTADO, -1) == 1 && x.log[0].tipo != PEV_PORTAL, "A11 al arrancar: %d", cuantos(&x, PEV_ESTADO, -1));
    uint32_t t = cuando(&x, PEV_ESTADO, VCP_FUGA, 0);
    OK(t != UINT32_MAX, "A11 al arrancar con cerrada_por=fuga");
    x.pl.boton = true; correr(&x, x.t + 9000);
    OK(cuantos(&x, PEV_PORTAL, -1) == 0, "portal antes de soltar");
    correr(&x, x.t + 2000); x.pl.boton = false; correr(&x, x.t + 500);
    OK(cuantos(&x, PEV_PORTAL, -1) == 1, "portal al soltar 11 s: %d", cuantos(&x, PEV_PORTAL, -1));
    OK(x.p.val.estado == VAL_CERRADA && x.p.bom.estado == BOM_CORTADA && !x.motor_movido_ms, "acción de la pulsación larga con 11 s");
    /* válvula en CERRANDO: ignorada */
    static sim_t y; sim_iniciar(&y, &LIMPIO, 1.0);
    y.pl.carrera_ms = 20000;                                   /* carrera larga: la pulsación cabe dentro */
    correr(&y, 2000);
    y.pl.sonda[0] = MOJADA; correr(&y, y.t + 1600);
    OK(y.p.val.estado == VAL_CERRANDO, "precondición: CERRANDO (%s)", val_nombre_estado(y.p.val.estado));
    int a11 = cuantos(&y, PEV_ESTADO, -1);
    OK(a11 >= 2, "A11 al cambiar cerrada_por y bloqueo: %d", a11);
    y.pl.boton = true; uint32_t t0 = y.t;
    correr(&y, t0 + 11000);
    y.pl.boton = false;
    OK(y.p.val.estado == VAL_CERRANDO, "precondición: sigue CERRANDO al soltar");
    correr(&y, y.t + 300);
    OK(cuantos(&y, PEV_PORTAL, -1) == 0, "portal con la válvula en CERRANDO");
}

/* FR-ORD-01: cada orden responde con su fila o con un rechazo con motivo; FR-ORD-04; FR-EJE-04 */
static void ordenes(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 2000);
    prot_salida_t r = ordenar(&x, PORD_CERRAR);                 /* V05; FR-ORD-04: fuga_remota es una orden más */
    OK(r.hay_resp && !r.resp_bomba && r.resp_fila == 5 && !r.resp_rechazo && x.p.val.cerrada_por == VCP_ORDEN && !x.p.val.bloqueo_fuga,
       "cerrar: fila %d, rechazo %s", r.resp_fila, r.resp_rechazo ? r.resp_rechazo : "-");
    r = ordenar(&x, PORD_ABRIR);                                 /* V19: en marcha */
    OK(r.resp_fila == 19 && r.resp_rechazo && !strcmp(r.resp_rechazo, "ocupado"), "abrir en CERRANDO: %s", r.resp_rechazo ? r.resp_rechazo : "-");
    correr(&x, x.t + 4000);
    x.pl.sonda[1] = MOJADA; correr(&x, x.t + 1600);
    r = ordenar(&x, PORD_ABRIR);                                 /* V09 */
    OK(r.resp_fila == 9 && r.resp_rechazo && !strcmp(r.resp_rechazo, "fuga_activa") && x.p.val.estado == VAL_CERRADA, "abrir con fuga: %s", r.resp_rechazo ? r.resp_rechazo : "-");
    x.pl.sonda[1] = SECA; correr(&x, x.t + 1000);
    r = ordenar(&x, PORD_CERRAR);                                /* CERRADA × cerrar: ignorada */
    OK(r.hay_resp && r.resp_fila == 0 && !r.resp_rechazo, "cerrar en CERRADA: fila %d", r.resp_fila);
    r = ordenar(&x, PORD_ABRIR);                                 /* V08 */
    OK(r.resp_fila == 8 && !r.resp_rechazo, "abrir seca: fila %d", r.resp_fila);
    /* bomba */
    r = ordenar(&x, PORD_CORTAR);
    OK(r.resp_bomba && r.resp_fila == 4 && x.p.bom.estado == BOM_CORTADA, "cortar: fila %d", r.resp_fila);
    x.pl.flotador_agua = false; correr(&x, x.t + 500);
    r = ordenar(&x, PORD_PERMITIR);
    OK(r.resp_rechazo && !strcmp(r.resp_rechazo, "sin_agua"), "permitir sin agua: %s", r.resp_rechazo ? r.resp_rechazo : "-");
    x.pl.flotador_agua = true; correr(&x, x.t + 500);
    x.pl.nivel_pct_real = 5; correr(&x, x.t + 1000);
    r = ordenar(&x, PORD_REARMAR);
    OK(r.resp_rechazo && !strcmp(r.resp_rechazo, "nivel_bajo"), "rearmar con nivel bajo: %s", r.resp_rechazo ? r.resp_rechazo : "-");
    /* BLOQUEADA → rearmar rechaza con bloqueada */
    static sim_t y;
    prot_persistente_t nvs = { false, VCP_NINGUNO, BOM_BLOQUEADA, BMOT_NINGUNO, 3, false };
    sim_iniciar(&y, &nvs, 1.0); correr(&y, 2000);
    r = ordenar(&y, PORD_REARMAR);
    OK(r.resp_fila == 14 && r.resp_rechazo && !strcmp(r.resp_rechazo, "bloqueada") && y.p.bom.estado == BOM_BLOQUEADA, "rearmar en BLOQUEADA");
    /* FR-EJE-04: posponer una vez por ciclo; la segunda se rechaza con estado */
    y.pl.hora_valida = true; y.pl.ahora_s = 1800000000u; correr(&y, y.t + 300);
    r = ordenar(&y, PORD_POSPONER);
    OK(!r.resp_rechazo && y.p.ultimo_mov_s == 1800000000u, "posponer: rechazo %s, cuenta %u", r.resp_rechazo ? r.resp_rechazo : "-", (unsigned)y.p.ultimo_mov_s);
    r = ordenar(&y, PORD_POSPONER);
    OK(r.resp_rechazo && !strcmp(r.resp_rechazo, "estado"), "segundo posponer: %s", r.resp_rechazo ? r.resp_rechazo : "-");
    /* ejercicio por orden: V13 con las mismas guardas */
    r = ordenar(&y, PORD_EJERCICIO);
    OK(r.resp_fila == 13 && y.p.val.estado == VAL_CERRANDO && y.p.val.cerrada_por == VCP_EJERCICIO, "ejercicio por orden: fila %d", r.resp_fila);
    correr(&y, y.t + 8000);
    OK(!y.p.pospuesto, "el ejercicio completado no reinicia el posponer");
    /* sin orden, ninguna respuesta */
    paso(&y);
    OK(!y.s.hay_resp, "respuesta sin orden");
}

/* FR-EJE-01 + FR-EJE-03: el ejercicio a la hora 3 con los días cumplidos; una fuga a mitad de
 * cierre termina CERRADA con bloqueo */
static void ejercicio(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    x.pl.hora_valida = true; x.pl.hora_local = 2; x.pl.ahora_s = 1800000000u;
    correr(&x, 2000);
    x.pl.ahora_s += 30 * 86400; x.pl.hora_local = 3;
    correr(&x, x.t + 1000);
    OK(x.p.val.estado == VAL_CERRANDO && x.p.val.cerrada_por == VCP_EJERCICIO, "V13: %s", val_nombre_estado(x.p.val.estado));
    correr(&x, x.t + 8000);
    OK(x.p.val.estado == VAL_ABIERTA && cuantos(&x, PEV_VAL, 14) == 1 && cuantos(&x, PEV_VAL, 12) == 1, "ejercicio completo: %s", val_nombre_estado(x.p.val.estado));
    /* otro ejercicio 30 días después, con una fuga a mitad del cierre */
    x.pl.ahora_s += 30 * 86400;
    correr(&x, x.t + 1500);
    x.pl.sonda[0] = MOJADA;
    correr(&x, x.t + 8000);
    OK(x.p.val.estado == VAL_CERRADA && x.p.val.bloqueo_fuga && x.p.val.cerrada_por == VCP_FUGA, "fuga durante el ejercicio: %s, bloqueo %d",
       val_nombre_estado(x.p.val.estado), x.p.val.bloqueo_fuga);
}

/* FR-BOM-10: nivel 50 % y flotador sin agua → B02 enseguida; flotador_falla a los 10 min; PERMITIDA por B05
 * a los 10 min + t_rearme_min (FSD 0.8.7, Q-04);
 * transductor en falla → CORTADA(flotador) otra vez. Con nivel 20 % (< 30 %), nunca falla. */
static void flotador_falla(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 3000);
    x.pl.flotador_agua = false;
    uint32_t t0 = x.t;
    correr(&x, t0 + 1000);
    OK(x.s.rele && x.p.bom.motivo == BMOT_FLOTADOR, "B02 al empezar");
    correr(&x, t0 + 590000);
    OK(x.s.rele && cuantos(&x, PEV_FLOTADOR_FALLA, 1) == 0, "flotador_falla antes de 10 min");
    correr(&x, t0 + 602000);
    uint32_t t = cuando(&x, PEV_FLOTADOR_FALLA, 1, t0);
    OK(t != UINT32_MAX && t - t0 >= 600000 && t - t0 <= 601000, "flotador_falla a los %u ms", t - t0);
    OK(x.s.rele && x.p.bom.estado == BOM_CORTADA, "PERMITIDA al declarar la falla (B05 cuenta desde ahí)");
    correr(&x, t0 + 1190000);
    OK(x.s.rele, "PERMITIDA antes de 10 min + t_rearme_min");
    correr(&x, t0 + 1212000);
    OK(!x.s.rele && x.p.bom.estado == BOM_PERMITIDA && cuantos(&x, PEV_BOM, 5) == 1, "PERMITIDA por B05 a los 10 min + t_rearme_min");
    correr(&x, t0 + 1900000);                                             /* más de 10 min tras el rearme (P20) */
    OK(cuantos(&x, PEV_FLOTADOR_FALLA, 1) == 1 && !x.s.rele, "un solo aviso y sigue permitida");
    x.pl.i_a = 0.8f; x.pl.presion_bar = 0.2f; x.p.cfg.i_seco_a = 1.0f;   /* la detección de seco sigue protegiendo */
    correr(&x, x.t + 13000);
    OK(x.s.rele && x.p.bom.motivo == BMOT_SECO, "seco con el flotador en falla: motivo %d", x.p.bom.motivo);
    x.pl.i_a = 0; x.pl.presion_bar = 3.0f;
    x.pl.boton = true; correr(&x, x.t + 3200); x.pl.boton = false; correr(&x, x.t + 200);   /* B07 con el flotador en falla */
    OK(!x.s.rele, "rearmar con el flotador en falla");
    x.pl.transductor_ok = false;                                          /* sin transductor: B02 otra vez */
    uint32_t t1 = x.t, t_rele = 0;
    while (x.t < t1 + 3000) { paso(&x); if (x.s.rele && !t_rele) t_rele = x.t; }
    OK(t_rele && t_rele - t1 <= 1000 && x.p.bom.motivo == BMOT_FLOTADOR && !x.p.flotador_falla && cuantos(&x, PEV_FLOTADOR_FALLA, 0) == 1,
       "transductor en falla: relé a los %u ms, motivo %d", t_rele - t1, x.p.bom.motivo);
    static sim_t y; sim_iniciar(&y, &LIMPIO, 1.0);
    y.pl.nivel_pct_real = 20;
    correr(&y, 3000);
    y.pl.flotador_agua = false;
    correr(&y, 20 * 60 * 1000);
    OK(cuantos(&y, PEV_FLOTADOR_FALLA, -1) == 0 && y.s.rele, "flotador_falla con nivel 20 %%");
}

/* Referencia de los lazos (FSD 0.6.0): con 3,0 V reales, un lazo en 0,42 es seca si se mide A3;
 * con A3 inválida se usa 3,3 V y se avisa (son: vref nominal) */
static void vref(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    x.pl.vref_real = 3.0f; x.pl.v_a3 = 3.0f;
    for (int i = 0; i < 8; i++) x.pl.sonda[i] = LIMITE;
    correr(&x, 5000);
    OK(cuantos(&x, PEV_FUGA, -1) == 0 && x.p.clase[0] == SON_SECA && cuantos(&x, PEV_VREF, -1) == 0, "con A3: clase %d", x.p.clase[0]);
    x.pl.v_a3 = 0.0f;                                   /* A3 sin cablear */
    correr(&x, x.t + 2000);
    OK(cuantos(&x, PEV_VREF, 1) == 1 && x.p.vref == ES_VREF_SONDAS_V, "aviso de referencia nominal");
    x.pl.v_a3 = 3.0f; correr(&x, x.t + 1000);
    OK(cuantos(&x, PEV_VREF, 0) == 1 && x.p.vref == 3.0f, "vuelve la medida");
}

int main(int argc, char **argv)
{
    const char *c = argc > 1 ? argv[1] : "";
    if (!strcmp(c, "arranque")) arranque();
    else if (!strcmp(c, "fuga")) fuga();
    else if (!strcmp(c, "arranque_mojado")) arranque_mojado();
    else if (!strcmp(c, "restaurar")) restaurar();
    else if (!strcmp(c, "ads")) ads_caido();
    else if (!strcmp(c, "sondas")) sondas();
    else if (!strcmp(c, "seco")) seco();
    else if (!strcmp(c, "flotador")) flotador();
    else if (!strcmp(c, "bateria")) bateria();
    else if (!strcmp(c, "boton")) boton();
    else if (!strcmp(c, "ejercicio")) ejercicio();
    else if (!strcmp(c, "flotador_falla")) flotador_falla();
    else if (!strcmp(c, "vref")) vref();
    else if (!strcmp(c, "ciego")) ciego();
    else if (!strcmp(c, "falla_vref")) falla_vref();
    else if (!strcmp(c, "rearme_reinicio")) rearme_reinicio();
    else if (!strcmp(c, "portal")) portal();
    else if (!strcmp(c, "ordenes")) ordenes();
    else { printf("caso desconocido: %s\n", c); return 2; }
    printf("%s: %d fallos\n", c, fallos);
    return fallos != 0;
}
