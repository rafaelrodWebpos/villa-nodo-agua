/* L1 es — drivers de E/S del nodo (FSD §2.2 C-HW-22…30, §12). Esta cabecera es la parte
 * pura (sin ESP-IDF; la compila tests/host): codificación del ADS1115, conversiones de
 * los canales, antirrebote, puente H, RMS de la pinza, pulsaciones del botón y formas de
 * onda del LED y del zumbador. La parte con hardware está en es_hw.h. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* ---- Pines (C-HW-30, P1) ---- */
#define ES_GPIO_SDA          8
#define ES_GPIO_SCL          9
#define ES_GPIO_PINZA        4   /* ADC1_CH3 */
#define ES_GPIO_RED          5   /* alto = red presente */
#define ES_GPIO_FIN_ABIERTA  6   /* contacto a GND con pull-up: bajo = activo */
#define ES_GPIO_FIN_CERRADA  7
#define ES_GPIO_FLOTADOR    10   /* contacto a GND con pull-up: bajo = hay agua (cable cortado = sin agua) */
#define ES_GPIO_BOTON       11   /* a GND con pull-up: bajo = pulsado */
#define ES_GPIO_IN1         12   /* puente H: cerrar */
#define ES_GPIO_IN2         13   /* puente H: abrir */
#define ES_GPIO_RELE        14   /* alto = relé energizado = permiso cortado */
#define ES_GPIO_ZUMBADOR    15
#define ES_GPIO_LED_VERDE   16
#define ES_GPIO_LED_ROJO    17

/* ---- ADS1115 (C-HW-24, P1) ---- */
#define ES_I2C_HZ        100000
#define ES_N_ADS         3
#define ES_ADS_DIRS      { 0x48, 0x49, 0x4A }   /* sondas 1–4, 5–8, analógicas (A0 nivel, A1 presión, A2 Vbat) */
#define ES_ADS_FS_V      4.096f   /* PGA ±4,096 V: cubre 3,3 V de las sondas y ≤ 3,0 V del resto */

/* Palabra del registro de configuración: conversión única del canal AINch contra GND,
 * PGA ±4,096 V, 860 muestras/s, comparador desactivado. */
uint16_t es_ads_config(int canal);
/* Cuentas de la conversión → voltios */
float es_ads_voltios(int16_t cuentas);

/* ---- Conversiones de los canales (P4, P5, P9, P18) ---- */
#define ES_VREF_SONDAS_V 3.3f     /* referencia nominal de los lazos (FSD 0.6.0, §2.2) */
#define ES_VREF_MIN_V    3.0f     /* A3 del 0x4A fuera de 3,0–3,6 V: se usa la nominal */
#define ES_VREF_MAX_V    3.6f
#define ES_SHUNT_OHM     150.0f   /* C-HW-27 */
float es_sonda_fraccion(float v, float vref);   /* V del lazo / referencia (P4) */
/* Referencia medida en A3 del 0x4A si es válida (ADS bien y 3,0–3,6 V); si no, la nominal.
 * *nominal = true cuando se usa la nominal. */
float es_vref(bool ads_ok, float v_a3, bool *nominal);
float es_nivel_ma(float v);           /* V sobre el shunt de 150 Ω → mA */
float es_presion_sensor_v(float v);   /* divisor 2:3 → tensión del sensor */
float es_vbat_v(float v);             /* divisor 1:6 → tensión de la batería */

/* ---- Antirrebote de 50 ms (FR-IO-05) ---- */
#define ES_ANTIRREBOTE_MS 50u
typedef struct { bool estable, candidato, iniciado; uint32_t desde_ms; } es_antirrebote_t;
/* Devuelve el valor estable: cambia cuando el crudo lleva ≥ 50 ms con el valor nuevo.
 * La primera llamada fija el valor sin esperar. */
bool es_antirrebote(es_antirrebote_t *a, uint32_t t_ms, bool crudo);

/* ---- Puente H DRV8871 (C-HW-22, FR-VAL-11) ---- */
typedef enum { ES_MOTOR_PARADO, ES_MOTOR_CERRAR, ES_MOTOR_ABRIR } es_motor_t;
/* IN1/IN2 para cada sentido; nunca los dos a 1 */
void es_puente(es_motor_t m, bool *in1, bool *in2);

/* ---- Pinza SCT-013-030 (C-HW-25, P10, FR-IO-04): 30 A → 1 V RMS ---- */
#define ES_PINZA_A_POR_V   30.0f
#define ES_RMS_MUESTRAS    400     /* 10 ciclos de 60 Hz (166,7 ms) a 2400 muestras/s */
#define ES_RMS_PERIODO_US  417
/* RMS de la componente alterna (se resta la media, que es la polarización) → amperios */
float es_rms_amperios(const int16_t *mv, int n);

/* ---- Botón (FR-LOC-02/03/06) ---- */
#define ES_PULSACION_CORTA_MS  1000u
#define ES_PULSACION_LARGA_MS  3000u
#define ES_PULSACION_PORTAL_MS 10000u
typedef enum { ES_BOTON_NADA, ES_BOTON_CORTA, ES_BOTON_LARGA, ES_BOTON_PORTAL } es_boton_ev_t;
typedef struct { bool pulsado; uint32_t desde_ms; } es_boton_t;
/* Con el valor ya filtrado, todo se evalúa al soltar (FSD 0.8.6–0.8.7): < 1 s corta;
 * ≥ 3 s y < 10 s larga (FR-LOC-03); ≥ 10 s vuelta al portal (FR-LOC-06); entre 1 y 3 s nada. */
es_boton_ev_t es_boton(es_boton_t *b, uint32_t t_ms, bool pulsado);

/* ---- Formas de onda (§8.2, P17) ---- */
bool es_onda_hz(uint32_t t_ms, uint32_t periodo_ms);       /* mitad encendido, mitad apagado */
bool es_onda_3_pitidos(uint32_t t_ms, uint32_t desde_ms);  /* 3 × 200 ms al empezar cada 10 min */
