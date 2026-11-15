/* L1 ha_discovery — Discovery de Home Assistant (FSD §14; FR-HA-A1). Núcleo puro. */
#include "ha_discovery.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

/* Una entidad: componente de HA, objeto (sufijo del unique_id), nombre y el
 * resto de sus campos como fragmento JSON fijo (tópicos relativos a «~»). */
typedef struct { const char *comp, *obj, *nombre, *campos; bool expira; } had_def_t;

#define VJ(x) "{{ value_json." x " }}"
#define FUGA(n) {"binary_sensor", "fuga_" #n, "Fuga " #n, \
    "\"state_topic\":\"~/fuga/" #n "\",\"value_template\":\"{{ 'ON' if value_json.estado == 'mojada' else 'OFF' }}\"," \
    "\"device_class\":\"moisture\"", false}

static const had_def_t DEF[] = {
    {"valve", "valvula", "Válvula general",
     "\"command_topic\":\"~/valvula/set\",\"state_topic\":\"~/valvula/state\","
     "\"payload_open\":\"{\\\"accion\\\":\\\"abrir\\\",\\\"motivo\\\":\\\"manual\\\"}\","
     "\"payload_close\":\"{\\\"accion\\\":\\\"cerrar\\\",\\\"motivo\\\":\\\"manual\\\"}\","
     "\"value_template\":\"{{ {'abierta':'open','cerrada':'closed','abriendo':'opening','cerrando':'closing'}"
     ".get(value_json.estado, 'None') }}\",\"device_class\":\"water\",\"qos\":1", false},
    {"switch", "bomba", "Permiso de la bomba",
     "\"command_topic\":\"~/bomba/set\",\"state_topic\":\"~/bomba/state\","
     "\"payload_on\":\"{\\\"accion\\\":\\\"permitir\\\"}\",\"payload_off\":\"{\\\"accion\\\":\\\"cortar\\\"}\","
     "\"value_template\":\"{{ 'ON' if value_json.permiso == 'permitida' else 'OFF' }}\","
     "\"state_on\":\"ON\",\"state_off\":\"OFF\",\"qos\":1", false},
    FUGA(1), FUGA(2), FUGA(3), FUGA(4), FUGA(5), FUGA(6), FUGA(7), FUGA(8),
    {"binary_sensor", "flotador", "Flotador",
     "\"state_topic\":\"~/flotador\",\"value_template\":\"{{ 'ON' if value_json.estado == 'agua' else 'OFF' }}\","
     "\"device_class\":\"moisture\"", false},
    {"binary_sensor", "red", "Red eléctrica",
     "\"state_topic\":\"~/red\",\"value_template\":\"{{ 'ON' if value_json.estado == 'presente' else 'OFF' }}\","
     "\"device_class\":\"power\"", false},
    {"sensor", "nivel", "Nivel de la cisterna",
     "\"state_topic\":\"~/nivel\",\"value_template\":\"" VJ("v") "\",\"unit_of_measurement\":\"cm\","
     "\"device_class\":\"distance\",\"state_class\":\"measurement\"", true},
    {"sensor", "nivel_pct", "Nivel de la cisterna (%)",
     "\"state_topic\":\"~/nivel\",\"value_template\":\"" VJ("pct") "\",\"unit_of_measurement\":\"%\","
     "\"state_class\":\"measurement\"", true},
    {"sensor", "presion", "Presión",
     "\"state_topic\":\"~/presion\",\"value_template\":\"" VJ("v") "\",\"unit_of_measurement\":\"bar\","
     "\"device_class\":\"pressure\",\"state_class\":\"measurement\"", true},
    {"sensor", "corriente_bomba", "Corriente de la bomba",
     "\"state_topic\":\"~/corriente_bomba\",\"value_template\":\"" VJ("v") "\",\"unit_of_measurement\":\"A\","
     "\"device_class\":\"current\",\"state_class\":\"measurement\"", true},
    {"sensor", "vbat", "Tensión de la batería",
     "\"state_topic\":\"~/bateria\",\"value_template\":\"" VJ("v") "\",\"unit_of_measurement\":\"V\","
     "\"device_class\":\"voltage\",\"state_class\":\"measurement\"", true},
    {"sensor", "estado_bateria", "Estado de la batería",
     "\"state_topic\":\"~/bateria\",\"value_template\":\"" VJ("estado") "\",\"device_class\":\"enum\","
     "\"options\":[\"ok\",\"baja\",\"critica\"]", true},
    {"button", "rearmar_bomba", "Rearmar la bomba",
     "\"command_topic\":\"~/bomba/set\",\"payload_press\":\"{\\\"accion\\\":\\\"rearmar\\\"}\",\"qos\":1", false},
    {"button", "ejercicio", "Ejercitar la válvula",
     "\"command_topic\":\"~/valvula/set\","
     "\"payload_press\":\"{\\\"accion\\\":\\\"ejercicio\\\",\\\"motivo\\\":\\\"manual\\\"}\",\"qos\":1", false},
    {"event", "evento", "Evento",
     "\"state_topic\":\"~/evento\",\"value_template\":\"{ \\\"event_type\\\": \\\"" VJ("tipo") "\\\" }\","
     "\"event_types\":[\"fuga\",\"falla_valvula\",\"movimiento_manual\",\"falla_sonda\",\"falla_adc\","
     "\"bomba_seco\",\"bomba_bloqueada\",\"ciclado_anormal\",\"bateria_baja\",\"bateria_critica\","
     "\"orden_rechazada\",\"flotador_falla\"]", false},
};
#define N_DEF ((int)(sizeof DEF / sizeof DEF[0]))

