/* L1 configuracion — parámetros de campo de §19 (interfaz `mqtt`) y `config/set` (§16, D17).
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. */
#pragma once
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    CFG_T_CARRERA_MAX_S, CFG_EJERCICIO_DIAS, CFG_EJERCICIO_HORA, CFG_CASA_CERRADA, CFG_ZUMBADOR_ACTIVO,
    CFG_ZUMBADOR_MIN, CFG_NIVEL_CRITICO_PCT, CFG_NIVEL_REARME_PCT, CFG_T_REARME_MIN, CFG_INTENTOS_MAX,
    CFG_P_MIN_BAR, CFG_T_SECO_S, CFG_CICLADO_MAX, CFG_CICLADO_VENTANA_MIN, CFG_GALONES_POR_PULSO,
    CFG_CONSUMO_ALERTA_L, CFG_CONSUMO_VENTANA_MIN, CFG_CONSUMO_MARGEN_MIN, CFG_CONSUMO_DIARIO_L,
    CFG_N
} cfg_id_t;

typedef enum { CFG_ENTERO, CFG_DECIMAL, CFG_BOOL } cfg_tipo_t;
typedef struct { const char *nombre; cfg_tipo_t tipo; double fabrica, min, max; } cfg_param_t;
extern const cfg_param_t CFG_PARAMS[CFG_N];

typedef struct { double v[CFG_N]; } cfg_t;

void cfg_fabrica(cfg_t *c);
/* ¿Es un parámetro del portal o heredado de clima §17 (FR-CFG-A3/A4)? */
bool cfg_es_del_portal(const char *nombre);

typedef enum { CFG_OK, CFG_DESCONOCIDO, CFG_PORTAL, CFG_TIPO, CFG_RANGO } cfg_res_t;
/* FR-CFG-A1/A2: valida y, si vale, aplica; *id y *antes dicen qué cambió. Un entero debe ser
 * entero; un bool, true/false. nivel_rearme_pct ≥ nivel_critico_pct + 10 en los dos sentidos. */
cfg_res_t cfg_aplicar(cfg_t *c, const char *nombre, bool es_bool, bool b, double num, cfg_id_t *id, double *antes);
/* config/state completo (§13.1): {"t_carrera_max_s":30,…}; -1 si no cabe */
int cfg_json(char *buf, size_t cap, const cfg_t *c);
/* Un cfg leído de la NVS: todos los valores dentro de su rango; si no, se usan los de fábrica */
bool cfg_valido(const cfg_t *c);
