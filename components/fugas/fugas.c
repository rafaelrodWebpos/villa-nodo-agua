/* L2 fugas — FR-FUG-01. Núcleo puro. */
#include "fugas.h"

bool fug_muestra(fug_sonda_t *s, bool mojada)
{
    if (!mojada) { s->seguidas = 0; return false; }
    if (s->seguidas < FUG_MUESTRAS_CONFIRMAR) s->seguidas++;
    return s->seguidas >= FUG_MUESTRAS_CONFIRMAR;
}
