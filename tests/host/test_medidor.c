/* TC-MED-A9 · FR-MED-06 (periodo permitido) y FR-MED-10 (evento consumo_permitido, Q-11/S-07) */
#include <stdio.h>
#include "medidor.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    const int M = 15;                                   /* consumo_margen_min */
    med_periodo_t p = {0}; int h = -1;
    /* contrato de FR-MED-10: 30 min; a los 5 min, 60 min; a los 6 min, 10 min */
    OK(med_permitir(&p, 0, 30, M, &h) == MED_EVENTO && h == 30 + M, "30 min: hasta_min %d", h);
    h = -1;
    OK(med_permitir(&p, 300, 60, M, &h) == MED_EVENTO && h == 60 + M, "60 min a los 5 min: hasta_min %d", h);
    h = -1;
    OK(med_permitir(&p, 360, 10, M, &h) == MED_SIN_CAMBIO && h == -1, "10 min a los 6 min no mueve el fin y no da evento");
    OK(p.fin_s == 300 + (60 + M) * 60u, "el periodo se acortó: fin %u", (unsigned)p.fin_s);
    /* inválidas: periodo sin cambio (FR-MED-06, TC-MED-A7) */
    med_periodo_t antes = p;
    OK(med_permitir(&p, 400, 0, M, &h) == MED_RECHAZADA && med_permitir(&p, 400, 721, M, &h) == MED_RECHAZADA &&
       med_permitir(&p, 400, -5, M, &h) == MED_RECHAZADA && p.fin_s == antes.fin_s, "orden inválida aceptada o con efecto");
    OK(med_permitir(&p, 400, 720, M, &h) == MED_EVENTO, "720 min rechazado");
    /* dentro / fuera, con el margen */
    med_periodo_t q = {0};
    OK(!med_dentro(&q, 0), "dentro sin periodo");
    med_permitir(&q, 1000, 30, M, &h);
    OK(med_dentro(&q, 1000) && med_dentro(&q, 1000 + (30 + M) * 60 - 1) && !med_dentro(&q, 1000 + (30 + M) * 60),
       "límites del periodo con margen");
    /* vencido: una orden nueva abre otro periodo y da evento aunque su fin sea menor que el viejo */
    h = -1;
    OK(med_permitir(&q, 1000 + (30 + M) * 60 + 10, 5, M, &h) == MED_EVENTO && h == 5 + M, "periodo vencido: sin evento al reabrir");
    /* reinicio: el periodo vive en RAM */
    med_periodo_t r = {0};
    OK(!med_dentro(&r, 50), "periodo tras reinicio");
    printf("medidor: %d fallos\n", fallos);
    return fallos != 0;
}
