/* TC-IO-A1 · drivers de E/S, parte pura (FSD §12; C-HW-22, C-HW-24…28): ADS1115, cadenas de
 * conversión, antirrebote (FR-IO-05), puente H (FR-VAL-11), RMS de la pinza (FR-IO-04),
 * botón (FR-LOC-02/03) y pitidos (§8.2). */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "bomba.h"
#include "es.h"
#include "fugas.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    /* ADS1115: conversión única, AINx–GND, ±4,096 V, 860 SPS, comparador desactivado */
    const uint16_t CFG[4] = { 0xC3E3, 0xD3E3, 0xE3E3, 0xF3E3 };
    for (int c = 0; c < 4; c++) OK(es_ads_config(c) == CFG[c], "config canal %d: 0x%04x", c, es_ads_config(c));
    OK(fabsf(es_ads_voltios(16000) - 2.0f) < 1e-4f && fabsf(es_ads_voltios(-32768) + 4.096f) < 1e-4f, "cuentas → V");

    /* Sondas (C-HW-26, P4): lazo R contra 10 kΩ a 3,3 V → clase; el contrato de banco de FR-SON-01 */
    const struct { float r; son_clase_t c; } L[] = { {10000, SON_SECA}, {3000, SON_MOJADA}, {1e12f, SON_ABIERTA}, {0, SON_CORTO} };
    for (int i = 0; i < 4; i++) {
        float v = ES_VREF_SONDAS_V * L[i].r / (L[i].r + 10000.0f);
        OK(son_clasificar(es_sonda_fraccion(v, ES_VREF_SONDAS_V)) == L[i].c, "lazo %.0f Ω → %s", L[i].r, son_nombre(son_clasificar(es_sonda_fraccion(v, ES_VREF_SONDAS_V))));
    }
    /* Referencia de los lazos (FSD 0.6.0): A3 válida entre 3,0 y 3,6 V; si no, 3,3 V nominales */
    bool nom;
    OK(es_vref(true, 3.1f, &nom) == 3.1f && !nom, "referencia medida");
    OK(es_vref(true, 2.9f, &nom) == ES_VREF_SONDAS_V && nom, "A3 baja");
    OK(es_vref(true, 3.7f, &nom) == ES_VREF_SONDAS_V && nom, "A3 alta");
    OK(es_vref(false, 3.1f, &nom) == ES_VREF_SONDAS_V && nom, "0x4A caído");
    /* Nivel (C-HW-27): 12 mA sobre 150 Ω = 1,8 V → 250 cm de 500 */
    float cm, pct;
    OK(fabsf(es_nivel_ma(1.8f) - 12.0f) < 1e-3f && bom_nivel(es_nivel_ma(1.8f), 500, 400, &cm, &pct) && fabsf(cm - 250) < 0.1f, "12 mA");
    /* Presión (C-HW-28): 2,5 V del sensor con divisor 2:3 = 1,667 V → 5 bar */
    float bar;
    OK(bom_presion(es_presion_sensor_v(2.5f * 2 / 3), &bar) && fabsf(bar - 5) < 0.01f, "presión %.3f", bar);
    OK(fabsf(es_vbat_v(2.1f) - 12.6f) < 1e-3f, "Vbat 1:6");
    OK(es_ads_voltios(32767) > 20.5f * ES_SHUNT_OHM / 1000 && es_ads_voltios(32767) > 4.7f * 2 / 3 &&
       es_ads_voltios(32767) > 14.6f / 6 && es_ads_voltios(32767) > ES_VREF_SONDAS_V, "el fondo de escala cubre todos los canales");

    /* Antirrebote (FR-IO-05): rebotes de 30 ms no cambian; 50 ms estables sí */
    es_antirrebote_t a = {0};
    es_antirrebote(&a, 0, false);
    int cambios = 0; bool prev = false;
    for (uint32_t t = 10; t < 1000; t += 10) { bool v = es_antirrebote(&a, t, (t / 30) % 2); cambios += v != prev; prev = v; }
    OK(cambios == 0, "rebotes de 30 ms: %d cambios", cambios);
    memset(&a, 0, sizeof a); es_antirrebote(&a, 0, false);
    uint32_t cuando = 0;
    for (uint32_t t = 10; t <= 200 && !cuando; t += 10) if (es_antirrebote(&a, t, t >= 100)) cuando = t;
    OK(cuando == 150, "cambio estable a los %u ms (esperado 100 + 50)", cuando);

    /* Puente H: nunca IN1 e IN2 a la vez */
    for (int m = ES_MOTOR_PARADO; m <= ES_MOTOR_ABRIR; m++) {
        bool i1, i2; es_puente((es_motor_t)m, &i1, &i2);
        OK(!(i1 && i2) && i1 == (m == ES_MOTOR_CERRAR) && i2 == (m == ES_MOTOR_ABRIR), "puente %d: %d %d", m, i1, i2);
    }

    /* Pinza (FR-IO-04): senoide de 60 Hz, 0,5 V RMS sobre 1650 mV de polarización → 15 A ± 1,5 */
    int16_t mv[ES_RMS_MUESTRAS];
    const double fs = 1e6 / ES_RMS_PERIODO_US;
    for (int i = 0; i < ES_RMS_MUESTRAS; i++) mv[i] = (int16_t)lround(1650 + 500 * sqrt(2) * sin(2 * M_PI * 60 * i / fs));
    float ia = es_rms_amperios(mv, ES_RMS_MUESTRAS);
    OK(fabsf(ia - 15.0f) <= 1.5f, "RMS %.2f A (esperado 15 ± 1,5)", ia);
    OK(fs >= 2000 && ES_RMS_MUESTRAS / fs >= 10.0 / 60 - 1e-3, "≥ 2000 muestras/s durante 10 ciclos (%.0f/s, %.1f ms)", fs, 1000 * ES_RMS_MUESTRAS / fs);
    for (int i = 0; i < ES_RMS_MUESTRAS; i++) mv[i] = 1650;
    OK(es_rms_amperios(mv, ES_RMS_MUESTRAS) < 0.01f, "sin corriente");

    /* Botón: 0,5 s corta; 2,5 s nada; 3,5 s larga a los 3 s y nada al soltar */
    const struct { uint32_t ms; es_boton_ev_t ev; uint32_t a_los; } B[] = { {500, ES_BOTON_CORTA, 500}, {2500, ES_BOTON_NADA, 0}, {3500, ES_BOTON_LARGA, 3000} };
    for (int k = 0; k < 3; k++) {
        es_boton_t b = {0}; es_boton_ev_t visto = ES_BOTON_NADA; uint32_t t_ev = 0; int n = 0;
        for (uint32_t t = 0; t <= B[k].ms + 500; t += 10) {
            es_boton_ev_t ev = es_boton(&b, t, t < B[k].ms);
            if (ev != ES_BOTON_NADA) { visto = ev; t_ev = t; n++; }
        }
        OK(visto == B[k].ev && n <= 1 && t_ev == B[k].a_los, "pulsación de %u ms: evento %d a los %u ms (%d eventos)", B[k].ms, visto, t_ev, n);
    }

    /* Pitidos: 3 por cada 10 min; 1 Hz: 50 % */
    int flancos = 0; bool z0 = false;
    for (uint32_t t = 0; t < 600000; t += 10) { bool z = es_onda_3_pitidos(t, 0); flancos += z && !z0; z0 = z; }
    OK(flancos == 3, "pitidos en 10 min: %d", flancos);
    OK(es_onda_hz(0, 1000) && !es_onda_hz(500, 1000) && es_onda_hz(1000, 1000), "1 Hz");

    printf("es: %d fallos\n", fallos);
    return fallos != 0;
}
