/* Medidor testigo (FSD §6.7). Uso: test_medidor <caso>
 *   periodo · TC-MED-A9 · FR-MED-06 (periodo permitido) y FR-MED-10 (consumo_permitido, Q-11/S-07)
 *   pulsos  · TC-MED-A1 · FR-MED-01 (antirrebote) y FR-MED-02 (total)
 *   nvs     · TC-MED-A3 · FR-MED-03 (guardar como mucho cada 60 s y solo si cambió)
 *   consumo · TC-MED-A2 · FR-MED-05, FR-MED-06, FR-MED-08, FR-MED-09 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "medidor.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void periodo(void)
{
    const int M = 15;                                   /* consumo_margen_min */
    med_periodo_t p = {0}; int h = -1;
    /* contrato de FR-MED-10: 30 min; a los 5 min, 60 min; a los 6 min, 10 min */
    OK(med_permitir(&p, 0, 30, M, &h) == MED_EVENTO && h == 30 + M, "30 min: hasta_min %d", h);
    h = -1;
    OK(med_permitir(&p, 300, 60, M, &h) == MED_EVENTO && h == 60 + M, "60 min a los 5 min: hasta_min %d", h);
    h = -1;
    OK(med_permitir(&p, 360, 10, M, &h) == MED_SIN_CAMBIO && h == -1, "10 min a los 6 min no mueve el fin y no da evento");
    OK(p.fin_s == 300 + (60 + M) * 60u, "el periodo se acortó: fin %u", (unsigned)p.fin_s);
    /* inválidas: periodo sin cambio (FR-MED-06, TC-MED-A7) */
    med_periodo_t antes = p;
    OK(med_permitir(&p, 400, 0, M, &h) == MED_RECHAZADA && med_permitir(&p, 400, 721, M, &h) == MED_RECHAZADA &&
       med_permitir(&p, 400, -5, M, &h) == MED_RECHAZADA && p.fin_s == antes.fin_s, "orden inválida aceptada o con efecto");
    OK(med_permitir(&p, 400, 720, M, &h) == MED_EVENTO, "720 min rechazado");
    /* dentro / fuera, con el margen */
    med_periodo_t q = {0};
    OK(!med_dentro(&q, 0), "dentro sin periodo");
    med_permitir(&q, 1000, 30, M, &h);
    OK(med_dentro(&q, 1000) && med_dentro(&q, 1000 + (30 + M) * 60 - 1) && !med_dentro(&q, 1000 + (30 + M) * 60),
       "límites del periodo con margen");
    /* vencido: una orden nueva abre otro periodo y da evento aunque su fin sea menor que el viejo */
    h = -1;
    OK(med_permitir(&q, 1000 + (30 + M) * 60 + 10, 5, M, &h) == MED_EVENTO && h == 5 + M, "periodo vencido: sin evento al reabrir");
    /* reinicio: el periodo vive en RAM */
    med_periodo_t r = {0};
    OK(!med_dentro(&r, 50), "periodo tras reinicio");
}

/* Cuenta los pulsos de una señal muestreada cada 1 ms: cerrado(t) */
static int contar(bool (*cerrado)(uint32_t), uint32_t hasta_ms)
{
    med_pulso_t d = {0}; int n = 0;
    for (uint32_t t = 0; t <= hasta_ms; t++) n += med_muestra(&d, t, cerrado(t));
    return n;
}
static bool s_60_60(uint32_t t)  { return t >= 100 && (t - 100) % 120 < 60; }            /* 60 ms cerrado / 60 abierto */
static bool s_40(uint32_t t)     { return t >= 100 && t < 140; }                          /* un cierre de 40 ms */
static bool s_rebotes(uint32_t t)                                                         /* cierre de 200 ms con 5 rebotes de 3 ms */
{
    if (t < 100 || t >= 300) return false;
    uint32_t d = t - 100;
    return !(d < 30 && (d / 3) % 2 == 1);
}
static bool s_5hz(uint32_t t)    { return t >= 100 && (t - 100) % 200 < 100; }            /* 5 pulsos/s */

