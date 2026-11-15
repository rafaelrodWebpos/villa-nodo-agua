/* L2 bomba — FR-NIV-01/02, FR-PRE-01 y FR-CIC-01/02. Núcleo puro. */
#include "bomba.h"

static float acotar(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }

bool bom_nivel(float ma, int rango_cm, int util_cm, float *cm, float *pct)
{
    if (ma < 3.8f || ma > 20.5f || rango_cm <= 0 || util_cm <= 0) return false;
    float c = acotar((ma - 4.0f) / 16.0f * (float)rango_cm, 0.0f, (float)rango_cm);
    *cm = c;
    *pct = acotar(c / (float)util_cm * 100.0f, 0.0f, 100.0f);
    return true;
}

bool bom_presion(float v, float *bar)
{
    if (v < 0.3f || v > 4.7f) return false;
    *bar = (v - 0.5f) / 4.0f * 10.0f;
    return true;
}

bool cic_arranque(cic_t *c, uint32_t t_s, uint32_t ventana_s, int max, int *n)
{
    int k = 0;                                           /* quita los que salieron de la ventana */
    for (int i = 0; i < c->n; i++)
        if (t_s - c->t[i] < ventana_s) c->t[k++] = c->t[i];
    c->n = k;
    if (c->n == CIC_MAX_ARRANQUES) {                     /* lleno: descarta el más antiguo */
        for (int i = 1; i < c->n; i++) c->t[i - 1] = c->t[i];
        c->n--;
    }
    c->t[c->n++] = t_s;
    *n = c->n;
    if (c->avisado && t_s < c->aviso_hasta) return false;   /* un aviso por ventana */
    if (max < 1 || c->n < max) return false;
    c->avisado = true;
    c->aviso_hasta = t_s + ventana_s;
    return true;
}

/* ---- Permiso de la bomba (B01–B09) ---- */
#define DIA_S (24u * 3600u)

void bom_iniciar(bom_t *b, int intentos_max, int nivel_critico_pct)
{
    *b = (bom_t){ .estado = BOM_PERMITIDA, .intentos_max = intentos_max, .nivel_critico_pct = nivel_critico_pct };
}

int bom_intentos_24h(bom_t *b, uint32_t t_s)
{
    int k = 0;
    for (int i = 0; i < b->n_cortes; i++)
        if (t_s - b->cortes[i] < DIA_S) b->cortes[k++] = b->cortes[i];
    b->n_cortes = k;
    return k;
}

static bom_accion_t cortar(bom_t *b, int fila, bom_motivo_t m)
{
    b->estado = BOM_CORTADA;
    b->motivo = m;
    return (bom_accion_t){ .fila = fila };
}

bom_accion_t bom_evento(bom_t *b, const bom_entrada_t *e)
{
    const bom_accion_t ignorar = { 0 };
    if (b->estado == BOM_PERMITIDA) {
        switch (e->evento) {
        case BEV_SECO:
            bom_intentos_24h(b, e->t_s);
            if (b->n_cortes == BOM_MAX_CORTES) {          /* lleno: descarta el más antiguo */
                for (int i = 1; i < b->n_cortes; i++) b->cortes[i - 1] = b->cortes[i];
                b->n_cortes--;
            }
            b->cortes[b->n_cortes++] = e->t_s;            /* intentos_24h += 1 */
            return cortar(b, 1, BMOT_SECO);
        case BEV_FLOTADOR_SIN_AGUA:
            return cortar(b, 2, BMOT_FLOTADOR);
        case BEV_NIVEL:
            if (!e->nivel_valido || e->nivel_pct >= (float)b->nivel_critico_pct) return ignorar;
            return cortar(b, 3, BMOT_NIVEL);
        case BEV_CORTAR:
            return cortar(b, 4, BMOT_ORDEN);
        default:
            return ignorar;
        }
    }
    /* CORTADA o BLOQUEADA */
    switch (e->evento) {
    case BEV_REARME_CUMPLIDO:
        if (b->estado != BOM_CORTADA || b->motivo == BMOT_ORDEN) return ignorar;
        /* B05: los cortes por nivel o flotador no cuentan como intentos y rearman siempre */
        if (b->motivo != BMOT_SECO || bom_intentos_24h(b, e->t_s) < b->intentos_max) {
            b->estado = BOM_PERMITIDA; b->motivo = BMOT_NINGUNO;
            return (bom_accion_t){ .fila = 5 };
        }
        b->estado = BOM_BLOQUEADA; b->motivo = BMOT_NINGUNO;
        return (bom_accion_t){ .fila = 6, .evento_bloqueada = true };
    case BEV_REARMAR:
        if (!e->flotador_agua) return (bom_accion_t){ .fila = 8, .rechazo = BRECH_SIN_AGUA };
        b->estado = BOM_PERMITIDA; b->motivo = BMOT_NINGUNO;
        b->n_cortes = 0;                                  /* intentos_24h = 0 */
        return (bom_accion_t){ .fila = 7 };
    case BEV_PERMITIR:
        if (b->estado != BOM_CORTADA || b->motivo != BMOT_ORDEN) return ignorar;
        if (!e->flotador_agua) return (bom_accion_t){ .fila = 10, .rechazo = BRECH_SIN_AGUA };  /* B10, FR-BOM-08 */
        if (e->nivel_valido && e->nivel_pct < (float)b->nivel_critico_pct)
            return (bom_accion_t){ .fila = 11, .rechazo = BRECH_NIVEL_BAJO };                   /* B11 */
        b->estado = BOM_PERMITIDA; b->motivo = BMOT_NINGUNO;
        return (bom_accion_t){ .fila = 9 };
    default:
        return ignorar;
    }
}

bool sec_muestra(sec_t *s, uint32_t t_ms, float i_a, float p_bar,
                 float i_marcha, float i_seco, float p_min, uint32_t t_seco_s)
{
    bool cond = i_seco > 0.0f && i_a >= i_marcha && i_a < i_seco && p_bar < p_min;
    if (!cond) { s->en_curso = false; return false; }
    if (!s->en_curso) { s->en_curso = true; s->desde_ms = t_ms; }
    return t_ms - s->desde_ms > t_seco_s * 1000u;
}

bool rea_tick(rea_t *r, uint32_t t_s, bool nivel_valido, float nivel_pct, bool flotador_agua,
              int nivel_rearme_pct, uint32_t t_rearme_s)
{
    bool cond = nivel_valido && flotador_agua && nivel_pct >= (float)nivel_rearme_pct;
    if (!cond) { r->en_curso = false; return false; }
    if (!r->en_curso) { r->en_curso = true; r->desde_s = t_s; }
    if (t_s - r->desde_s < t_rearme_s) return false;
    r->desde_s = t_s;                                    /* vuelve a contar */
    return true;
}
