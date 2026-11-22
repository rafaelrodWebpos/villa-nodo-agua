/* L2 conectividad — vigilancia de los cortes parciales (FSD 0.7.1, H-93/H-95):
 *   FR-CON-A1 (P24): asociado al AP sin IP durante 60 s → se trata como wifi_down.
 *   FR-CON-A2 (P25): Wi-Fi con IP y broker inalcanzable durante 10 min → una reasociación
 *                    del Wi-Fi, sin reiniciar, como mucho una por cada 10 min.
 * Núcleo puro con reloj explícito (ms monótonos); el adaptador le pasa los eventos Wi-Fi,
 * IP y MQTT y lo consulta una vez por segundo. IP_LOST lo trata el adaptador directamente
 * (desasociar → wifi_down). */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#define CON_SIN_IP_MS      60000u     /* P24 */
#define CON_SIN_BROKER_MS  600000u    /* P25 */

typedef struct {
    bool esperando_ip;        /* asociado y sin IP */
    uint32_t desde_asociado;
    bool buscando_broker;     /* con IP y sin broker */
    uint32_t desde_sin_broker;
} con_vigia_t;

typedef enum { CON_VIG_NADA, CON_VIG_SIN_IP, CON_VIG_REASOCIAR } con_vigia_accion_t;

void con_vigia_iniciar(con_vigia_t *v);
void con_vigia_asociado(con_vigia_t *v, uint32_t t_ms);      /* WIFI_EVENT_STA_CONNECTED */
void con_vigia_con_ip(con_vigia_t *v);                       /* IP_EVENT_STA_GOT_IP */
void con_vigia_desasociado(con_vigia_t *v);                  /* STA_DISCONNECTED o IP_LOST */
/* Se entra (o se sigue) en CONECTANDO_BROKER: arranca la cuenta solo si no estaba en marcha */
void con_vigia_sin_broker(con_vigia_t *v, uint32_t t_ms);
void con_vigia_broker_ok(con_vigia_t *v);                    /* T11 */
/* Una consulta: devuelve la acción que toca y la consume (una sola vez por cuenta) */
con_vigia_accion_t con_vigia_tick(con_vigia_t *v, uint32_t t_ms);
