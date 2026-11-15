/* L1 identidad — id del nodo desde la MAC STA (FSD §16, FR-ID-01). Núcleo puro. */
#pragma once
#include <stdint.h>

/* «agua-» + 6 hex en minúsculas + NUL */
#define IDENTIDAD_LEN 13

/* Escribe en out el id del nodo: «agua-» seguido de los 3 últimos bytes de
 * mac en hexadecimal en minúsculas (p. ej., agua-a1b2c3). */
void identidad_desde_mac(const uint8_t mac[6], char out[IDENTIDAD_LEN]);