static void pulsos(void)
{
    int n = contar(s_60_60, 100 + 120 * 10 - 1);
    OK(n == 10, "60/60 ms: %d pulsos (esperado 10)", n);
    OK(contar(s_40, 1000) == 0, "cierre de 40 ms contado");
    OK(contar(s_rebotes, 1000) == 1, "cierre con rebotes: %d", contar(s_rebotes, 1000));
    n = contar(s_5hz, 100 + 200 * 50 - 1);
    OK(n == 50, "5 pulsos/s durante 10 s: %d", n);
    /* FR-MED-02: 10 pulsos a 1,0 gal → 37,85 L; cambio a 0,5 y 10 más → 56,78 L; el total nunca baja */
    med_total_t t = {0}; double antes = 0; bool baja = false;
    for (int k = 0; k < 10; k++) { med_sumar(&t, 1.0); baja |= t.total_l < antes; antes = t.total_l; }
    OK(fabs(t.total_l - 37.85) < 0.01, "10 pulsos a 1,0 gal: %.3f L", t.total_l);
    for (int k = 0; k < 10; k++) { med_sumar(&t, 0.5); baja |= t.total_l < antes; antes = t.total_l; }
    /* FR-MED-04: un pulso de 1 gal cada 6 s → 10 en 60 s → 2271 L/h */
    med_total_t q = {0};
    for (int k = 0; k < 10; k++) med_sumar(&q, 1.0);
    OK(fabs(med_caudal_lh(q.total_l, 60) - 2271.2) < 1.0 && med_caudal_lh(5, 0) == 0, "caudal %.1f L/h", med_caudal_lh(q.total_l, 60));
    OK(fabs(t.total_l - 56.78) < 0.01 && t.pulsos == 20 && !baja, "cambio a 0,5 gal: %.3f L, %u pulsos", t.total_l, (unsigned)t.pulsos);
}

static void nvs(void)
{
    /* pulsos seguidos 5 min (uno por segundo) → ≤ 1 escritura cada 60 s; 10 min sin pulsos → 0 */
    med_guardado_t g = {0}; uint32_t pulsos = 0; int escrituras = 0; uint32_t ultima = 0; bool cerca = false;
    for (uint32_t t = 0; t < 300; t++) {
        pulsos++;
        if (med_toca_guardar(&g, t, pulsos)) {
            if (escrituras && t - ultima < 60) cerca = true;
            med_guardado(&g, t, pulsos); escrituras++; ultima = t;
        }
    }
    OK(escrituras <= 6 && escrituras >= 5 && !cerca, "5 min de pulsos: %d escrituras", escrituras);
    if (med_toca_guardar(&g, 300 + 60, pulsos)) med_guardado(&g, 360, pulsos);          /* lo que quedaba */
    int sin = 0;
    for (uint32_t t = 361; t < 361 + 600; t++) if (med_toca_guardar(&g, t, pulsos)) { sin++; med_guardado(&g, t, pulsos); }
    OK(sin == 0, "10 min sin pulsos: %d escrituras", sin);
}

