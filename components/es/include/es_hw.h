/* L1 es — parte con hardware de los drivers de E/S (ESP-IDF). Solo la usa el firmware. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "es.h"

/* Salidas a 0 (relé sin energía, motor parado) y entradas con sus pull-ups.
 * Es lo primero de app_main (FR-BOM-02: GPIO14 en bajo hasta restaurar la NVS). */
void es_hw_gpio_iniciar(void);
/* Bus I²C (SDA 8, SCL 9, 100 kHz) y los 3 ADS1115; escribe en ack la lista de
 * direcciones que responden («0x48,0x49,0x4a»), para la línea A7. */
void es_hw_i2c_iniciar(char *ack, int cap);
/* Una conversión del canal (0…3) del ADS k (0…2); false si el ADS no responde. */
bool es_hw_ads_leer(int k, int canal, int16_t *cuentas);
/* ADC1 de la pinza (GPIO4, ADC1_CH3) */
bool es_hw_pinza_iniciar(void);
/* n muestras en mV a ~2400 muestras/s (FR-IO-04) */
bool es_hw_pinza_muestras(int16_t *mv, int n);

typedef struct { bool fin_abierta, fin_cerrada, flotador_agua, red_presente, boton, medidor; } es_hw_digitales_t;
es_hw_digitales_t es_hw_digitales(void);          /* con la polaridad ya resuelta: true = activo */
/* Escribe las salidas; nunca IN1 e IN2 a la vez (FR-VAL-11): si llegan los dos, los dos a 0 */
void es_hw_salidas(bool in1, bool in2, bool rele, bool led_verde, bool led_rojo, bool zumbador);
