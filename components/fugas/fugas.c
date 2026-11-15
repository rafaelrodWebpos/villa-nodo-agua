/* L2 fugas — FR-FUG-01. Núcleo puro. */
#include "fugas.h"

bool fug_muestra(fug_sonda_t *s, bool mojada)
{
    if (!mojada) { s->seguidas = 0; return false; }
    if (s->seguidas < FUG_MUESTRAS_CONFIRMAR) s->seguidas++;
    return s->seguidas >= FUG_MUESTRAS_CONFIRMAR;
}

son_clase_t son_clasificar(float x)
{
    if (x <= 0.05f) return SON_CORTO;
    if (x <= 0.40f) return SON_MOJADA;
    if (x <= 0.60f) return SON_SECA;
    if (x <= 0.90f) return SON_INDETERMINADA;
    return SON_ABIERTA;
}

const char *son_nombre(son_clase_t c)
{
    static const char *const N[] = { "corto", "mojada", "seca", "indeterminada", "abierta" };
    return (unsigned)c < 5 ? N[c] : "?";
}
