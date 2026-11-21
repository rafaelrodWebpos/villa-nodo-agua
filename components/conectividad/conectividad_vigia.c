/* L2 conectividad — FR-CON-A1/A2 (FSD 0.7.1). Núcleo puro. */
#include "conectividad_vigia.h"

void con_vigia_iniciar(con_vigia_t *v) { *v = (con_vigia_t){0}; }

void con_vigia_asociado(con_vigia_t *v, uint32_t t_ms)
{
    v->esperando_ip = true;
    v->desde_asociado = t_ms;
}

void con_vigia_con_ip(con_vigia_t *v) { v->esperando_ip = false; }

void con_vigia_desasociado(con_vigia_t *v)
{
    v->esperando_ip = false;
    v->buscando_broker = false;      /* sin Wi-Fi, la cuenta de FR-CON-A2 no corre */
}

void con_vigia_sin_broker(con_vigia_t *v, uint32_t t_ms)
{
    if (v->buscando_broker) return;
    v->buscando_broker = true;
    v->desde_sin_broker = t_ms;
}

void con_vigia_broker_ok(con_vigia_t *v) { v->buscando_broker = false; }

con_vigia_accion_t con_vigia_tick(con_vigia_t *v, uint32_t t_ms)
{
    if (v->esperando_ip && t_ms - v->desde_asociado >= CON_SIN_IP_MS) {
        v->esperando_ip = false;
        return CON_VIG_SIN_IP;
    }
    if (v->buscando_broker && t_ms - v->desde_sin_broker >= CON_SIN_BROKER_MS) {
        /* Se consume: la cuenta vuelve a empezar en el siguiente T08 tras reasociar, así que
         * la siguiente reasociación llega como pronto 10 min después (P25). */
        v->buscando_broker = false;
        return CON_VIG_REASOCIAR;
    }
    return CON_VIG_NADA;
}
