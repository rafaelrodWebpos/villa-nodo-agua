/* TC-PUB-A4 · FR-PUB-04/05 (FSD 0.7.1, P23) en host: orden de llegada, edad_s, ts solo con
 * hora válida, solo eventos críticos, y con la cola de 32 llena nunca se pierde un `fuga`
 * mientras quede otro evento que descartar. */
#include <stdio.h>
#include <string.h>
#include "cola_eventos.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static cev_t ev(const char *tipo, int n, uint32_t t_ms)
{
    cev_t e = { .tipo = tipo, .t_ms = t_ms };
    snprintf(e.extra, sizeof e.extra, "\"n\":%d", n);
    return e;
}

int main(void)
{
    cola_ev_t c;
    cev_t s;
    char buf[160];

    /* orden, edad_s y ts */
    cev_iniciar(&c);
    cev_t a = ev("fuga", 3, 1000); a.ts_valido = true; a.ts = 1795000000u;
    OK(cev_poner(&c, &a), "fuga no crítico");
    cev_t b = ev("falla_sonda", 6, 7000);
    OK(cev_poner(&c, &b), "falla_sonda no crítico");
    cev_t r = ev("orden_rechazada", 0, 8000);
    OK(!cev_poner(&c, &r) && c.n == 2, "orden_rechazada guardada");
    OK(cev_sacar(&c, &s) && !strcmp(s.tipo, "fuga"), "orden de llegada (1)");
    OK(cev_json(buf, sizeof buf, &s, 181000) > 0 &&
       !strcmp(buf, "{\"tipo\":\"fuga\",\"n\":3,\"ts\":1795000000,\"edad_s\":180}"), "json fuga: %s", buf);
    OK(cev_sacar(&c, &s) && !strcmp(s.tipo, "falla_sonda"), "orden de llegada (2)");
    OK(cev_json(buf, sizeof buf, &s, 181000) > 0 && !strcmp(buf, "{\"tipo\":\"falla_sonda\",\"n\":6,\"edad_s\":174}"),
       "json sin hora válida: %s", buf);
    OK(!cev_sacar(&c, &s), "cola vacía");
    cev_t sin = { .tipo = "falla_vref", .t_ms = 0 };
    OK(cev_json(buf, sizeof buf, &sin, 5000) > 0 && !strcmp(buf, "{\"tipo\":\"falla_vref\",\"edad_s\":5}"), "json sin extra: %s", buf);
    OK(cev_json(buf, 10, &a, 0) == -1 && buf[0] == 0, "json que no cabe");

    /* FR-PUB-05: 40 eventos con 2 fuga entre ellos, cola de 32 → 8 perdidos, los 2 fuga siguen */
    cev_iniciar(&c);
    for (int i = 0; i < 40; i++) {
        cev_t e = ev(i == 3 || i == 20 ? "fuga" : "falla_sonda", i, (uint32_t)i * 1000u);
        cev_poner(&c, &e);
    }
    OK(c.n == CEV_CAP && c.perdidos == 8, "n=%d perdidos=%u", c.n, (unsigned)c.perdidos);
    int fugas = 0, previo = -1, ordenado = 1;
    while (cev_sacar(&c, &s)) {
        int n; sscanf(s.extra, "\"n\":%d", &n);
        fugas += !strcmp(s.tipo, "fuga");
        ordenado &= n > previo; previo = n;
    }
    OK(fugas == 2 && ordenado, "fugas conservadas %d, orden %d", fugas, ordenado);

    /* todos fuga: se descarta el fuga más antiguo */
    cev_iniciar(&c);
    for (int i = 0; i < CEV_CAP + 1; i++) { cev_t e = ev("fuga", i, 0); cev_poner(&c, &e); }
    OK(c.perdidos == 1 && cev_sacar(&c, &s) && !strcmp(s.extra, "\"n\":1"), "todos fuga: sale el más antiguo (%s)", s.extra);

    /* los críticos de FR-PUB-04 */
    const char *crit[] = { "fuga", "falla_valvula", "movimiento_manual", "falla_sonda", "falla_adc", "falla_vref",
                           "bomba_seco", "bomba_bloqueada", "ciclado_anormal", "bateria_baja", "bateria_critica", "flotador_falla" };
    for (size_t i = 0; i < sizeof crit / sizeof crit[0]; i++) OK(cev_es_critico(crit[i]), "%s no crítico", crit[i]);
    OK(!cev_es_critico("orden_rechazada") && !cev_es_critico(NULL) && !cev_es_critico(""), "no críticos");

    printf("cola_eventos: %d fallos\n", fallos);
    return fallos != 0;
}
