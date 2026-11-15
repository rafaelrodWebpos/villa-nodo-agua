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
