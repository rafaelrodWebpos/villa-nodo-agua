/* L2 energia — núcleo puro de energía y batería (FSD §7). Sin ESP-IDF; lo compila tests/host. */
#pragma once

/* P5 (LiFePO4 4S): «baja» < 12,4 V, «crítica» < 12,0 V, histéresis 0,2 V */
#define ENE_BAJA_V      12.4f
#define ENE_CRITICA_V   12.0f
#define ENE_HISTERESIS_V 0.2f

typedef enum { ENE_OK, ENE_BAJA, ENE_CRITICA } ene_estado_t;

/* FR-ENE-02: estado nuevo a partir del anterior y de la tensión. Al bajar cambia por
 * debajo de 12,4 / 12,0 V; al subir, desde 12,6 / 12,2 V (umbral + histéresis). */
ene_estado_t ene_clasificar(ene_estado_t anterior, float v);

/* «ok», «baja» o «critica» (§13.1, tópico bateria) */
const char *ene_nombre(ene_estado_t e);

#include <stdbool.h>
#include <stdint.h>

/* FR-ENE-04 (P16): condición de cierre preventivo (evento de nivel VEV_BATERIA_CIERRE) */
static inline bool ene_cierre(ene_estado_t e, bool red_presente) { return e == ENE_CRITICA && !red_presente; }

/* FR-ENE-05 (P16): red presente durante 5 min sin interrupción y Vbat ≥ 12,8 V. Devuelve
 * true mientras se cumple (evento de nivel VEV_BATERIA_REAPERTURA); el bloqueo de fuga es
 * guarda de V23. */
#define ENE_REAPERTURA_V 12.8f
#define ENE_REAPERTURA_S 300u
typedef struct { bool en_curso; uint32_t desde_s; } ene_reap_t;
bool ene_reapertura(ene_reap_t *r, uint32_t t_s, bool red_presente, float v);
