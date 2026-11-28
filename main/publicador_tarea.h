/* Publicador de §13 (Fase 2): estados y medidas (FR-PUB-01/02/03), eventos con la cola de
 * FR-PUB-04/05 y órdenes valvula/set, bomba/set y medidor/set (FR-ORD-01…03, FR-MED-06/10). */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "mqtt_client.h"

typedef struct {
    esp_mqtt_client_handle_t (*cliente)(void);
    bool (*operativo)(void);
    bool (*hora)(uint32_t *ts);      /* hora válida y época Unix */
} publicador_ganchos_t;

/* Arranca la tarea; `base` = «<sitio>/<zona>/<nodo>» (se copia) */
void publicador_iniciar(const publicador_ganchos_t *g, const char *base);
/* T11: suscribe las órdenes y pide FR-PUB-02 y la cola de FR-PUB-04 en ese orden */
void publicador_entrar_operativo(void);
/* MQTT_EVENT_DATA: órdenes de §13.1 */
void publicador_mensaje(const esp_mqtt_event_t *d);
/* FR-PUB-05: descartados por la cola llena */
uint32_t publicador_perdidos(void);
