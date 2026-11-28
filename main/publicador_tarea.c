/* Publicador de §13. Sin E/S propia: lee la foto y las colas de la tarea de protección. */
#include "publicador_tarea.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "cola_eventos.h"
#include "configuracion.h"
#include "fugas.h"
#include "medidor.h"
#include "ordenes.h"
#include "proteccion.h"
#include "proteccion_tarea.h"
#include "publicador.h"

static const char *TAG = "nodo";
#define PASO_MS      100u      /* FR-PUB-01: cambios en ≤ 1 s */
#define MEDIDAS_MS   60000u    /* FR-PUB-03 */
#define TOP 160

static publicador_ganchos_t s_g;
static char s_base[TOP];
static cola_ev_t s_cola;                       /* FR-PUB-04 */
static SemaphoreHandle_t s_mux;
static volatile bool s_entrar;                 /* T11 pidió FR-PUB-02 + cola */
static volatile bool s_forzar_val, s_forzar_bom;   /* orden ignorada: responder con su state (FR-ORD-01) */
static const char *s_accion_val = "", *s_accion_bom = "";
static prot_foto_t s_pub;                      /* lo último publicado */
static bool s_hay_pub;

static const char *NOMBRE_VAL[] = { "arranque", "desconocida", "abierta", "cerrada", "cerrando", "abriendo", "falla" };
static const char *NOMBRE_CP[] = { "ninguno", "fuga", "orden", "bateria", "ejercicio", "sondas" };
static const char *NOMBRE_PERMISO[] = { "permitida", "cortada", "bloqueada" };
static const char *NOMBRE_MOTIVO[] = { NULL, "seco", "nivel", "flotador", "orden" };
static const char *NOMBRE_BAT[] = { "ok", "baja", "critica" };

static void topico(char t[TOP + 32], const char *suf)
{
    size_t n = strlen(s_base);                       /* s_base < TOP; los sufijos de §13.1 son cortos */
    memcpy(t, s_base, n);
    t[n] = '/';
    snprintf(t + n + 1, 31, "%s", suf);
}

static void publicar(const char *suf, const char *json, int qos, bool retain)
{
    char t[TOP + 32];
    topico(t, suf);
    esp_mqtt_client_publish(s_g.cliente(), t, json, 0, qos, retain);
}

static pub_ts_t ts(void) { pub_ts_t t = { false, 0 }; t.valido = s_g.hora && s_g.hora(&t.t); return t; }

static void pub_valvula(const prot_foto_t *f)
{
    char j[200];
    if (pub_valvula_json(j, sizeof j, NOMBRE_VAL[f->val_estado], f->bloqueo_fuga, NOMBRE_CP[f->cerrada_por],
                         f->t_cierre_ms, f->t_apertura_ms, ts()) > 0) publicar("valvula/state", j, 1, true);
}

static void pub_bomba(const prot_foto_t *f)
{
    char j[160];
    if (pub_bomba_json(j, sizeof j, NOMBRE_PERMISO[f->bom_estado], NOMBRE_MOTIVO[f->bom_motivo], f->intentos,
                       f->calibrada, ts()) > 0) publicar("bomba/state", j, 1, true);
}

static const char *nombre_sonda(int c)
{
    return c == -3 ? "inactiva" : c == -2 ? NULL : c == -1 ? "indeterminada" : son_nombre((son_clase_t)c);
}

static void pub_sonda(const prot_foto_t *f, int i)
{
    const char *n = nombre_sonda(f->sonda[i]);
    char j[80], suf[8] = "fuga/0";
    if (!n) return;                                                  /* todavía sin lectura */
    suf[5] = (char)('1' + i);
    if (pub_estado_json(j, sizeof j, n, ts()) > 0) publicar(suf, j, 1, true);
}

static void pub_digital(const char *suf, const char *estado)
{
    char j[80];
    if (pub_estado_json(j, sizeof j, estado, ts()) > 0) publicar(suf, j, 1, true);
}

static void pub_bateria(const prot_foto_t *f)
{
    char j[100];
    if (f->bateria_conocida && pub_bateria_json(j, sizeof j, f->vbat, NOMBRE_BAT[f->bateria], ts()) > 0)
        publicar("bateria", j, 1, true);
}

