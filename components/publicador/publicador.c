/* L1 publicador — tópicos y `status` MQTT (FSD §13; clima:FR-MQTT-01, clima:FR-MQTT-08).
 * Los payloads de §13.1 se añaden con la fase que los publica. Núcleo puro. */
#include "publicador.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

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

/* Cadena JSON con escape de comillas, barra inversa y controles */
static void cadena(esc_t *e, const char *s)
{
    poner(e, "\"");
    for (const unsigned char *p = (const unsigned char *)(s ? s : ""); *p && !e->roto; p++) {
        if (*p == '"' || *p == '\\') poner(e, "\\%c", *p);
        else if (*p < 0x20) poner(e, "\\u%04x", *p);
        else poner(e, "%c", *p);
    }
    poner(e, "\"");
}

static int fin(esc_t *e)
{
    if (e->roto) { if (e->cap) e->buf[0] = '\0'; return -1; }
    return (int)e->len;
}

int pub_topico(char *buf, size_t cap, const char *sitio, const char *zona, const char *nodo, const char *sufijo)
{
    esc_t e = {buf, cap, 0, cap == 0};
    poner(&e, "%s/%s/%s/%s", sitio, zona, nodo, sufijo);
    return fin(&e);
}

int pub_status_json(char *buf, size_t cap, const pub_status_t *s)
{
    esc_t e = {buf, cap, 0, cap == 0};
    poner(&e, "{\"estado\":\"%s\",\"fw\":", s->online ? "online" : "offline");
    cadena(&e, s->fw);
    poner(&e, ",\"rssi\":%d,\"reset\":", s->rssi);
    cadena(&e, s->reset ? s->reset : "otro");
    poner(&e, ",\"uptime_s\":%" PRIu32 ",\"lecturas\":%" PRIu32 ",\"err_sensor\":%" PRIu32 ",\"ota\":",
          s->uptime_s, s->lecturas, s->err_sensor);
    cadena(&e, s->ota ? s->ota : "ninguno");
    poner(&e, ",\"hora_valida\":%s,\"eventos_perdidos\":%" PRIu32 "}", s->hora_valida ? "true" : "false", s->eventos_perdidos);
    return fin(&e);
}

static void ts(esc_t *e, pub_ts_t t)
{
    if (t.valido) poner(e, ",\"ts\":%" PRIu32, t.t);
    poner(e, "}");
}

int pub_valvula_json(char *buf, size_t cap, const char *estado, bool bloqueo_fuga, const char *cerrada_por,
                     int32_t t_cierre_ms, int32_t t_apertura_ms, pub_ts_t t)
{
    esc_t e = {buf, cap, 0, cap == 0};
    poner(&e, "{\"estado\":");
    cadena(&e, estado);
    poner(&e, ",\"bloqueo_fuga\":%s,\"cerrada_por\":", bloqueo_fuga ? "true" : "false");
    cadena(&e, cerrada_por);
    if (t_cierre_ms >= 0) poner(&e, ",\"t_cierre_ms\":%" PRId32, t_cierre_ms);
    if (t_apertura_ms >= 0) poner(&e, ",\"t_apertura_ms\":%" PRId32, t_apertura_ms);
    ts(&e, t);
    return fin(&e);
}

int pub_bomba_json(char *buf, size_t cap, const char *permiso, const char *motivo, int intentos_24h, bool calibrada, pub_ts_t t)
{
    esc_t e = {buf, cap, 0, cap == 0};
    poner(&e, "{\"permiso\":");
    cadena(&e, permiso);
    if (motivo) { poner(&e, ",\"motivo\":"); cadena(&e, motivo); }
    poner(&e, ",\"intentos_24h\":%d,\"calibrada\":%s", intentos_24h, calibrada ? "true" : "false");
    ts(&e, t);
    return fin(&e);
}

int pub_estado_json(char *buf, size_t cap, const char *estado, pub_ts_t t)
{
    esc_t e = {buf, cap, 0, cap == 0};
    poner(&e, "{\"estado\":");
    cadena(&e, estado);
    ts(&e, t);
    return fin(&e);
}

int pub_medida_json(char *buf, size_t cap, float v, const char *unidad, bool con_pct, float pct,
                    bool con_valido, bool valido, pub_ts_t t)
{
    esc_t e = {buf, cap, 0, cap == 0};
    bool hay_v = !con_valido || valido;
    poner(&e, "{");
    if (hay_v) poner(&e, "\"v\":%.2f,", (double)v);
    poner(&e, "\"u\":");
    cadena(&e, unidad);
    if (con_pct && hay_v) poner(&e, ",\"pct\":%.1f", (double)pct);
    if (con_valido) poner(&e, ",\"valido\":%s", valido ? "true" : "false");
    ts(&e, t);
    return fin(&e);
}

int pub_bateria_json(char *buf, size_t cap, float v, const char *estado, pub_ts_t t)
{
    esc_t e = {buf, cap, 0, cap == 0};
    poner(&e, "{\"v\":%.2f,\"u\":\"V\",\"estado\":", (double)v);
    cadena(&e, estado);
    ts(&e, t);
    return fin(&e);
}