static void consumo(void)
{
    const int V = 60, AL = 20, DI = 50;
    /* FR-MED-05 × FR-MED-06: 21 L en la ventana fuera de periodo → un evento hora solo con casa_cerrada=true */
    for (int cc = 0; cc < 2; cc++) for (int lit = 19; lit <= 21; lit += 2) for (int per = 0; per < 4; per++) {
        static med_consumo_t c; memset(&c, 0, sizeof c);
        med_periodo_t p = {0}; int h;
        uint32_t t0 = 100000;
        if (per == 1) med_permitir(&p, t0 - 600, 60, 0, &h);           /* dentro */
        if (per == 2) med_permitir(&p, t0 - 3600, 50, 15, &h);         /* en el margen */
        if (per == 3) med_permitir(&p, t0 - 7200, 30, 15, &h);         /* vencido */
        int avisos = 0;
        for (int k = 0; k < lit; k++) {
            uint32_t t = t0 + (uint32_t)k * 60;
            med_consumo_pulso(&c, t, 1.0, !med_dentro(&p, t));
            avisos += med_consumo_evaluar(&c, t, cc, V, AL, DI).hora;
        }
        int esperado = cc && lit == 21 && (per == 0 || per == 3);
        OK(avisos == esperado, "casa_cerrada %d, %d L, periodo %d: %d avisos (esperado %d)", cc, lit, per, avisos, esperado);
    }
    /* uno por episodio; el siguiente solo tras una ventana completa sin pulsos */
    {
        static med_consumo_t c; memset(&c, 0, sizeof c); int avisos = 0;
        for (uint32_t t = 0; t < 3 * 3600; t += 60) { med_consumo_pulso(&c, t, 1.0, true); avisos += med_consumo_evaluar(&c, t, true, V, AL, 100000).hora; }
        OK(avisos == 1, "3 h de consumo seguido: %d avisos de hora", avisos);
        uint32_t t1 = 3 * 3600 + 30 * 60;
        avisos += med_consumo_evaluar(&c, t1, true, V, AL, 100000).hora;
        for (uint32_t t = t1; t < t1 + 1800; t += 60) { med_consumo_pulso(&c, t, 1.0, true); avisos += med_consumo_evaluar(&c, t, true, V, AL, 100000).hora; }
        OK(avisos == 1, "otro aviso sin una ventana completa sin pulsos: %d", avisos);
        uint32_t t2 = t1 + 1800 + 3600;
        avisos += med_consumo_evaluar(&c, t2, true, V, AL, 100000).hora;
        for (uint32_t t = t2; t < t2 + 1800; t += 60) { med_consumo_pulso(&c, t, 1.0, true); avisos += med_consumo_evaluar(&c, t, true, V, AL, 100000).hora; }
        OK(avisos == 2, "tras una ventana sin pulsos: %d avisos (esperado 2)", avisos);
    }
    /* FR-MED-09: 2,5 L/h durante 30 h → con true un evento diario a la hora 20 (50 L) y ninguno más en 24 h;
     * con false o dentro de periodos, ninguno; ningún aviso de hora (2,5 L < 20 L) */
    for (int caso = 0; caso < 3; caso++) {
        static med_consumo_t c; memset(&c, 0, sizeof c);
        med_periodo_t p = {0}; int h;
        int dias = 0, horas = 0; uint32_t t_dia = 0;
        for (uint32_t t = 0; t < 30 * 3600; t += 360) {               /* 0,25 L cada 6 min = 2,5 L/h */
            if (caso == 2) med_permitir(&p, t, 720, 0, &h);
            med_consumo_pulso(&c, t, 0.25, !med_dentro(&p, t));
            med_aviso_t a = med_consumo_evaluar(&c, t, caso != 1, V, AL, DI);
            if (a.dia) { dias++; if (!t_dia) t_dia = t; }
            horas += a.hora;
        }
        int esperado = caso == 0;
        OK(dias == esperado && horas == 0, "diario caso %d: %d avisos diarios, %d de hora", caso, dias, horas);
        if (caso == 0) OK(t_dia >= 20 * 3600 - 360 && t_dia <= 20 * 3600 + 360, "aviso diario a las %.2f h", t_dia / 3600.0);
    }
    /* la suma de 24 h vive en RAM: tras un reinicio vuelve a 0 */
    { static med_consumo_t c; memset(&c, 0, sizeof c);
      for (uint32_t t = 0; t < 40; t++) med_consumo_pulso(&c, t * 60, 1.0, true);
      memset(&c, 0, sizeof c);
      OK(!med_consumo_evaluar(&c, 5000, true, V, 1000, 10).dia, "suma de 24 h tras el reinicio"); }
    /* FR-MED-08: el nivel sube 10 cm sin pulsos → un medidor_falla, hasta el siguiente pulso */
    med_falla_t f = {0}; int fallas = 0;
    for (int k = 0; k <= 30; k++) fallas += med_falla_nivel(&f, 100.0f + k * 0.5f);
    OK(fallas == 1, "subida de 15 cm sin pulsos: %d avisos", fallas);
    med_falla_pulso(&f);
    fallas = 0;
    for (int k = 0; k <= 30; k++) { if (k % 5 == 0) med_falla_pulso(&f); fallas += med_falla_nivel(&f, 120.0f + k * 0.5f); }
    OK(fallas == 0, "subida con pulsos: %d avisos", fallas);
    fallas = 0; med_falla_pulso(&f);
    for (int k = 0; k <= 18; k++) fallas += med_falla_nivel(&f, 150.0f + k * 0.5f);
    OK(fallas == 0, "subida de 9 cm: %d avisos", fallas);
}

int main(int argc, char **argv)
{
    const char *c = argc > 1 ? argv[1] : "periodo";
    if (!strcmp(c, "periodo")) periodo();
    else if (!strcmp(c, "pulsos")) pulsos();
    else if (!strcmp(c, "nvs")) nvs();
    else if (!strcmp(c, "consumo")) consumo();
    else { printf("caso desconocido: %s\n", c); return 2; }
    printf("medidor %s: %d fallos\n", c, fallos);
    return fallos != 0;
}
