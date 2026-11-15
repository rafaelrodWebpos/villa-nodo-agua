/* L2 interfaz — patrones del LED y del zumbador (FSD §8.2, P17; FR-LOC-04). Núcleo puro. */
#pragma once
#include <stdbool.h>

typedef struct {
    bool fuga_confirmada;
    bool valvula_falla;
    bool bomba_bloqueada;
    bool sonda_falla;
    bool bateria_critica;
    bool valvula_cerrada;
    bool bomba_cortada;
    bool sin_red_o_broker;
} loc_entrada_t;

typedef enum { LED_ROJO_FIJO, LED_ROJO_1HZ, LED_ROJO_025HZ, LED_VERDE_1HZ, LED_VERDE_FIJO } loc_led_t;
typedef enum { ZUM_NINGUNO, ZUM_1HZ, ZUM_3_PITIDOS_10MIN } loc_zumbador_t;

typedef struct {
    int prioridad;             /* 1…5 de §8.2 */
    loc_led_t led;
    loc_zumbador_t zumbador;
} loc_patron_t;

/* FR-LOC-04: el patrón del estado de mayor prioridad (1 = la más alta) */
loc_patron_t loc_patron(const loc_entrada_t *e);
