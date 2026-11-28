/* L1 ordenes — análisis estricto de las órdenes MQTT de §13.1 y §16 (FR-ORD-02, P15).
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. */
#pragma once
#include <stdbool.h>
#include <stddef.h>

#define ORD_MAX_BYTES 256          /* más bytes: rechazo `payload` (FR-ORD-02) */
#define ORD_MAX_CAMPOS 4
#define ORD_TXT 32

typedef enum { ORDV_TEXTO, ORDV_NUMERO, ORDV_BOOL } ord_tipo_t;
typedef struct { char clave[ORD_TXT]; ord_tipo_t tipo; char texto[ORD_TXT]; double num; bool b; } ord_campo_t;
typedef struct { ord_campo_t c[ORD_MAX_CAMPOS]; int n; } ord_obj_t;

/* Objeto JSON plano: claves de texto; valores de texto, número o true/false. false si no es
 * JSON válido, pasa de ORD_MAX_BYTES, repite una clave o tiene más de ORD_MAX_CAMPOS campos. */
bool ord_objeto(const char *p, size_t n, ord_obj_t *o);
const ord_campo_t *ord_campo(const ord_obj_t *o, const char *clave);

typedef enum { ORD_OK, ORD_PAYLOAD, ORD_ACCION } ord_res_t;
const char *ord_motivo(ord_res_t r);        /* "payload" | "accion" | "" */

/* `valvula/set` (§13.1): accion ∈ cerrar | abrir | ejercicio | posponer_ejercicio; motivo opcional
 * ∈ fuga_remota | casa_cerrada | llegada | manual (otro valor o tipo → payload) */
typedef enum { OV_CERRAR, OV_ABRIR, OV_EJERCICIO, OV_POSPONER } ord_valvula_t;
typedef enum { OM_NINGUNO, OM_FUGA_REMOTA, OM_CASA_CERRADA, OM_LLEGADA, OM_MANUAL } ord_motivo_t;
ord_res_t ord_valvula(const char *p, size_t n, ord_valvula_t *a, ord_motivo_t *m);

/* `bomba/set` (§13.1): accion ∈ cortar | permitir | rearmar */
typedef enum { OB_CORTAR, OB_PERMITIR, OB_REARMAR } ord_bomba_t;
ord_res_t ord_bomba(const char *p, size_t n, ord_bomba_t *a);

/* `config/set` (§16): {"param":"<nombre>","v":<número | true | false>}; el nombre y el rango
 * los comprueba el catálogo (FR-CFG-A1/A2), aquí solo la forma */
ord_res_t ord_config(const char *p, size_t n, char param[ORD_TXT], ord_campo_t *v);
