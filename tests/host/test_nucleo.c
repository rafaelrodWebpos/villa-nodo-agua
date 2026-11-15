/* Núcleos puros de §5–§7: TC-FUG-A1 (FR-FUG-01), TC-ENE-A1 (FR-ENE-02), TC-NIV-A1 (FR-NIV-01),
 * TC-PRE-A1 (FR-PRE-01), TC-CIC-A1 (FR-CIC-01/02). Uso: test_nucleo <caso> */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bomba.h"
#include "energia.h"
#include "fugas.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define CERCA(a, b) (fabsf((a) - (b)) < 1e-3f)

static void fug(void)
{
    fug_sonda_t s = {0};
    OK(!fug_muestra(&s, true) && !fug_muestra(&s, true) && !fug_muestra(&s, true), "confirma con < 4");
    OK(fug_muestra(&s, true), "no confirma con 4 seguidas");
    OK(fug_muestra(&s, true), "deja de confirmar con la 5.ª");
    memset(&s, 0, sizeof s);
    fug_muestra(&s, true); fug_muestra(&s, true); fug_muestra(&s, true);
    OK(!fug_muestra(&s, false), "3 + 1 seca confirma");
    OK(!fug_muestra(&s, true) && !fug_muestra(&s, true) && !fug_muestra(&s, true), "tras la seca, confirma con < 4");
    OK(fug_muestra(&s, true), "tras la seca, no confirma con 4 nuevas");
}

static void ene(void)
{
    ene_estado_t e = ENE_OK;                     /* rampa de bajada 13,00 → 11,50 en pasos de 0,05 V */
    float a_baja = 0, a_crit = 0;
    for (int mv = 13000; mv >= 11500; mv -= 50) {
        ene_estado_t n = ene_clasificar(e, mv / 1000.0f);
        if (n != e && n == ENE_BAJA) a_baja = mv / 1000.0f;
        if (n != e && n == ENE_CRITICA) a_crit = mv / 1000.0f;
        e = n;
    }
    OK(CERCA(a_baja, 12.35f) && CERCA(a_crit, 11.95f), "bajada: baja en %.2f, crítica en %.2f (esperado 12,35 y 11,95: por debajo de 12,4/12,0)", a_baja, a_crit);
    float s_baja = 0, s_ok = 0;                   /* rampa de subida 11,50 → 13,00 */
    for (int mv = 11500; mv <= 13000; mv += 50) {
        ene_estado_t n = ene_clasificar(e, mv / 1000.0f);
        if (n != e && n == ENE_BAJA) s_baja = mv / 1000.0f;
        if (n != e && n == ENE_OK) s_ok = mv / 1000.0f;
        e = n;
    }
    OK(CERCA(s_baja, 12.2f) && CERCA(s_ok, 12.6f), "subida: baja en %.2f, ok en %.2f (esperado 12,20 y 12,60)", s_baja, s_ok);
    OK(ene_clasificar(ENE_CRITICA, 12.7f) == ENE_OK, "de crítica a ok de un salto");
    OK(ene_clasificar(ENE_OK, 11.9f) == ENE_CRITICA, "de ok a crítica de un salto");
    /* ruido ± 0,1 V alrededor de cada umbral: un cambio como mucho, sin oscilar */
    const float centros[] = {12.4f, 12.0f};
    for (int k = 0; k < 2; k++) for (ene_estado_t ini = ENE_OK; ini <= ENE_CRITICA; ini++) {
        e = ini;
        int cambios = 0;
        for (int i = 0; i < 40; i++) {
            ene_estado_t n = ene_clasificar(e, centros[k] + ((i & 1) ? 0.1f : -0.1f) * (i % 3 ? 1.0f : 0.5f));
            cambios += n != e;
            e = n;
        }
        OK(cambios <= 1, "oscila con ruido ± 0,1 V en %.1f desde %s: %d cambios", centros[k], ene_nombre(ini), cambios);
    }
    OK(!strcmp(ene_nombre(ENE_OK), "ok") && !strcmp(ene_nombre(ENE_BAJA), "baja") && !strcmp(ene_nombre(ENE_CRITICA), "critica"), "nombres");
}