static void pub_medidas(const prot_foto_t *f)             /* FR-PUB-03 */
{
    char j[160];
    pub_ts_t t = ts();
    if (pub_medida_json(j, sizeof j, f->nivel_cm, "cm", true, f->nivel_pct, true, f->nivel_valido, t) > 0) publicar("nivel", j, 0, true);
    if (pub_medida_json(j, sizeof j, f->presion_bar, "bar", false, 0, true, f->presion_valida, t) > 0) publicar("presion", j, 0, true);
    if (pub_medida_json(j, sizeof j, f->i_rms, "A", false, 0, false, false, t) > 0) publicar("corriente_bomba", j, 0, true);
    pub_bateria(f);
    med_lectura_t ml;                                    /* FR-MED-04 */
    medidor_lectura(&ml);
    ml.ts_valido = t.valido; ml.ts = t.t;
    if (med_json(j, sizeof j, &ml) > 0) publicar("medidor", j, 0, true);
}

/* FR-PUB-01: publica lo que cambió desde la última vez (todo con `todo`, FR-PUB-02) */
static void pub_estados(const prot_foto_t *f, bool todo)
{
    const prot_foto_t *a = &s_pub;
    bool hay = s_hay_pub && !todo;
    if (!hay || s_forzar_val || f->val_estado != a->val_estado || f->bloqueo_fuga != a->bloqueo_fuga ||
        f->cerrada_por != a->cerrada_por || f->t_cierre_ms != a->t_cierre_ms || f->t_apertura_ms != a->t_apertura_ms)
        pub_valvula(f);
    if (!hay || s_forzar_bom || f->bom_estado != a->bom_estado || f->bom_motivo != a->bom_motivo ||
        f->intentos != a->intentos || f->calibrada != a->calibrada)
        pub_bomba(f);
    s_forzar_val = s_forzar_bom = false;
    for (int i = 0; i < 8; i++) if (!hay || f->sonda[i] != a->sonda[i]) pub_sonda(f, i);
    if (!hay || f->flotador_agua != a->flotador_agua) pub_digital("flotador", f->flotador_agua ? "agua" : "sin_agua");
    if (!hay || f->red_presente != a->red_presente) pub_digital("red", f->red_presente ? "presente" : "ausente");
    if (!hay ? f->bateria_conocida : f->bateria_conocida && (f->bateria != a->bateria || !a->bateria_conocida)) pub_bateria(f);
    s_pub = *f;
    s_hay_pub = true;
}

static void pub_evento(const cev_t *ev)
{
    char j[CEV_EXTRA_LEN + 96];
    if (cev_json(j, sizeof j, ev, (uint32_t)(esp_timer_get_time() / 1000)) > 0) publicar("evento", j, 1, false);
}

static void rechazo(const char *orden, const char *accion, const char *motivo)
{
    ESP_LOGI(TAG, "ord: %s %s -> rechazado:%s", orden, accion, motivo);                      /* A6 */
    ESP_LOGW(TAG, "eve: orden_rechazada %s %s", orden, motivo);                             /* A9 */
    cev_t ev = { .tipo = "orden_rechazada", .t_ms = (uint32_t)(esp_timer_get_time() / 1000) };
    ev.ts_valido = s_g.hora && s_g.hora(&ev.ts);
    snprintf(ev.extra, sizeof ev.extra, "\"orden\":\"%s\",\"motivo\":\"%s\"", orden, motivo);
    pub_evento(&ev);
}

static void pub_config(void)                     /* config/state completo, retain (§13.1) */
{
    cfg_t c;
    config_actual(&c);
    char j[720];
    if (cfg_json(j, sizeof j, &c) > 0) publicar("config/state", j, 1, true);
}

