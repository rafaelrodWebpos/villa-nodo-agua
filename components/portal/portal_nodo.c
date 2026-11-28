/* L1 portal — parámetros del nodo (§19). Núcleo puro. */
#include "portal_nodo.h"
#include "portal_textos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void portal_nodo_fabrica(portal_nodo_t *p)
{
    for (int i = 0; i < 8; i++) p->sonda_activa[i] = true;
    p->rango_cm = 500; p->altura_cm = 200; p->i_marcha_a = 0.5f; p->i_seco_a = 0.0f;
}

bool portal_nodo_presente(const char *const v[PORTAL_N_CAMPOS]) { return v[PORTAL_NODO] != NULL; }

static bool entero(const char *s, int lo, int hi, int *out)
{
    if (!s || !*s || strlen(s) > 6) return false;
    for (const char *q = s; *q; q++) if (*q < '0' || *q > '9') return false;
    int x = atoi(s);
    if (x < lo || x > hi) return false;
    *out = x;
    return true;
}

static bool decimal(const char *s, float lo, float hi, float *out)
{
    if (!s || !*s || strlen(s) > 8) return false;
    char b[12]; size_t k = 0; int puntos = 0;
    for (const char *q = s; *q; q++) {                        /* acepta «,» o «.» como separador */
        char c = *q == ',' ? '.' : *q;
        if (c == '.') { if (++puntos > 1) return false; }
        else if (c < '0' || c > '9') return false;
        b[k++] = c;
    }
    b[k] = 0;
    if (b[0] == '.' || b[k - 1] == '.') return false;
    float x = strtof(b, NULL);
    if (x < lo || x > hi) return false;
    *out = x;
    return true;
}

int portal_nodo_validar(const char *const v[PORTAL_N_CAMPOS], const portal_nodo_t *actual, const bool mojada[8],
                        const char *error[PORTAL_N_CAMPOS], portal_nodo_t *out)
{
    portal_nodo_t r = *actual;
    int n = 0;
    for (int i = 0; i < 8; i++) {
        r.sonda_activa[i] = v[PORTAL_SONDA_1 + i] != NULL;
        if (!r.sonda_activa[i] && mojada[i]) { error[PORTAL_SONDA_1 + i] = TX_ERR_SONDA_MOJADA; n++; }   /* FR-CFG-A3 */
    }
    if (!entero(v[PORTAL_RANGO], 50, 1000, &r.rango_cm)) { error[PORTAL_RANGO] = TX_ERR_RANGO_CM; n++; }
    if (!entero(v[PORTAL_ALTURA], 50, 1000, &r.altura_cm)) { error[PORTAL_ALTURA] = TX_ERR_RANGO_CM; n++; }
    bool marcha_ok = decimal(v[PORTAL_I_MARCHA], 0.2f, 10.0f, &r.i_marcha_a);
    if (!marcha_ok) { error[PORTAL_I_MARCHA] = TX_ERR_I_MARCHA; n++; }
    const char *s = v[PORTAL_I_SECO];
    if (!s || !*s) {                                                           /* FR-BOM-03 (Q-09) */
        if (actual->i_seco_a > 0.0f) { error[PORTAL_I_SECO] = TX_ERR_I_SECO_VACIO; n++; }
        else r.i_seco_a = 0.0f;
    } else if (!decimal(s, 0.2f, 20.0f, &r.i_seco_a) || (marcha_ok && r.i_seco_a <= r.i_marcha_a)) {
        error[PORTAL_I_SECO] = TX_ERR_I_SECO; n++;
    }
    if (!n) *out = r;
    return n;
}

void portal_nodo_valores(const portal_nodo_t *p, char txt[4][16], const char *v[PORTAL_N_CAMPOS])
{
    v[PORTAL_NODO] = "1";
    for (int i = 0; i < 8; i++) v[PORTAL_SONDA_1 + i] = p->sonda_activa[i] ? "1" : NULL;
    snprintf(txt[0], 16, "%d", p->rango_cm);
    snprintf(txt[1], 16, "%d", p->altura_cm);
    snprintf(txt[2], 16, "%.2f", (double)p->i_marcha_a);
    if (p->i_seco_a > 0.0f) snprintf(txt[3], 16, "%.2f", (double)p->i_seco_a); else txt[3][0] = 0;
    v[PORTAL_RANGO] = txt[0]; v[PORTAL_ALTURA] = txt[1]; v[PORTAL_I_MARCHA] = txt[2];
    v[PORTAL_I_SECO] = txt[3][0] ? txt[3] : NULL;
}

bool portal_nodo_valido(const portal_nodo_t *p)
{
    if (p->rango_cm < 50 || p->rango_cm > 1000 || p->altura_cm < 50 || p->altura_cm > 1000) return false;
    if (!(p->i_marcha_a >= 0.2f && p->i_marcha_a <= 10.0f)) return false;
    if (p->i_seco_a > 0.0f && !(p->i_seco_a >= 0.2f && p->i_seco_a <= 20.0f && p->i_seco_a > p->i_marcha_a)) return false;
    return true;
}
