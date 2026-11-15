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
} pub_status_t;

/* Diagnóstico de §9.3 en su orden: estado, fw, rssi, reset, uptime_s, lecturas, err_sensor, ota, hora_valida */
int pub_status_json(char *buf, size_t cap, const pub_status_t *s);

/* Last Will de `status` (§9.2) */
#define PUB_LWT_OFFLINE "{\"estado\":\"offline\"}"