/* config/set (§16): FR-CFG-A1…A5 */
static void orden_config(const char *p, size_t n)
{
    char param[ORD_TXT]; ord_campo_t v;
    ord_res_t r = ord_config(p, n, param, &v);
    if (r != ORD_OK) { rechazo("config/set", "?", ord_motivo(r)); return; }
    cfg_t c; cfg_id_t id; double antes;
    config_actual(&c);
    cfg_res_t cr = cfg_aplicar(&c, param, v.tipo == ORDV_BOOL, v.b, v.num, &id, &antes);
    if (cr != CFG_OK) { rechazo("config/set", param, "payload"); return; }        /* FR-CFG-A2/A3/A4 */
    if (!config_guardar_y_aplicar(&c)) { rechazo("config/set", param, "payload"); return; }
    ESP_LOGI(TAG, "ord: config/set %s -> ok", param);                                     /* A6 */
    pub_config();
    if (id == CFG_CASA_CERRADA && c.v[id] != antes) {                                     /* FR-CFG-A5 */
        cev_t ev = { .tipo = "casa_cerrada", .t_ms = (uint32_t)(esp_timer_get_time() / 1000) };
        ev.ts_valido = s_g.hora && s_g.hora(&ev.ts);
        snprintf(ev.extra, sizeof ev.extra, "\"v\":%s,\"antes\":%s", c.v[id] ? "true" : "false", antes ? "true" : "false");
        ESP_LOGW(TAG, "eve: casa_cerrada v=%d antes=%d", c.v[id] != 0, antes != 0);      /* A9 */
        pub_evento(&ev);
    }
}

static void tarea(void *arg)
{
    (void)arg;
    uint32_t ultima_med = 0;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PASO_MS));
        bool op = s_g.operativo();
        prot_foto_t f;
        proteccion_foto(&f);
        if (op && s_entrar) {                                        /* T11: FR-PUB-02 y luego la cola */
            s_entrar = false;
            pub_estados(&f, true);
            pub_medidas(&f);
            pub_config();
            bool inactivas[8];                                       /* FR-CFG-A3: una vez tras guardar el portal */
            if (nodo_sondas_pendiente(inactivas)) {
                cev_t ev = { .tipo = "sondas_config", .t_ms = (uint32_t)(esp_timer_get_time() / 1000) };
                ev.ts_valido = s_g.hora && s_g.hora(&ev.ts);
                size_t k = (size_t)snprintf(ev.extra, sizeof ev.extra, "\"inactivas\":[");
                for (int i = 0; i < 8; i++)
                    if (inactivas[i]) k += (size_t)snprintf(ev.extra + k, sizeof ev.extra - k, "%s%d", ev.extra[k - 1] == '[' ? "" : ",", i + 1);
                snprintf(ev.extra + k, sizeof ev.extra - k, "]");
                ESP_LOGW(TAG, "eve: sondas_config %s", ev.extra);                            /* A9 */
                pub_evento(&ev);
                nodo_sondas_hecho();
            }
            ultima_med = (uint32_t)(esp_timer_get_time() / 1000);
            xSemaphoreTake(s_mux, portMAX_DELAY);
            cev_t ev;
            while (cev_sacar(&s_cola, &ev)) pub_evento(&ev);
            xSemaphoreGive(s_mux);
        } else if (op) {
            pub_estados(&f, false);
            uint32_t ahora = (uint32_t)(esp_timer_get_time() / 1000);
            if (ahora - ultima_med >= MEDIDAS_MS) { ultima_med = ahora; pub_medidas(&f); }
        }
        cev_t ev;                                                    /* FR-PUB-04 */
        while (proteccion_evento(&ev, 0)) {
            if (op && !s_entrar) { pub_evento(&ev); continue; }
            xSemaphoreTake(s_mux, portMAX_DELAY);
            cev_poner(&s_cola, &ev);                                 /* los no críticos se descartan */
            xSemaphoreGive(s_mux);
        }
        prot_resp_t r;                                               /* FR-ORD-01 */
        while (proteccion_respuesta(&r, 0)) {
            const char *orden = r.bomba ? "bomba/set" : "valvula/set", *acc = r.bomba ? s_accion_bom : s_accion_val;
            if (r.rechazo) { if (op) rechazo(orden, acc, r.rechazo); continue; }
            ESP_LOGI(TAG, "ord: %s %s -> ok", orden, acc);                                   /* A6 */
            if (r.fila == 0) { if (r.bomba) s_forzar_bom = true; else s_forzar_val = true; }
        }
    }
}

