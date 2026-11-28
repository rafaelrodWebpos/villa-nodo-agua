/* TC-POR-A1 · §19 (interfaz portal), FR-CFG-A3 (portal), FR-BOM-03 (Q-09), FR-LOC-06 (host) */
#include <stdio.h>
#include <string.h>
#include "portal_form.h"
#include "portal_nodo.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void base(const char *v[PORTAL_N_CAMPOS])
{
    memset(v, 0, sizeof(const char *) * PORTAL_N_CAMPOS);
    v[PORTAL_NODO] = "1";
    for (int i = 0; i < 8; i++) v[PORTAL_SONDA_1 + i] = "1";
    v[PORTAL_RANGO] = "500"; v[PORTAL_ALTURA] = "200"; v[PORTAL_I_MARCHA] = "0.5"; v[PORTAL_I_SECO] = "1,0";
}

int main(void)
{
    portal_nodo_t fab, out; bool seca[8] = {0}, moj[8] = {0};
    const char *v[PORTAL_N_CAMPOS], *err[PORTAL_N_CAMPOS];
    portal_nodo_fabrica(&fab);
    OK(portal_nodo_valido(&fab) && fab.sonda_activa[7] && fab.rango_cm == 500 && fab.i_seco_a == 0, "fábrica");
    base(v); memset(err, 0, sizeof err);
    OK(portal_nodo_validar(v, &fab, seca, err, &out) == 0 && out.i_seco_a == 1.0f && out.rango_cm == 500, "válido con «1,0»");
    /* sondas 7 y 8 inactivas */
    base(v); v[PORTAL_SONDA_7] = v[PORTAL_SONDA_8] = NULL; memset(err, 0, sizeof err);
    OK(portal_nodo_validar(v, &fab, seca, err, &out) == 0 && !out.sonda_activa[6] && !out.sonda_activa[7] && out.sonda_activa[0], "7 y 8 inactivas");
    /* FR-CFG-A3: la sonda 3 mojada no se desactiva */
    base(v); v[PORTAL_SONDA_3] = NULL; moj[2] = true; memset(err, 0, sizeof err);
    portal_nodo_t antes = out;
    OK(portal_nodo_validar(v, &fab, moj, err, &out) == 1 && err[PORTAL_SONDA_3] && !memcmp(&antes, &out, sizeof out), "sonda 3 mojada desactivada");
    /* rangos */
    const struct { int campo; const char *val; } MAL[] = {
        { PORTAL_RANGO, "49" }, { PORTAL_RANGO, "1001" }, { PORTAL_RANGO, "5a" }, { PORTAL_RANGO, "" },
        { PORTAL_ALTURA, "2000" }, { PORTAL_I_MARCHA, "0.1" }, { PORTAL_I_MARCHA, "11" }, { PORTAL_I_MARCHA, "1.2.3" },
        { PORTAL_I_SECO, "0.4" }, { PORTAL_I_SECO, "21" }, { PORTAL_I_SECO, "x" },
    };
    for (size_t k = 0; k < sizeof MAL / sizeof MAL[0]; k++) {
        base(v); v[MAL[k].campo] = MAL[k].val; memset(err, 0, sizeof err);
        OK(portal_nodo_validar(v, &fab, seca, err, &out) == 1 && err[MAL[k].campo], "«%s» aceptado en el campo %d", MAL[k].val, MAL[k].campo);
    }
    /* i_seco_a ≤ i_marcha_a */
    base(v); v[PORTAL_I_MARCHA] = "1.5"; v[PORTAL_I_SECO] = "1.5"; memset(err, 0, sizeof err);
    OK(portal_nodo_validar(v, &fab, seca, err, &out) == 1 && err[PORTAL_I_SECO], "i_seco igual a i_marcha");
    /* FR-BOM-03 (Q-09): vacío solo si no estaba calibrada */
    base(v); v[PORTAL_I_SECO] = ""; memset(err, 0, sizeof err);
    OK(portal_nodo_validar(v, &fab, seca, err, &out) == 0 && out.i_seco_a == 0, "vacío sin calibrar rechazado");
    portal_nodo_t cal = fab; cal.i_seco_a = 1.0f;
    base(v); v[PORTAL_I_SECO] = NULL; memset(err, 0, sizeof err);
    OK(portal_nodo_validar(v, &cal, seca, err, &out) == 1 && err[PORTAL_I_SECO], "vaciar i_seco calibrada aceptado");
    /* sin el campo oculto el envío no trae los del nodo (quedan como están) */
    base(v); v[PORTAL_NODO] = NULL;
    OK(!portal_nodo_presente(v), "envío sin «nodo» tomado como del nodo");
    /* FR-LOC-06: el formulario muestra el valor actual */
    char txt[4][16]; const char *w[PORTAL_N_CAMPOS] = {0};
    cal.sonda_activa[4] = false;
    portal_nodo_valores(&cal, txt, w);
    OK(w[PORTAL_NODO] && w[PORTAL_SONDA_1] && !w[PORTAL_SONDA_5] && !strcmp(w[PORTAL_RANGO], "500") &&
       !strcmp(w[PORTAL_I_SECO], "1.00") && !strcmp(w[PORTAL_I_MARCHA], "0.50"), "valores actuales");
    portal_nodo_valores(&fab, txt, w);
    OK(w[PORTAL_I_SECO] == NULL, "i_seco sin calibrar sale vacío");
    /* el valor mostrado vuelve a validar igual (ida y vuelta) */
    memset(err, 0, sizeof err);
    OK(portal_nodo_validar(w, &fab, seca, err, &out) == 0 && out.rango_cm == fab.rango_cm && out.i_marcha_a == fab.i_marcha_a, "ida y vuelta");
    portal_nodo_t malo = fab; malo.i_seco_a = 0.3f;
    OK(!portal_nodo_valido(&malo), "NVS con i_seco < i_marcha aceptada");
    printf("portal_nodo: %d fallos\n", fallos);
    return fallos != 0;
}
