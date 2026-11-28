/* L1 es — parte pura de los drivers de E/S (FSD §12). */
#include "es.h"
#include <math.h>

uint16_t es_ads_config(int canal)
{
    return (uint16_t)(0x8000u                               /* OS: iniciar una conversión */
                      | ((uint16_t)(4 + (canal & 3)) << 12) /* MUX: AINx contra GND */
                      | (1u << 9)                           /* PGA ±4,096 V */
                      | (1u << 8)                           /* modo de conversión única */
                      | (7u << 5)                           /* 860 muestras/s */
                      | 3u);                                /* comparador desactivado */
}

float es_ads_voltios(int16_t c) { return (float)c * ES_ADS_FS_V / 32768.0f; }

float es_sonda_fraccion(float v, float vref) { return v / vref; }

float es_vref(bool ads_ok, float v, bool *nominal)
{
    *nominal = !(ads_ok && v >= ES_VREF_MIN_V && v <= ES_VREF_MAX_V);
    return *nominal ? ES_VREF_SONDAS_V : v;
}
float es_nivel_ma(float v) { return v / ES_SHUNT_OHM * 1000.0f; }
float es_presion_sensor_v(float v) { return v * 1.5f; }
float es_vbat_v(float v) { return v * 6.0f; }

bool es_antirrebote(es_antirrebote_t *a, uint32_t t_ms, bool crudo)
{
    if (!a->iniciado) { a->iniciado = true; a->estable = a->candidato = crudo; a->desde_ms = t_ms; return crudo; }
    if (crudo == a->estable) { a->candidato = crudo; return a->estable; }
    if (crudo != a->candidato) { a->candidato = crudo; a->desde_ms = t_ms; }
    if (t_ms - a->desde_ms >= ES_ANTIRREBOTE_MS) a->estable = crudo;
    return a->estable;
}

void es_puente(es_motor_t m, bool *in1, bool *in2)
{
    *in1 = m == ES_MOTOR_CERRAR;
    *in2 = m == ES_MOTOR_ABRIR;
}

float es_rms_amperios(const int16_t *mv, int n)
{
    if (n <= 0) return 0.0f;
    double s = 0, s2 = 0;
    for (int i = 0; i < n; i++) s += mv[i];
    double media = s / n;
    for (int i = 0; i < n; i++) { double d = mv[i] - media; s2 += d * d; }
    return (float)(sqrt(s2 / n) / 1000.0 * ES_PINZA_A_POR_V);
}

es_boton_ev_t es_boton(es_boton_t *b, uint32_t t_ms, bool pulsado)
{
    if (pulsado && !b->pulsado) { b->pulsado = true; b->desde_ms = t_ms; return ES_BOTON_NADA; }
    if (pulsado || !b->pulsado) return ES_BOTON_NADA;
    b->pulsado = false;
    uint32_t d = t_ms - b->desde_ms;
    if (d < ES_PULSACION_CORTA_MS) return ES_BOTON_CORTA;
    if (d < ES_PULSACION_LARGA_MS) return ES_BOTON_NADA;
    return d < ES_PULSACION_PORTAL_MS ? ES_BOTON_LARGA : ES_BOTON_PORTAL;
}

bool es_onda_hz(uint32_t t_ms, uint32_t periodo_ms) { return (t_ms % periodo_ms) < periodo_ms / 2; }

bool es_onda_3_pitidos(uint32_t t_ms, uint32_t desde_ms)
{
    uint32_t f = (t_ms - desde_ms) % 600000u;     /* cada 10 min */
    return f < 1200u && (f % 400u) < 200u;        /* 200 ms sí, 200 ms no, tres veces */
}
