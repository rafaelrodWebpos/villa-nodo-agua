/* Nodo Agua — esqueleto de la Fase 1 del harness (raíz de composición).
 *
 * Prueba la cadena build → artefacto → grabación → marcador. No implementa ni
 * verifica ningún requisito del FSD: los requisitos llegan con las olas de /build.
 * El log UDP (clima §18.1, heredado) se integra desde ya: son los ojos del ciclo
 * dentro del DUT, y solo existe en la imagen de banco (C-LOG-01). */
#include "esp_app_desc.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "udplog.h"

static const char *TAG = "nodo";

#if CONFIG_NODO_BENCH_BUILD
/* Marcador de la imagen de banco: la CI comprueba que la de producción no lo lleva. */
static const char BENCH_MARKER[] = "NODO-BENCH-IMAGE";
#endif

void app_main(void)
{
    const esp_app_desc_t *app = esp_app_get_description();
    ESP_LOGI(TAG, "%s %s", app->project_name, app->version);
#if CONFIG_NODO_BENCH_BUILD
    ESP_LOGI(TAG, "%s", BENCH_MARKER);
#endif
    ESP_ERROR_CHECK(esp_netif_init());                 /* el log UDP registra eventos de IP */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(udplog_iniciar());   /* sin efecto en producción (FR-LOG-01) */
    /* Marcador de arranque del contrato del testbench: última línea de app_main. */
    ESP_LOGI(TAG, "Init complete");
}
