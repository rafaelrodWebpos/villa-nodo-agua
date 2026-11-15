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
