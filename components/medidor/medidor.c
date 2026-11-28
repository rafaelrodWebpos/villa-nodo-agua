/* L2 medidor — FR-MED-01…10 (FSD §6.7). Núcleo puro. */
#include "medidor.h"

med_resultado_t med_permitir(med_periodo_t *p, uint32_t ahora_s, int min, int margen_min, int *hasta_min)
{
    if (min < 1 || min > MED_MIN_MAX || margen_min < 0) return MED_RECHAZADA;
    uint32_t fin = ahora_s + (uint32_t)(min + margen_min) * 60u;
    if (p->hay && med_dentro(p, ahora_s) && (int32_t)(fin - p->fin_s) <= 0) return MED_SIN_CAMBIO;
    p->hay = true;
    p->fin_s = fin;
    if (hasta_min) *hasta_min = min + margen_min;
    return MED_EVENTO;
}

bool med_dentro(const med_periodo_t *p, uint32_t ahora_s)
{
    return p->hay && (int32_t)(p->fin_s - ahora_s) > 0;
}

bool med_muestra(med_pulso_t *d, uint32_t t_ms, bool cerrado)
{
    if (!d->inicio) {                                    /* el primer valor es el estado de partida */
        *d = (med_pulso_t){ .inicio = true, .estable = cerrado, .cand = cerrado, .cand_desde = t_ms,
                            .estable_desde = t_ms - (cerrado ? 0u : MED_ANTIRREBOTE_MS) };
        return false;
    }
    if (cerrado != d->cand) { d->cand = cerrado; d->cand_desde = t_ms; }
    if (d->cand == d->estable || t_ms - d->cand_desde < MED_ANTIRREBOTE_MS) return false;
    uint32_t duro = d->cand_desde - d->estable_desde;    /* lo que duró el estado anterior */
    d->estable = d->cand;
    d->estable_desde = d->cand_desde;
    return d->estable && duro >= MED_ANTIRREBOTE_MS;     /* cierre tras ≥ 50 ms abierto */
}

double med_sumar(med_total_t *t, double galones_por_pulso)
{
    double l = galones_por_pulso > 0 ? galones_por_pulso * MED_L_POR_GAL : 0.0;
    t->pulsos++;
    t->total_l += l;
    return l;
}

double med_caudal_lh(double litros, uint32_t dt_s)
{
    return dt_s ? litros * 3600.0 / (double)dt_s : 0.0;
}

bool med_toca_guardar(const med_guardado_t *g, uint32_t t_s, uint32_t pulsos)
{
    if (g->hecho && pulsos == g->pulsos) return false;
    return !g->hecho || t_s - g->t_s >= MED_GUARDAR_S;
}

void med_guardado(med_guardado_t *g, uint32_t t_s, uint32_t pulsos)
{
    *g = (med_guardado_t){ .pulsos = pulsos, .t_s = t_s, .hecho = true };
}

/* Avanza el anillo hasta el minuto de t_s, vaciando los minutos que pasaron */
static void avanzar(med_consumo_t *c, uint32_t t_s)
{
    uint32_t m = t_s / 60u;
    if (!c->inicio) { c->inicio = true; c->min_actual = m; return; }
    uint32_t pasos = m - c->min_actual;
    if (pasos > MED_MINUTOS) pasos = MED_MINUTOS;
    for (uint32_t k = 1; k <= pasos; k++) c->l[(c->min_actual + k) % MED_MINUTOS] = 0.0f;
    c->min_actual = m;
}

static float suma(const med_consumo_t *c, int minutos)
{
    float s = 0.0f;
    if (minutos > MED_MINUTOS) minutos = MED_MINUTOS;
    for (int k = 0; k < minutos; k++) s += c->l[(c->min_actual + MED_MINUTOS - (uint32_t)k) % MED_MINUTOS];
    return s;
}

void med_consumo_pulso(med_consumo_t *c, uint32_t t_s, double litros, bool fuera)
{
    avanzar(c, t_s);
    c->hay_pulso = true;
    c->ultimo_pulso_s = t_s;
    if (fuera) c->l[c->min_actual % MED_MINUTOS] += (float)litros;
}

med_aviso_t med_consumo_evaluar(med_consumo_t *c, uint32_t t_s, bool casa_cerrada,
                                int ventana_min, int alerta_l, int diario_l)
{
    med_aviso_t a = {0};
    avanzar(c, t_s);
    /* FR-MED-05: el episodio termina tras una ventana completa sin pulsos */
    if (c->avisado_hora && (!c->hay_pulso || t_s - c->ultimo_pulso_s >= (uint32_t)ventana_min * 60u))
        c->avisado_hora = false;
    if (!casa_cerrada) return a;
    float lh = suma(c, ventana_min);
    if (!c->avisado_hora && lh > (float)alerta_l) { c->avisado_hora = true; a.hora = true; a.l_hora = lh; }
    float ld = suma(c, MED_MINUTOS);                     /* FR-MED-09: una vez cada 24 h desde el aviso */
    if (ld > (float)diario_l && (!c->avisado_dia || t_s - c->aviso_dia_s >= 86400u)) {
        c->avisado_dia = true; c->aviso_dia_s = t_s; a.dia = true; a.l_dia = ld;
    }
    return a;
}

void med_falla_pulso(med_falla_t *f) { *f = (med_falla_t){0}; }

bool med_falla_nivel(med_falla_t *f, float nivel_cm)
{
    if (!f->hay_base || nivel_cm < f->base_cm) { f->hay_base = true; f->base_cm = nivel_cm; }
    if (f->avisado || nivel_cm - f->base_cm < MED_FALLA_CM) return false;
    f->avisado = true;
    return true;
}