static bool es(const esp_mqtt_event_t *d, const char *suf)
{
    char t[TOP + 32];
    topico(t, suf);
    size_t n = strlen(t);
    return d->topic_len == (int)n && !memcmp(d->topic, t, n);
}

void publicador_mensaje(const esp_mqtt_event_t *d)
{
    const char *orden = es(d, "valvula/set") ? "valvula/set" : es(d, "bomba/set") ? "bomba/set"
                      : es(d, "medidor/set") ? "medidor/set" : es(d, "config/set") ? "config/set" : NULL;
    if (!orden || d->current_data_offset != 0) return;              /* el resto de un mensaje troceado */
    if (d->retain) { ESP_LOGI(TAG, "ord: %s ignorada (retain)", orden); return; }   /* FR-ORD-03 */
    bool troceado = d->total_data_len > d->data_len;
    size_t n = troceado ? ORD_MAX_BYTES + 1 : (size_t)d->data_len;
    if (orden[0] == 'c') { orden_config(d->data, n); return; }
    if (orden[0] == 'm') {                                          /* FR-MED-06/10 */
        int min = 0;
        med_orden_t r = troceado ? MORD_PAYLOAD : med_orden(d->data, n, &min);
        if (r != MORD_OK) { rechazo(orden, "permitir_consumo", med_orden_motivo(r)); return; }
        int hasta = 0;
        med_resultado_t m = medidor_permitir(min, &hasta);
        ESP_LOGI(TAG, "ord: medidor/set permitir_consumo -> ok");                             /* A6 */
        if (m == MED_EVENTO) {
            cev_t ev = { .tipo = "consumo_permitido", .t_ms = (uint32_t)(esp_timer_get_time() / 1000) };
            ev.ts_valido = s_g.hora && s_g.hora(&ev.ts);
            snprintf(ev.extra, sizeof ev.extra, "\"hasta_min\":%d", hasta);
            ESP_LOGW(TAG, "eve: consumo_permitido hasta_min=%d", hasta);                       /* A9 */
            pub_evento(&ev);
        }
        return;
    }
    static const char *const AV[] = { "cerrar", "abrir", "ejercicio", "posponer_ejercicio" };
    static const char *const AB[] = { "cortar", "permitir", "rearmar" };
    static const int PV[] = { PORD_CERRAR, PORD_ABRIR, PORD_EJERCICIO, PORD_POSPONER };
    static const int PB[] = { PORD_CORTAR, PORD_PERMITIR, PORD_REARMAR };
    ord_res_t r; int pord;
    if (orden[0] == 'v') {
        ord_valvula_t a; ord_motivo_t m;
        r = troceado ? ORD_PAYLOAD : ord_valvula(d->data, n, &a, &m);
        if (r == ORD_OK) { s_accion_val = AV[a]; pord = PV[a]; }
    } else {
        ord_bomba_t a;
        r = troceado ? ORD_PAYLOAD : ord_bomba(d->data, n, &a);
        if (r == ORD_OK) { s_accion_bom = AB[a]; pord = PB[a]; }
    }
    if (r != ORD_OK) { rechazo(orden, "?", ord_motivo(r)); return; }   /* FR-ORD-02 */
    if (!proteccion_orden(pord)) rechazo(orden, orden[0] == 'v' ? s_accion_val : s_accion_bom, "ocupado");
}

void publicador_entrar_operativo(void)
{
    char t[TOP + 32];
    const char *subs[] = { "valvula/set", "bomba/set", "medidor/set", "config/set" };
    for (int i = 0; i < 4; i++) { topico(t, subs[i]); esp_mqtt_client_subscribe(s_g.cliente(), t, 1); }
    s_entrar = true;
}

uint32_t publicador_perdidos(void)
{
    if (!s_mux) return 0;
    xSemaphoreTake(s_mux, portMAX_DELAY);
    uint32_t n = s_cola.perdidos;
    xSemaphoreGive(s_mux);
    return n;
}

void publicador_iniciar(const publicador_ganchos_t *g, const char *base)
{
    s_g = *g;
    snprintf(s_base, sizeof s_base, "%s", base);
    s_mux = xSemaphoreCreateMutex();
    cev_iniciar(&s_cola);
    xTaskCreate(tarea, "publicador", 6144, NULL, 4, NULL);
}
