/* L1 ha_discovery — configuraciones de Discovery de Home Assistant (FSD §14;
 * clima:FR-HA-01…03, FR-HA-A1): una por entidad, en
 * homeassistant/<componente>/<nodo>/<objeto>/config, con unique_id =
 * <nodo>_<objeto> (FR-HA-A1), sw_version = la versión en ejecución
 * (clima:FR-HA-02) y expire_after = 180 s en las medidas periódicas
 * (clima:FR-HA-03). Los tópicos de estado y de órdenes son los de §13.1, bajo
 * la base «~» = <sitio>/<zona>/<nodo>. Núcleo puro, sin ESP-IDF; lo compila
 * tests/host. Las funciones devuelven la longitud escrita, o -1 si no cabe
 * (el búfer queda vacío) o si la entidad no existe.
 *
 * Pendiente (defecto del FSD, decisión del propietario): §14 pide el estado de
 * la batería como sensor `device_class: battery` en %, pero no define la
 * conversión de Vbat a %; esa entidad no se anuncia hasta que se defina. */
#pragma once
#include <stddef.h>

#define HAD_PREFIJO "homeassistant"
#define HAD_EXPIRE_AFTER_S 180
#define HAD_MODELO "Waveshare ESP32-S3-DEV-KIT-N8R8 + E/S nodo-agua"

int had_n_entidades(void);

/* «homeassistant/<componente>/<nodo>/<objeto>/config» */
int had_topico(char *buf, size_t cap, const char *nodo, int i);

/* «<nodo>_<objeto>» */
int had_unique_id(char *buf, size_t cap, const char *nodo, int i);

/* JSON de la entidad i */
int had_config_json(char *buf, size_t cap, const char *sitio, const char *zona,
                    const char *nodo, const char *fw, int i);
