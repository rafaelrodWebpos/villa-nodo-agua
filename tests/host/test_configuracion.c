/* TC-CFG-A1 · FR-CFG-A1, FR-CFG-A2, FR-CFG-A3 (config/set), FR-CFG-A4 (host) */
#include <stdio.h>
#include <string.h>
#include "configuracion.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

int main(void)
{
    cfg_t c; cfg_id_t id; double antes;
    cfg_fabrica(&c);
    OK(cfg_valido(&c), "los valores de fábrica no son válidos");
    OK(cfg_aplicar(&c, "t_seco_s", false, false, 15, &id, &antes) == CFG_OK && id == CFG_T_SECO_S && antes == 10 && c.v[CFG_T_SECO_S] == 15, "t_seco_s=15");
    cfg_t copia = c;
    OK(cfg_aplicar(&c, "t_seco_s", false, false, 0, &id, &antes) == CFG_RANGO, "t_seco_s=0");
    OK(cfg_aplicar(&c, "t_seco_s", false, false, 9999, &id, &antes) == CFG_RANGO, "t_seco_s=9999");
    OK(cfg_aplicar(&c, "xyz", false, false, 1, &id, &antes) == CFG_DESCONOCIDO, "xyz=1");
    OK(!memcmp(&c, &copia, sizeof c), "un rechazo cambió la configuración");
    /* FR-CFG-A3/A4: los del portal y los heredados se rechazan */
    const char *portal[] = { "sonda_activa[1]", "sonda_activa", "mqtt_host", "wifi_ssid", "i_marcha_a", "i_seco_a",
                             "rango_transductor_cm", "altura_util_cm" };
    for (size_t k = 0; k < sizeof portal / sizeof portal[0]; k++)
        OK(cfg_aplicar(&c, portal[k], false, false, 1, &id, &antes) == CFG_PORTAL, "%s aceptado", portal[k]);
    /* tipos */
    OK(cfg_aplicar(&c, "casa_cerrada", false, false, 0, &id, &antes) == CFG_TIPO, "casa_cerrada numérico");
    OK(cfg_aplicar(&c, "casa_cerrada", true, false, 0, &id, &antes) == CFG_OK && c.v[CFG_CASA_CERRADA] == 0 && antes == 1, "casa_cerrada=false");
    OK(cfg_aplicar(&c, "t_seco_s", false, false, 12.5, &id, &antes) == CFG_TIPO, "entero con decimales");
    OK(cfg_aplicar(&c, "t_seco_s", true, true, 0, &id, &antes) == CFG_TIPO, "entero con bool");
    OK(cfg_aplicar(&c, "p_min_bar", false, false, 0.75, &id, &antes) == CFG_OK && c.v[CFG_P_MIN_BAR] == 0.75, "p_min_bar=0,75");
    OK(cfg_aplicar(&c, "p_min_bar", false, false, 0.1, &id, &antes) == CFG_RANGO, "p_min_bar=0,1 (D17: no apaga el seco)");
    /* nivel_rearme_pct ≥ nivel_critico_pct + 10, en los dos sentidos */
    OK(cfg_aplicar(&c, "nivel_rearme_pct", false, false, 19, &id, &antes) == CFG_RANGO, "rearme 19 con crítico 10");
    OK(cfg_aplicar(&c, "nivel_rearme_pct", false, false, 20, &id, &antes) == CFG_OK, "rearme 20 con crítico 10");
    OK(cfg_aplicar(&c, "nivel_critico_pct", false, false, 11, &id, &antes) == CFG_RANGO, "crítico 11 con rearme 20");
    OK(cfg_aplicar(&c, "nivel_critico_pct", false, false, 10, &id, &antes) == CFG_OK, "crítico 10 con rearme 20");
    /* límites de cada parámetro */
    for (int i = 0; i < CFG_N; i++) {
        const cfg_param_t *p = &CFG_PARAMS[i];
        if (p->tipo == CFG_BOOL || i == CFG_NIVEL_CRITICO_PCT || i == CFG_NIVEL_REARME_PCT) continue;
        cfg_t d; cfg_fabrica(&d);
        OK(cfg_aplicar(&d, p->nombre, false, false, p->min, &id, &antes) == CFG_OK, "%s=min", p->nombre);
        OK(cfg_aplicar(&d, p->nombre, false, false, p->max, &id, &antes) == CFG_OK, "%s=max", p->nombre);
        double paso = p->tipo == CFG_ENTERO ? 1 : 0.01;
        OK(cfg_aplicar(&d, p->nombre, false, false, p->min - paso, &id, &antes) == CFG_RANGO, "%s<min", p->nombre);
        OK(cfg_aplicar(&d, p->nombre, false, false, p->max + paso, &id, &antes) == CFG_RANGO, "%s>max", p->nombre);
    }
    char b[700];
    cfg_fabrica(&c);
    OK(cfg_json(b, sizeof b, &c) > 0 && strstr(b, "{\"t_carrera_max_s\":30,") == b && strstr(b, "\"casa_cerrada\":true")
       && strstr(b, "\"p_min_bar\":0.5") && strstr(b, "\"consumo_diario_l\":50}"), "config/state: %s", b);
    OK(cfg_json(b, 40, &c) == -1 && !b[0], "config/state que no cabe");
    c.v[CFG_T_SECO_S] = 0;
    OK(!cfg_valido(&c), "NVS fuera de rango aceptada");
    printf("configuracion: %d fallos\n", fallos);
    return fallos != 0;
}
