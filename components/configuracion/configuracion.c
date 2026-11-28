/* L1 configuracion — §16 y §19. Núcleo puro. */
#include "configuracion.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

const cfg_param_t CFG_PARAMS[CFG_N] = {
    [CFG_T_CARRERA_MAX_S]     = { "t_carrera_max_s",     CFG_ENTERO,  30,  5,   120 },
    [CFG_EJERCICIO_DIAS]      = { "ejercicio_dias",      CFG_ENTERO,  30,  7,   90 },
    [CFG_EJERCICIO_HORA]      = { "ejercicio_hora",      CFG_ENTERO,  3,   0,   23 },
    [CFG_CASA_CERRADA]        = { "casa_cerrada",        CFG_BOOL,    1,   0,   1 },
    [CFG_ZUMBADOR_ACTIVO]     = { "zumbador_activo",     CFG_BOOL,    1,   0,   1 },
    [CFG_ZUMBADOR_MIN]        = { "zumbador_min",        CFG_ENTERO,  5,   1,   30 },
    [CFG_NIVEL_CRITICO_PCT]   = { "nivel_critico_pct",   CFG_ENTERO,  10,  5,   30 },
    [CFG_NIVEL_REARME_PCT]    = { "nivel_rearme_pct",    CFG_ENTERO,  30,  15,  90 },   /* y ≥ crítico + 10 */
    [CFG_T_REARME_MIN]        = { "t_rearme_min",        CFG_ENTERO,  10,  2,   120 },
    [CFG_INTENTOS_MAX]        = { "intentos_max",        CFG_ENTERO,  3,   1,   5 },
    [CFG_P_MIN_BAR]           = { "p_min_bar",           CFG_DECIMAL, 0.5, 0.2, 5 },
    [CFG_T_SECO_S]            = { "t_seco_s",            CFG_ENTERO,  10,  3,   60 },
    [CFG_CICLADO_MAX]         = { "ciclado_max",         CFG_ENTERO,  6,   3,   30 },
    [CFG_CICLADO_VENTANA_MIN] = { "ciclado_ventana_min", CFG_ENTERO,  60,  10,  240 },
    [CFG_GALONES_POR_PULSO]   = { "galones_por_pulso",   CFG_DECIMAL, 1.0, 0.1, 100 },
    [CFG_CONSUMO_ALERTA_L]    = { "consumo_alerta_l",    CFG_ENTERO,  20,  5,   1000 },
    [CFG_CONSUMO_VENTANA_MIN] = { "consumo_ventana_min", CFG_ENTERO,  60,  10,  1440 },
    [CFG_CONSUMO_MARGEN_MIN]  = { "consumo_margen_min",  CFG_ENTERO,  120, 0,   720 },
    [CFG_CONSUMO_DIARIO_L]    = { "consumo_diario_l",    CFG_ENTERO,  50,  10,  2000 },
};

void cfg_fabrica(cfg_t *c)
{
    for (int i = 0; i < CFG_N; i++) c->v[i] = CFG_PARAMS[i].fabrica;
}

bool cfg_es_del_portal(const char *n)
{
    static const char *const P[] = { "rango_transductor_cm", "altura_util_cm", "i_marcha_a", "i_seco_a",
                                     "wifi_ssid", "wifi_pass", "mqtt_host", "mqtt_port", "mqtt_user", "mqtt_pass",
                                     "zona", "sitio" };
    if (!strncmp(n, "sonda_activa", 12)) return true;                  /* FR-CFG-A3, con o sin [n] */
    for (size_t i = 0; i < sizeof P / sizeof P[0]; i++) if (!strcmp(n, P[i])) return true;
    return false;
}

static bool en_rango(const cfg_t *c, int i, double x)
{
    const cfg_param_t *p = &CFG_PARAMS[i];
    if (x < p->min || x > p->max) return false;
    if (i == CFG_NIVEL_REARME_PCT && x < c->v[CFG_NIVEL_CRITICO_PCT] + 10) return false;
    if (i == CFG_NIVEL_CRITICO_PCT && c->v[CFG_NIVEL_REARME_PCT] < x + 10) return false;
    return true;
}

cfg_res_t cfg_aplicar(cfg_t *c, const char *nombre, bool es_bool, bool b, double num, cfg_id_t *id, double *antes)
{
    if (cfg_es_del_portal(nombre)) return CFG_PORTAL;
    int i = 0;
    while (i < CFG_N && strcmp(nombre, CFG_PARAMS[i].nombre)) i++;
    if (i == CFG_N) return CFG_DESCONOCIDO;
    const cfg_param_t *p = &CFG_PARAMS[i];
    double x;
    if (p->tipo == CFG_BOOL) { if (!es_bool) return CFG_TIPO; x = b ? 1 : 0; }
    else {
        if (es_bool || !isfinite(num)) return CFG_TIPO;
        if (p->tipo == CFG_ENTERO && num != floor(num)) return CFG_TIPO;
        x = num;
    }
    if (!en_rango(c, i, x)) return CFG_RANGO;
    *id = (cfg_id_t)i;
    *antes = c->v[i];
    c->v[i] = x;
    return CFG_OK;
}

int cfg_json(char *buf, size_t cap, const cfg_t *c)
{
    size_t n = 0;
    for (int i = 0; i < CFG_N; i++) {
        const cfg_param_t *p = &CFG_PARAMS[i];
        int k;
        const char *sep = i ? "," : "{";
        if (p->tipo == CFG_BOOL) k = snprintf(buf + n, cap - n, "%s\"%s\":%s", sep, p->nombre, c->v[i] ? "true" : "false");
        else if (p->tipo == CFG_ENTERO) k = snprintf(buf + n, cap - n, "%s\"%s\":%.0f", sep, p->nombre, c->v[i]);
        else k = snprintf(buf + n, cap - n, "%s\"%s\":%g", sep, p->nombre, c->v[i]);
        if (k < 0 || (size_t)k >= cap - n) { if (cap) buf[0] = 0; return -1; }
        n += (size_t)k;
    }
    if (n + 2 > cap) { if (cap) buf[0] = 0; return -1; }
    buf[n++] = '}'; buf[n] = 0;
    return (int)n;
}

bool cfg_valido(const cfg_t *c)
{
    for (int i = 0; i < CFG_N; i++) if (!isfinite(c->v[i]) || !en_rango(c, i, c->v[i])) return false;
    return true;
}
