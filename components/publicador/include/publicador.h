/* L1 publicador — tópicos MQTT (FSD §13; clima:FR-MQTT-01) y `status` con los
 * 9 campos de diagnóstico heredados (clima:FR-MQTT-08).
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. Todas las funciones
 * devuelven la longitud escrita, o -1 si no cabe (el búfer queda vacío). */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* «<sitio>/<zona>/<nodo>/<sufijo>» */
int pub_topico(char *buf, size_t cap, const char *sitio, const char *zona, const char *nodo, const char *sufijo);

typedef struct {
    bool online;            /* estado: online | offline */
    const char *fw;         /* versión de la aplicación */
    int rssi;               /* dBm */
    const char *reset;      /* valor de §9.3 (diag_reset_nombre) */
    uint32_t uptime_s;
    uint32_t lecturas;
    uint32_t err_sensor;
    const char *ota;        /* ninguno | en_curso | ok | rechazado:… | error:… */
    bool hora_valida;
    uint32_t eventos_perdidos;  /* FR-PUB-05: descartados por la cola llena */
} pub_status_t;

/* Diagnóstico de §9.3 en su orden: estado, fw, rssi, reset, uptime_s, lecturas, err_sensor, ota, hora_valida,
 * y `eventos_perdidos` (FR-PUB-05) al final */
int pub_status_json(char *buf, size_t cap, const pub_status_t *s);

/* Last Will de `status` (§9.2) */
#define PUB_LWT_OFFLINE "{\"estado\":\"offline\"}"

/* ---- Payloads de §13.1 (FR-PUB-01/02/03). `ts` (época Unix) solo si ts_valido ---- */
typedef struct { bool valido; uint32_t t; } pub_ts_t;
/* valvula/state: estado en minúsculas; cerrada_por de §5.2; t_*_ms < 0 = sin medir */
int pub_valvula_json(char *buf, size_t cap, const char *estado, bool bloqueo_fuga, const char *cerrada_por,
                     int32_t t_cierre_ms, int32_t t_apertura_ms, pub_ts_t ts);
/* bomba/state: permiso permitida | cortada | bloqueada; motivo NULL fuera de CORTADA */
int pub_bomba_json(char *buf, size_t cap, const char *permiso, const char *motivo, int intentos_24h, bool calibrada, pub_ts_t ts);
/* fuga/<n>, flotador, red: {"estado":…} */
int pub_estado_json(char *buf, size_t cap, const char *estado, pub_ts_t ts);
/* nivel (con pct), presion, corriente_bomba: v solo si valido; con_valido = el tópico lleva "valido" */
int pub_medida_json(char *buf, size_t cap, float v, const char *unidad, bool con_pct, float pct,
                    bool con_valido, bool valido, pub_ts_t ts);
/* bateria: {"v":…,"u":"V","estado":ok|baja|critica} */
int pub_bateria_json(char *buf, size_t cap, float v, const char *estado, pub_ts_t ts);
