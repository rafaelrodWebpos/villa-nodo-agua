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
enum { SECA, MOJADA, ABIERTA, CORTO };
typedef struct {
    double pos;                 /* 0 = cerrada, 1 = abierta */
    double carrera_ms;
    int sonda[8];
    bool ads_ok[3];
    float nivel_pct_real;       /* sobre altura útil de fábrica: 200 cm de 500 */
    float presion_bar, vbat, i_a;
    bool flotador_agua, red, boton;
    bool hora_valida; int hora_local; uint32_t ahora_s;
} planta_t;

static const float V_SONDA[] = { 1.65f, 0.60f, 3.30f, 0.0f };

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
} sim_t;

static void sim_iniciar(sim_t *x, const prot_persistente_t *nvs, double pos)
{
    memset(x, 0, sizeof *x);
    prot_config_t c;
    prot_config_fabrica(&c);
    prot_iniciar(&x->p, &c, nvs, 0);
    x->pl = (planta_t){ .pos = pos, .carrera_ms = 3000, .ads_ok = { true, true, true }, .nivel_pct_real = 50,
                        .presion_bar = 3.0f, .vbat = 13.2f, .flotador_agua = true, .red = true };
}

static void paso(sim_t *x)
{
    planta_t *pl = &x->pl;
    prot_entrada_t e = { .t_ms = x->t, .t_s = x->t / 1000,
                         .fin_abierta = pl->pos >= 1.0, .fin_cerrada = pl->pos <= 0.0,
                         .flotador_agua = pl->flotador_agua, .red_presente = pl->red, .boton = pl->boton,
                         .conectado = true, .hora_valida = pl->hora_valida, .hora_local = pl->hora_local, .ahora_s = pl->ahora_s };
    if (x->t % 250 == 0) {
        e.hay_medidas = true;
        for (int k = 0; k < 3; k++) e.ads_ok[k] = pl->ads_ok[k];
        for (int i = 0; i < 8; i++) e.v_sonda[i] = V_SONDA[pl->sonda[i]];
        float cm = pl->nivel_pct_real / 100.0f * 200.0f;                /* altura útil 200 cm */
        e.v_nivel = (4.0f + cm / 500.0f * 16.0f) * 150.0f / 1000.0f;   /* rango 500 cm, shunt 150 Ω */
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

static const prot_persistente_t LIMPIO = { false, VCP_NINGUNO, BOM_PERMITIDA, BMOT_NINGUNO, 0 };

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
    prot_persistente_t nvs = { true, VCP_FUGA, BOM_BLOQUEADA, BMOT_NINGUNO, 3 };
    sim_iniciar(&x, &nvs, 0.0);
    paso(&x);
    OK(x.s.rele, "relé en bajo en el primer paso");
    correr(&x, 30 * 60 * 1000);
    OK(x.s.rele && x.p.bom.estado == BOM_BLOQUEADA && x.p.val.bloqueo_fuga && x.p.val.estado == VAL_CERRADA && !x.motor_movido_ms,
       "tras 30 min: relé %d, bomba %d, bloqueo %d, válvula %s", x.s.rele, x.p.bom.estado, x.p.val.bloqueo_fuga, val_nombre_estado(x.p.val.estado));
    OK(x.n_persistir == 0, "escrituras de NVS sin cambios: %d", x.n_persistir);
}

/* FR-IO-06: el ADS 0x49 deja de responder → falla_adc, sondas 5–8 en falla ≤ 2 s, sin cierre */
static void ads_caido(void)
{
    static sim_t x; sim_iniciar(&x, &LIMPIO, 1.0);
    correr(&x, 5000);
    x.pl.ads_ok[1] = false;
    uint32_t t0 = x.t;
    correr(&x, 20000);
    OK(cuantos(&x, PEV_FALLA_ADC, 1) == 1, "falla_adc 0x49: %d", cuantos(&x, PEV_FALLA_ADC, 1));
    for (int s = 5; s <= 8; s++) OK(cuando(&x, PEV_FALLA_SONDA, s, t0) - t0 <= 2000, "sonda %d en falla a los %u ms", s, cuando(&x, PEV_FALLA_SONDA, s, t0) - t0);
    for (int s = 1; s <= 4; s++) OK(cuando(&x, PEV_FALLA_SONDA, s, t0) == UINT32_MAX, "sonda %d en falla", s);
    OK(x.p.val.estado == VAL_ABIERTA && !x.motor_movido_ms, "cierre por el ADS caído");
    OK(x.s.led_rojo || cuantos(&x, PEV_FALLA_SONDA, -1) == 4, "LED de falla");
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
    prot_persistente_t nvs = { true, VCP_FUGA, BOM_PERMITIDA, BMOT_NINGUNO, 0 };
    sim_iniciar(&x, &nvs, 0.0);
    correr(&x, 2000);
    x.pl.boton = true; correr(&x, x.t + 2500); x.pl.boton = false; correr(&x, x.t + 1000);
    OK(x.p.val.estado == VAL_CERRADA && !x.motor_movido_ms, "acción con 2,5 s");
    x.pl.boton = true; correr(&x, x.t + 3500); x.pl.boton = false; correr(&x, x.t + 5000);
    OK(x.p.val.estado == VAL_ABIERTA && !x.p.val.bloqueo_fuga && x.n_persistir >= 1, "3,5 s: %s", val_nombre_estado(x.p.val.estado));
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
    else { printf("caso desconocido: %s\n", c); return 2; }
    printf("%s: %d fallos\n", c, fallos);
    return fallos != 0;
}