int had_n_entidades(void) { return N_DEF; }

/* Escritor acotado: si algo no cabe, todo el resultado es -1 y el búfer queda vacío */
typedef struct { char *buf; size_t cap, len; bool roto; } esc_t;

static void poner(esc_t *e, const char *fmt, ...)
{
    if (e->roto) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(e->buf + e->len, e->cap - e->len, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= e->cap - e->len) { e->roto = true; return; }
    e->len += (size_t)n;
}

/* Contenido de una cadena JSON (sin comillas) con escape de comillas, barra inversa y controles */
static void escapar(esc_t *e, const char *s)
{
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p && !e->roto; p++) {
        if (*p == '"' || *p == '\\') poner(e, "\\%c", *p);
        else if (*p < 0x20) poner(e, "\\u%04x", *p);
        else poner(e, "%c", *p);
    }
}

static int fin(esc_t *e)
{
    if (e->roto) { if (e->cap) e->buf[0] = '\0'; return -1; }
    return (int)e->len;
}

static bool valida(size_t cap, int i) { return cap > 0 && i >= 0 && i < N_DEF; }

int had_topico(char *buf, size_t cap, const char *nodo, int i)
{
    esc_t e = {buf, cap, 0, !valida(cap, i)};
    if (!e.roto) poner(&e, HAD_PREFIJO "/%s/%s/%s/config", DEF[i].comp, nodo, DEF[i].obj);
    return fin(&e);
}

int had_unique_id(char *buf, size_t cap, const char *nodo, int i)
{
    esc_t e = {buf, cap, 0, !valida(cap, i)};
    if (!e.roto) poner(&e, "%s_%s", nodo, DEF[i].obj);
    return fin(&e);
}

int had_config_json(char *buf, size_t cap, const char *sitio, const char *zona,
                    const char *nodo, const char *fw, int i)
{
    esc_t e = {buf, cap, 0, !valida(cap, i)};
    if (e.roto) return fin(&e);
    const had_def_t *d = &DEF[i];
    poner(&e, "{\"~\":\"");
    escapar(&e, sitio); poner(&e, "/"); escapar(&e, zona); poner(&e, "/"); escapar(&e, nodo);
    poner(&e, "\",\"name\":\""); escapar(&e, d->nombre);
    poner(&e, "\",\"unique_id\":\""); escapar(&e, nodo); poner(&e, "_%s\",", d->obj);  /* sin sitio ni zona */
    poner(&e, "%s", d->campos);
    if (d->expira) poner(&e, ",\"expire_after\":%d", HAD_EXPIRE_AFTER_S);
    poner(&e, ",\"availability_topic\":\"~/status\",\"availability_template\":\"{{ value_json.estado }}\"");
    poner(&e, ",\"payload_available\":\"online\",\"payload_not_available\":\"offline\"");
    poner(&e, ",\"device\":{\"identifiers\":[\""); escapar(&e, nodo);
    poner(&e, "\"],\"name\":\"Agua "); escapar(&e, sitio); poner(&e, " "); escapar(&e, zona);
    poner(&e, "\",\"model\":\"%s\",\"sw_version\":\"", HAD_MODELO); escapar(&e, fw);
    poner(&e, "\"}}");
    return fin(&e);
}