static void niv(void)
{
    float cm = -1, pct = -1;
    OK(bom_nivel(4.0f, 500, 400, &cm, &pct) && CERCA(cm, 0) && CERCA(pct, 0), "4 mA: %.2f cm %.2f %%", cm, pct);
    OK(bom_nivel(12.0f, 500, 400, &cm, &pct) && CERCA(cm, 250) && CERCA(pct, 62.5f), "12 mA: %.2f cm %.2f %%", cm, pct);
    OK(bom_nivel(20.0f, 500, 400, &cm, &pct) && CERCA(cm, 500) && CERCA(pct, 100), "20 mA: %.2f cm %.2f %% (saturado)", cm, pct);
    for (float ma = 3.8f; ma <= 20.5f; ma += 0.1f)
        OK(bom_nivel(ma, 500, 400, &cm, &pct) && pct <= 100.0f && pct >= 0.0f && cm >= 0.0f, "%.1f mA: %% fuera de 0–100", ma);
    OK(!bom_nivel(3.7f, 500, 400, &cm, &pct) && !bom_nivel(20.6f, 500, 400, &cm, &pct), "fuera de 3,8–20,5 mA no es falla (FR-NIV-02)");
}

static void pre(void)
{
    float b = -1;
    OK(bom_presion(0.5f, &b) && CERCA(b, 0), "0,5 V: %.3f bar", b);
    OK(bom_presion(2.5f, &b) && CERCA(b, 5), "2,5 V: %.3f bar", b);
    OK(bom_presion(4.5f, &b) && CERCA(b, 10), "4,5 V: %.3f bar", b);
    OK(!bom_presion(0.29f, &b) && !bom_presion(4.71f, &b), "fuera de 0,3–4,7 V no es falla");
}

static void cic(void)
{
    const uint32_t V = 60 * 60;                    /* ciclado_ventana_min = 60 */
    cic_t c = {0};
    int n = 0, avisos = 0, n_aviso = 0;
    for (int i = 0; i < 6; i++) if (cic_arranque(&c, 1000 + i * 600, V, 6, &n)) { avisos++; n_aviso = n; }   /* 6 en 50 min */
    OK(avisos == 1 && n_aviso == 6, "6 arranques en 50 min: %d avisos (n=%d)", avisos, n_aviso);
    OK(!cic_arranque(&c, 1000 + 6 * 600, V, 6, &n), "7.º arranque en la misma ventana: segundo aviso");
    memset(&c, 0, sizeof c);
    avisos = 0;
    for (int i = 0; i < 5; i++) avisos += cic_arranque(&c, 1000 + i * 900, V, 6, &n);   /* 5 en 60 min */
    OK(avisos == 0, "5 arranques en 60 min: %d avisos", avisos);
    memset(&c, 0, sizeof c);                       /* ventana móvil: 5 viejos + 1 fuera de ventana = sin aviso */
    avisos = 0;
    for (int i = 0; i < 5; i++) avisos += cic_arranque(&c, 1000 + i * 60, V, 6, &n);
    avisos += cic_arranque(&c, 1000 + V, V, 6, &n);
    OK(avisos == 0 && n == 5, "arranque fuera de la ventana contado: n=%d avisos=%d", n, avisos);
}

int main(int argc, char **argv)
{
    const char *caso = argc > 1 ? argv[1] : "";
    if (!strcmp(caso, "fug")) fug();
    else if (!strcmp(caso, "ene")) ene();
    else if (!strcmp(caso, "niv")) niv();
    else if (!strcmp(caso, "pre")) pre();
    else if (!strcmp(caso, "cic")) cic();
    else { printf("caso desconocido: %s\n", caso); return 2; }
    printf("%s: %d fallos\n", caso, fallos);
    return fallos != 0;
}
