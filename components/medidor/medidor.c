/* L2 medidor — FR-MED-06 y FR-MED-10. Núcleo puro. */
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
