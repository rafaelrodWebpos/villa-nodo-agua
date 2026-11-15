/* TC-HA-01 · clima:FR-HA-02, clima:FR-HA-03, FR-HA-A1 (host): generador de
 * Discovery (FSD §14). Contrato: nodo agua-a1b2c3, sitio sitio1, zona zona1,
 * fw 0.3.0 → una configuración por entidad de §14 (menos el % de batería,
 * pendiente de definir), JSON válido con unique_id = <nodo>_<objeto> sin sitio
 * ni zona y único, sw_version = fw, expire_after = 180 solo en las medidas
 * periódicas, y los tópicos de §13.1. */
#include <stdio.h>
#include <string.h>
#include "ha_discovery.h"

static int fallos;
#define ESPERA(cond, ...) do { if (!(cond)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); puts(""); } } while (0)
#define IGUAL(obt, esp) ESPERA(strcmp((obt), (esp)) == 0, "\n  obtenido: %s\n  esperado: %s", (obt), (esp))

/* Entidades de §14: tópico de Discovery esperado */
static const char *const TOPICOS[] = {
    "homeassistant/valve/agua-a1b2c3/valvula/config",
    "homeassistant/switch/agua-a1b2c3/bomba/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_1/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_2/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_3/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_4/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_5/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_6/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_7/config",
    "homeassistant/binary_sensor/agua-a1b2c3/fuga_8/config",
    "homeassistant/binary_sensor/agua-a1b2c3/flotador/config",
    "homeassistant/binary_sensor/agua-a1b2c3/red/config",
    "homeassistant/sensor/agua-a1b2c3/nivel/config",
    "homeassistant/sensor/agua-a1b2c3/nivel_pct/config",
    "homeassistant/sensor/agua-a1b2c3/presion/config",
    "homeassistant/sensor/agua-a1b2c3/corriente_bomba/config",
    "homeassistant/sensor/agua-a1b2c3/vbat/config",
    "homeassistant/button/agua-a1b2c3/rearmar_bomba/config",
    "homeassistant/button/agua-a1b2c3/ejercicio/config",
    "homeassistant/event/agua-a1b2c3/evento/config",
};
#define N (int)(sizeof TOPICOS / sizeof TOPICOS[0])

/* La configuración de la válvula, literal */
static const char *const VALVULA =
    "{\"~\":\"sitio1/zona1/agua-a1b2c3\",\"name\":\"Válvula general\",\"unique_id\":\"agua-a1b2c3_valvula\","
    "\"command_topic\":\"~/valvula/set\",\"state_topic\":\"~/valvula/state\","
    "\"payload_open\":\"{\\\"accion\\\":\\\"abrir\\\",\\\"motivo\\\":\\\"manual\\\"}\","
    "\"payload_close\":\"{\\\"accion\\\":\\\"cerrar\\\",\\\"motivo\\\":\\\"manual\\\"}\","
    "\"value_template\":\"{{ {'abierta':'open','cerrada':'closed','abriendo':'opening','cerrando':'closing'}"
    ".get(value_json.estado, 'None') }}\",\"device_class\":\"water\",\"qos\":1,"
    "\"availability_topic\":\"~/status\",\"availability_template\":\"{{ value_json.estado }}\","
    "\"payload_available\":\"online\",\"payload_not_available\":\"offline\","
    "\"device\":{\"identifiers\":[\"agua-a1b2c3\"],\"name\":\"Agua sitio1 zona1\","
    "\"model\":\"Waveshare ESP32-S3-DEV-KIT-N8R8 + E/S nodo-agua\",\"sw_version\":\"0.3.0\"}}";

/* JSON bien formado: llaves y corchetes equilibrados fuera de las cadenas, escapes válidos */
static int json_ok(const char *s)
{
    int prof = 0, cad = 0;
    for (; *s; s++) {
        if (cad) {
            if (*s == '\\') { s++; if (!*s) return 0; }
            else if (*s == '"') cad = 0;
            else if ((unsigned char)*s < 0x20) return 0;
        } else if (*s == '"') cad = 1;
        else if (*s == '{' || *s == '[') prof++;
        else if (*s == '}' || *s == ']') { if (--prof < 0) return 0; }
    }
    return prof == 0 && !cad;
}

static void ha01(void)
{
    static char b[2048], t[128], u[64], ids[32][64];
    ESPERA(had_n_entidades() == N, "entidades: %d, esperadas %d", had_n_entidades(), N);
    for (int i = 0; i < had_n_entidades() && i < N; i++) {
        int n = had_config_json(b, sizeof b, "sitio1", "zona1", "agua-a1b2c3", "0.3.0", i);
        ESPERA(n > 0 && n == (int)strlen(b), "entidad %d: n=%d", i, n);
        ESPERA(json_ok(b), "entidad %d: JSON mal formado: %s", i, b);
        had_topico(t, sizeof t, "agua-a1b2c3", i);
        IGUAL(t, TOPICOS[i]);
        had_unique_id(u, sizeof u, "agua-a1b2c3", i);
        char k[96];
        snprintf(k, sizeof k, "\"unique_id\":\"%s\"", u);
        ESPERA(strstr(b, k) != NULL, "entidad %d sin %s", i, k);
        ESPERA(strncmp(u, "agua-a1b2c3_", 12) == 0 && !strstr(u, "sitio1") && !strstr(u, "zona1"), "unique_id %s", u);
        for (int j = 0; j < i; j++) ESPERA(strcmp(ids[j], u) != 0, "unique_id repetido: %s", u);
        snprintf(ids[i], sizeof ids[i], "%s", u);
        ESPERA(strstr(b, "\"sw_version\":\"0.3.0\"") != NULL, "entidad %d sin sw_version", i);
        int medida = strstr(t, "homeassistant/sensor/") == t;
        ESPERA(medida == (strstr(b, "\"expire_after\":180") != NULL), "entidad %d: expire_after mal puesto", i);
    }
    had_config_json(b, sizeof b, "sitio1", "zona1", "agua-a1b2c3", "0.3.0", 0);
    IGUAL(b, VALVULA);
    /* Sin espacio o entidad inexistente: -1 y búfer vacío */
    int n = had_config_json(b, 100, "sitio1", "zona1", "agua-a1b2c3", "0.3.0", 0);
    ESPERA(n == -1 && b[0] == '\0', "sin espacio: n=%d", n);
    n = had_topico(t, sizeof t, "agua-a1b2c3", N);
    ESPERA(n == -1 && t[0] == '\0', "entidad inexistente: n=%d", n);
    /* Escape de sitio y zona con comillas */
    had_config_json(b, sizeof b, "si\"t", "zo\\na", "agua-a1b2c3", "0.3.0", 2);
    ESPERA(json_ok(b) && strstr(b, "\"~\":\"si\\\"t/zo\\\\na/agua-a1b2c3\""), "escape: %s", b);
}

int main(void)
{
    ha01();
    if (fallos) { printf("%d fallo(s)\n", fallos); return 1; }
    puts("TC-HA-01 ok");
    return 0;
}
