/* TC-CON-A1 · FR-CON-A1/A2 (FSD 0.7.1, P24/P25) en host, con reloj simulado:
 *   60 s asociado sin IP → wifi_down (no a los 59 s); 10 min sin broker con IP → una
 *   reasociación, como mucho una por 10 min; la cuenta se reinicia con IP, broker o Wi-Fi caído. */
#include <stdio.h>
#include "conectividad_vigia.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* primer instante (ms, paso de 1 s) en [desde, hasta] en que el tick devuelve `acc`; 0 si nunca */
static uint32_t primera(con_vigia_t *v, uint32_t desde, uint32_t hasta, con_vigia_accion_t acc)
{
    for (uint32_t t = desde; t <= hasta; t += 1000) if (con_vigia_tick(v, t) == acc) return t;
    return 0;
}

int main(void)
{
    con_vigia_t v;
    /* FR-CON-A1: 59 s sin nada, 60 s → SIN_IP una sola vez */
    con_vigia_iniciar(&v);
    con_vigia_asociado(&v, 1000);
    OK(primera(&v, 1000, 60000, CON_VIG_SIN_IP) == 0, "sin IP antes de 60 s");
    OK(con_vigia_tick(&v, 61000) == CON_VIG_SIN_IP, "sin IP a los 60 s");
    OK(primera(&v, 61000, 300000, CON_VIG_SIN_IP) == 0, "sin IP repetido sin nueva asociación");
    /* con IP a tiempo, nada */
    con_vigia_iniciar(&v);
    con_vigia_asociado(&v, 0); con_vigia_con_ip(&v);
    OK(primera(&v, 0, 200000, CON_VIG_SIN_IP) == 0, "sin IP con IP obtenida");
    /* desasociado antes de 60 s: la cuenta se anula; una asociación nueva empieza de cero */
    con_vigia_iniciar(&v);
    con_vigia_asociado(&v, 0); con_vigia_desasociado(&v);
    con_vigia_asociado(&v, 30000);
    OK(con_vigia_tick(&v, 89000) == CON_VIG_NADA && con_vigia_tick(&v, 90000) == CON_VIG_SIN_IP, "cuenta desde la nueva asociación");

    /* FR-CON-A2: 10 min sin broker → REASOCIAR a los 600 s, no antes */
    con_vigia_iniciar(&v);
    con_vigia_sin_broker(&v, 0);
    con_vigia_sin_broker(&v, 120000);                  /* T12 repetidos no reinician la cuenta */
    OK(primera(&v, 0, 599000, CON_VIG_REASOCIAR) == 0, "reasociación antes de 10 min");
    OK(con_vigia_tick(&v, 600000) == CON_VIG_REASOCIAR, "reasociación a los 10 min");
    OK(primera(&v, 600000, 1800000, CON_VIG_REASOCIAR) == 0, "otra reasociación sin volver a CONECTANDO_BROKER");
    /* tras reasociar, el siguiente T08 (≥ 600 s) vuelve a contar: como mucho una por 10 min */
    con_vigia_sin_broker(&v, 620000);
    uint32_t t2 = primera(&v, 620000, 2000000, CON_VIG_REASOCIAR);
    OK(t2 == 1220000, "segunda reasociación a %u ms (esperado 1220000)", t2);
    /* el broker vuelve: la cuenta se anula */
    con_vigia_iniciar(&v);
    con_vigia_sin_broker(&v, 0); con_vigia_broker_ok(&v);
    OK(primera(&v, 0, 1200000, CON_VIG_REASOCIAR) == 0, "reasociación con el broker de vuelta");
    /* el Wi-Fi cae: la cuenta se anula (FR-CON-A2 pide el Wi-Fi asociado) */
    con_vigia_iniciar(&v);
    con_vigia_sin_broker(&v, 0); con_vigia_desasociado(&v);
    OK(primera(&v, 0, 1200000, CON_VIG_REASOCIAR) == 0, "reasociación sin Wi-Fi");
    /* reloj que desborda (uint32_t ms ≈ 49,7 días) */
    con_vigia_iniciar(&v);
    con_vigia_sin_broker(&v, 0xFFFFFFFFu - 1000u);
    OK(con_vigia_tick(&v, 598999u) == CON_VIG_REASOCIAR || con_vigia_tick(&v, 599000u) == CON_VIG_REASOCIAR,
       "desborde del reloj");

    printf("vigia: %d fallos\n", fallos);
    /* FSD 0.8.2 (P24): IP_LOST solo suelta la asociación en los estados con IP. Sin haber
     * tenido IP (ESP-IDF lo avisa 120 s después de cada desconexión) no la suelta ni toca la
     * cuenta de «asociado sin IP» (TC-CON-06, 2026-11-22). */
    for (int e = 0; e < CON_N_ESTADOS; e++) {
        con_vigia_iniciar(&v);
        con_vigia_asociado(&v, 0);
        bool suelta = con_vigia_ip_perdida(&v, (con_estado_t)e);
        bool con_ip = e == CON_CONECTANDO_BROKER || e == CON_OPERATIVO;
        OK(suelta == con_ip, "IP_LOST en el estado %d: suelta=%d", e, suelta);
        if (!con_ip) OK(con_vigia_tick(&v, 60000) == CON_VIG_SIN_IP, "IP_LOST sin IP anuló la cuenta de 60 s (estado %d)", e);
    }
    con_vigia_iniciar(&v);
    con_vigia_sin_broker(&v, 0);
    OK(con_vigia_ip_perdida(&v, CON_CONECTANDO_BROKER), "IP_LOST en CONECTANDO_BROKER no suelta");
    OK(primera(&v, 0, 1200000, CON_VIG_REASOCIAR) == 0, "IP_LOST con IP anula la cuenta sin broker (desasociado)");
    return fallos != 0;
}
