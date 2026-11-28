/* TC-MQTT-01 · clima:FR-MQTT-01 y TC-MQTT-02 · clima:FR-MQTT-08 (host): serializadores del
 * publicador (FSD §13; clima §9.2, §9.3). «mqtt01»: tópicos y Last Will;
 * «mqtt02»: `status` con los 9 campos, sus tipos y su orden, y eventos_perdidos (FR-PUB-05).
 * TC-PUB-A1 · FR-PUB-01/02/03 (host): «estados»: payloads de §13.1. */
#include <stdio.h>
#include <string.h>
#include "publicador.h"

static int fallos;
#define ESPERA(cond, ...) do { if (!(cond)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); puts(""); } } while (0)
#define IGUAL(obt, esp) ESPERA(strcmp((obt), (esp)) == 0, "\n  obtenido: %s\n  esperado: %s", (obt), (esp))

static void mqtt01(void)
{
    char b[128];
    int n;
    pub_topico(b, sizeof b, "sitio1", "zona1", "agua-a1b2c3", "valvula/state");
    IGUAL(b, "sitio1/zona1/agua-a1b2c3/valvula/state");
    pub_topico(b, sizeof b, "sitio1", "zona1", "agua-a1b2c3", "status");
    IGUAL(b, "sitio1/zona1/agua-a1b2c3/status");
    n = pub_topico(b, 12, "sitio1", "zona1", "agua-a1b2c3", "status");
    ESPERA(n == -1 && b[0] == '\0', "tópico sin espacio: n=%d", n);
    IGUAL(PUB_LWT_OFFLINE, "{\"estado\":\"offline\"}");
}

static void mqtt02(void)
{
    char b[256];
    /* Ejemplo de §9.3 */
    pub_status_t s = {.online = true, .fw = "0.3.0", .rssi = -61, .reset = "poweron", .uptime_s = 3600,
                      .lecturas = 60, .err_sensor = 0, .ota = "ninguno", .hora_valida = true};
    int n = pub_status_json(b, sizeof b, &s);
    IGUAL(b, "{\"estado\":\"online\",\"fw\":\"0.3.0\",\"rssi\":-61,\"reset\":\"poweron\","
             "\"uptime_s\":3600,\"lecturas\":60,\"err_sensor\":0,\"ota\":\"ninguno\",\"hora_valida\":true,\"eventos_perdidos\":0}");
    ESPERA(n == (int)strlen(b), "longitud %d", n);
    /* Valores límite y booleano falso; una versión con comillas se escapa (JSON válido) */
    s = (pub_status_t){.online = true, .fw = "0.0.0-9f8d3f8\"x", .rssi = -100, .reset = "usb",
                       .uptime_s = 4294967295u, .lecturas = 0, .err_sensor = 7, .ota = "rechazado:host",
                       .hora_valida = false};
    pub_status_json(b, sizeof b, &s);
    IGUAL(b, "{\"estado\":\"online\",\"fw\":\"0.0.0-9f8d3f8\\\"x\",\"rssi\":-100,\"reset\":\"usb\","
             "\"uptime_s\":4294967295,\"lecturas\":0,\"err_sensor\":7,\"ota\":\"rechazado:host\",\"hora_valida\":false,\"eventos_perdidos\":0}");
    /* Salida ordenada: offline */
    s.online = false;
    pub_status_json(b, sizeof b, &s);
    ESPERA(strncmp(b, "{\"estado\":\"offline\",", 20) == 0, "offline: %s", b);
    /* Campos nulos: nunca «(null)» ni campos ausentes */
    s = (pub_status_t){.online = true};
    pub_status_json(b, sizeof b, &s);
    IGUAL(b, "{\"estado\":\"online\",\"fw\":\"\",\"rssi\":0,\"reset\":\"otro\","
             "\"uptime_s\":0,\"lecturas\":0,\"err_sensor\":0,\"ota\":\"ninguno\",\"hora_valida\":false,\"eventos_perdidos\":0}");
    /* No cabe */
    n = pub_status_json(b, 40, &s);
    ESPERA(n == -1 && b[0] == '\0', "sin espacio: n=%d", n);
}

static void estados(void)
{
    char b[256];
    pub_ts_t sin = { false, 0 }, con = { true, 1800000000u };
    pub_valvula_json(b, sizeof b, "cerrada", true, "fuga", 8200, 8100, con);
    IGUAL(b, "{\"estado\":\"cerrada\",\"bloqueo_fuga\":true,\"cerrada_por\":\"fuga\",\"t_cierre_ms\":8200,\"t_apertura_ms\":8100,\"ts\":1800000000}");
    pub_valvula_json(b, sizeof b, "abierta", false, "ninguno", -1, -1, sin);
    IGUAL(b, "{\"estado\":\"abierta\",\"bloqueo_fuga\":false,\"cerrada_por\":\"ninguno\"}");
    pub_bomba_json(b, sizeof b, "cortada", "seco", 1, true, sin);
    IGUAL(b, "{\"permiso\":\"cortada\",\"motivo\":\"seco\",\"intentos_24h\":1,\"calibrada\":true}");
    pub_bomba_json(b, sizeof b, "permitida", NULL, 0, false, con);
    IGUAL(b, "{\"permiso\":\"permitida\",\"intentos_24h\":0,\"calibrada\":false,\"ts\":1800000000}");
    pub_estado_json(b, sizeof b, "mojada", sin);
    IGUAL(b, "{\"estado\":\"mojada\"}");
    pub_medida_json(b, sizeof b, 250.0f, "cm", true, 62.5f, true, true, sin);
    IGUAL(b, "{\"v\":250.00,\"u\":\"cm\",\"pct\":62.5,\"valido\":true}");
    pub_medida_json(b, sizeof b, 0, "cm", true, 0, true, false, sin);
    IGUAL(b, "{\"u\":\"cm\",\"valido\":false}");
    pub_medida_json(b, sizeof b, 4.1f, "A", false, 0, false, false, sin);
    IGUAL(b, "{\"v\":4.10,\"u\":\"A\"}");
    pub_bateria_json(b, sizeof b, 12.9f, "ok", sin);
    IGUAL(b, "{\"v\":12.90,\"u\":\"V\",\"estado\":\"ok\"}");
    ESPERA(pub_valvula_json(b, 20, "cerrada", true, "fuga", 1, 1, con) == -1 && b[0] == 0, "valvula que no cabe");
}

int main(int argc, char **argv)
{
    const char *caso = argc > 1 ? argv[1] : "";
    if (!strcmp(caso, "mqtt01") || !*caso) mqtt01();
    if (!strcmp(caso, "mqtt02") || !*caso) mqtt02();
    if (!strcmp(caso, "estados") || !*caso) estados();
    printf("%s: %s\n", *caso ? caso : "publicador", fallos ? "FALLO" : "ok");
    return fallos != 0;
}
