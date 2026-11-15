/* L2 energia — FR-ENE-02 (P5). Núcleo puro. */
#include "energia.h"
#include <stdbool.h>

ene_estado_t ene_clasificar(ene_estado_t anterior, float v)
{
    /* Cada umbral tiene dos caras: se cruza hacia abajo en U y hacia arriba en U + H.
     * Entre las dos se conserva el lado del estado anterior. Las comparaciones se hacen
     * en mV enteros para que 12,6 V sea exactamente el umbral de subida. */
    long mv = (long)(v * 1000.0f + (v >= 0 ? 0.5f : -0.5f));
    const long baja = (long)(ENE_BAJA_V * 1000.0f + 0.5f), crit = (long)(ENE_CRITICA_V * 1000.0f + 0.5f),
               h = (long)(ENE_HISTERESIS_V * 1000.0f + 0.5f);
    bool bajo_baja = anterior == ENE_OK ? mv < baja : mv < baja + h;          /* ¿del lado baja/crítica? */
    bool bajo_crit = anterior == ENE_CRITICA ? mv < crit + h : mv < crit;      /* ¿del lado crítica? */
    if (bajo_crit) return ENE_CRITICA;
    if (bajo_baja) return ENE_BAJA;
    return ENE_OK;
}

const char *ene_nombre(ene_estado_t e)
{
    return e == ENE_CRITICA ? "critica" : e == ENE_BAJA ? "baja" : "ok";
}

bool ene_reapertura(ene_reap_t *r, uint32_t t_s, bool red_presente, float v)
{
    long mv = (long)(v * 1000.0f + 0.5f);
    if (!red_presente || mv < (long)(ENE_REAPERTURA_V * 1000.0f + 0.5f)) { r->en_curso = false; return false; }
    if (!r->en_curso) { r->en_curso = true; r->desde_s = t_s; }
    return t_s - r->desde_s >= ENE_REAPERTURA_S;
}
