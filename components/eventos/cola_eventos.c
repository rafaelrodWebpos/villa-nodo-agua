/* L1 eventos — FR-PUB-04/05 (FSD 0.7.1). Núcleo puro. */
#include "cola_eventos.h"
#include <stdio.h>
#include <string.h>

static const char *const CRITICOS[] = {
    "fuga", "falla_valvula", "movimiento_manual", "falla_sonda", "falla_adc", "falla_vref",
    "bomba_seco", "bomba_bloqueada", "ciclado_anormal", "bateria_baja", "bateria_critica",
    "flotador_falla",
};

bool cev_es_critico(const char *tipo)
{
    if (!tipo) return false;
    for (size_t i = 0; i < sizeof CRITICOS / sizeof CRITICOS[0]; i++)
        if (!strcmp(tipo, CRITICOS[i])) return true;
    return false;
}

void cev_iniciar(cola_ev_t *c) { memset(c, 0, sizeof *c); }

static cev_t *en(cola_ev_t *c, int i) { return &c->e[(c->ini + i) % CEV_CAP]; }

/* Quita la posición i (0 = la más antigua) conservando el orden de las demás */
static void quitar(cola_ev_t *c, int i)
{
    for (int k = i; k > 0; k--) *en(c, k) = *en(c, k - 1);
    c->ini = (c->ini + 1) % CEV_CAP;
    c->n--;
}

bool cev_poner(cola_ev_t *c, const cev_t *ev)
{
    if (!cev_es_critico(ev->tipo)) return false;
    if (c->n == CEV_CAP) {
        int victima = 0;                                   /* todos fuga: el fuga más antiguo */
        for (int i = 0; i < c->n; i++) if (strcmp(en(c, i)->tipo, "fuga")) { victima = i; break; }
        quitar(c, victima);
        c->perdidos++;
    }
    *en(c, c->n) = *ev;
    c->n++;
    return true;
}

bool cev_sacar(cola_ev_t *c, cev_t *ev)
{
    if (c->n == 0) return false;
    *ev = *en(c, 0);
    c->ini = (c->ini + 1) % CEV_CAP;
    c->n--;
    return true;
}

int cev_json(char *buf, size_t cap, const cev_t *ev, uint32_t ahora_ms)
{
    char ts[24] = "";
    if (ev->ts_valido) snprintf(ts, sizeof ts, ",\"ts\":%lu", (unsigned long)ev->ts);
    int n = snprintf(buf, cap, "{\"tipo\":\"%s\"%s%s%s,\"edad_s\":%lu}", ev->tipo, ev->extra[0] ? "," : "",
                     ev->extra, ts, (unsigned long)((ahora_ms - ev->t_ms) / 1000u));
    if (n < 0 || (size_t)n >= cap) { if (cap) buf[0] = 0; return -1; }
    return n;
}
