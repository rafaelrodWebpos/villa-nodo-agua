/* L2 interfaz — FR-LOC-04 (P17). Núcleo puro. */
#include "interfaz.h"

loc_patron_t loc_patron(const loc_entrada_t *e)
{
    if (e->fuga_confirmada) return (loc_patron_t){ 1, LED_ROJO_FIJO, ZUM_1HZ };
    if (e->valvula_falla || e->bomba_bloqueada || e->sonda_falla || e->bateria_critica)
        return (loc_patron_t){ 2, LED_ROJO_1HZ, ZUM_3_PITIDOS_10MIN };
    if (e->valvula_cerrada || e->bomba_cortada) return (loc_patron_t){ 3, LED_ROJO_025HZ, ZUM_NINGUNO };
    if (e->sin_red_o_broker) return (loc_patron_t){ 4, LED_VERDE_1HZ, ZUM_NINGUNO };
    return (loc_patron_t){ 5, LED_VERDE_FIJO, ZUM_NINGUNO };
}
