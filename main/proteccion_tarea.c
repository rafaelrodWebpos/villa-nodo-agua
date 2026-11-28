/* Adaptador de la protección: lee los drivers (es_hw), llama al núcleo (proteccion) cada
 * 10 ms y aplica sus salidas. Sin llamadas de red (§2.3): los eventos solo se registran
 * en el log hasta que exista el publicador de §13 (Fase 2). */
#include "proteccion_tarea.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs.h"
#include "es_hw.h"
#include "proteccion.h"

static const char *TAG = "nodo";

#define MEDIDAS_MS    250u      /* FR-FUG-01: una muestra de cada sonda cada 250 ms */
#define PINZA_MS      1000u
#define ANA_MS        60000u    /* A4: cada ciclo de medidas (FR-PUB-03) */
#define ADS_REINTENTO_MS 5000u  /* un ADS caído se vuelve a probar cada 5 s */
#define NVS_ESPACIO   "proteccion"
#define NVS_CLAVE     "estado"

static proteccion_ganchos_t s_g;
static prot_t s_prot;
static SemaphoreHandle_t s_mux;
static struct { uint32_t seq; bool ads_ok[ES_N_ADS]; float v[12]; } s_med;   /* 8 sondas + nivel, presión, Vbat, referencia */
static struct { uint32_t seq; float i_rms; } s_pin;
static uint32_t s_escrituras;

static uint32_t ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
static uint32_t seg(void) { return (uint32_t)(esp_timer_get_time() / 1000000); }

/* ---- NVS (FR-PER-01/02) ---- */
typedef struct __attribute__((packed)) { uint8_t ver, bloqueo, cerrada_por, bomba, motivo, n_cortes, rearme; } nvs_blob_t;
#define NVS_BLOB_V1 6   /* tamaño de la versión 1, sin `rearme` */

static prot_persistente_t nvs_leer(void)
{
    prot_persistente_t p = { false, VCP_NINGUNO, BOM_PERMITIDA, BMOT_NINGUNO, 0, false };
    nvs_handle_t h;
    nvs_blob_t b;
    size_t n = sizeof b;
    if (nvs_open(NVS_ESPACIO, NVS_READONLY, &h) != ESP_OK) return p;
    if (nvs_get_blob(h, NVS_CLAVE, &b, &n) == ESP_OK &&
        ((n == sizeof b && b.ver == 2) || (n == NVS_BLOB_V1 && b.ver == 1)))
        p = (prot_persistente_t){ b.bloqueo != 0, (val_cerrada_por_t)b.cerrada_por, (bom_estado_t)b.bomba,
                                  (bom_motivo_t)b.motivo, b.n_cortes, b.ver == 2 && b.rearme != 0 };
    nvs_close(h);
    return p;
}

static void nvs_escribir(const prot_persistente_t *p)
{
    nvs_blob_t b = { 2, p->bloqueo_fuga, (uint8_t)p->cerrada_por, (uint8_t)p->bomba, (uint8_t)p->motivo,
                     (uint8_t)p->n_cortes, p->rearme_reciente };
    nvs_handle_t h;
    if (nvs_open(NVS_ESPACIO, NVS_READWRITE, &h) != ESP_OK) return;
    if (nvs_set_blob(h, NVS_CLAVE, &b, sizeof b) == ESP_OK && nvs_commit(h) == ESP_OK)
        ESP_LOGI(TAG, "nvs: bloqueos escritos (%lu)", (unsigned long)++s_escrituras);   /* A5 */
    nvs_close(h);
}

/* ---- Tarea de medidas: 3 ADS1115 cada 250 ms ---- */
static void tarea_medidas(void *arg)
{
    (void)arg;
    bool ok[ES_N_ADS] = { true, true, true };
    uint32_t reintento[ES_N_ADS] = { 0 };
    TickType_t ultimo = xTaskGetTickCount();
    for (;;) {
        float v[12] = { 0 };
        uint32_t t = ms();
        for (int k = 0; k < ES_N_ADS; k++) {
            if (!ok[k] && t - reintento[k] < ADS_REINTENTO_MS) continue;
            bool bien = true;
            const int canales = 4;
            for (int c = 0; c < canales && bien; c++) {
                int16_t cu;
                bien = es_hw_ads_leer(k, c, &cu);
                if (bien) v[k < 2 ? k * 4 + c : 8 + c] = es_ads_voltios(cu);
            }
            ok[k] = bien;
            if (!bien) reintento[k] = t;
        }
        xSemaphoreTake(s_mux, portMAX_DELAY);
        memcpy(s_med.ads_ok, ok, sizeof ok);
        memcpy(s_med.v, v, sizeof v);
        s_med.seq++;
        xSemaphoreGive(s_mux);
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(MEDIDAS_MS));
    }
}

/* ---- Tarea de la pinza: RMS de 10 ciclos cada segundo (FR-IO-04) ---- */
static void tarea_pinza(void *arg)
{
    (void)arg;
    static int16_t mv[ES_RMS_MUESTRAS];
    bool hay = es_hw_pinza_iniciar();
    if (!hay) ESP_LOGW(TAG, "pinza: ADC1 sin calibrar; corriente = 0");
    for (;;) {
        float i = (hay && es_hw_pinza_muestras(mv, ES_RMS_MUESTRAS)) ? es_rms_amperios(mv, ES_RMS_MUESTRAS) : 0.0f;
        xSemaphoreTake(s_mux, portMAX_DELAY);
        s_pin.i_rms = i;
        s_pin.seq++;
        xSemaphoreGive(s_mux);
        vTaskDelay(pdMS_TO_TICKS(PINZA_MS));
    }
}

/* ---- Log de los eventos (A1–A3 y avisos) ---- */
static const char *NOMBRE_BOM[] = { "PERMITIDA", "CORTADA", "BLOQUEADA" };
static const char *EV_BOM[] = { "seco", "flotador_sin_agua", "nivel", "cortar", "rearme", "rearmar", "permitir", "pulsacion" };
static const char *NOMBRE_BAT[] = { "ok", "baja", "critica" };
static const char *NOMBRE_CP[] = { "ninguno", "fuga", "orden", "bateria", "ejercicio", "sondas" };

static void registrar(const prot_evento_t *e)
{
    switch (e->tipo) {
    case PEV_VAL:
        ESP_LOGI(TAG, "val: %s -> %s (%s)", val_nombre_estado(e->b), val_nombre_estado(e->c), val_nombre_evento(e->d));
        break;
    case PEV_BOM:
        ESP_LOGI(TAG, "bom: %s -> %s (%s)", NOMBRE_BOM[e->b], NOMBRE_BOM[e->c], EV_BOM[e->d]);
        break;
    case PEV_SON:
        ESP_LOGI(TAG, "son: s%d %s", e->a, e->b < 0 ? "falla" : son_nombre(e->b));
        break;
    case PEV_FUGA: ESP_LOGW(TAG, "evento: fuga s%d", e->a); break;
    case PEV_FALLA_SONDA: ESP_LOGW(TAG, "evento: falla_sonda s%d %s", e->a, e->b < 0 ? "falla" : son_nombre(e->b)); break;
    case PEV_FALLA_VALVULA: ESP_LOGW(TAG, "evento: falla_valvula"); break;
    case PEV_MOVIMIENTO_MANUAL: ESP_LOGW(TAG, "evento: movimiento_manual"); break;
    case PEV_BOMBA_BLOQUEADA: ESP_LOGW(TAG, "evento: bomba_bloqueada"); break;
    case PEV_CICLADO: ESP_LOGW(TAG, "evento: ciclado_anormal n=%d valvula=%s", e->a, e->b ? "abierta" : "cerrada"); break;
    case PEV_BATERIA: ESP_LOGW(TAG, "evento: bateria_%s", NOMBRE_BAT[e->a]); break;
    case PEV_FALLA_ADC: ESP_LOGW(TAG, "evento: falla_adc 0x%02x", 0x48 + e->a); break;
    case PEV_RECHAZO:
        ESP_LOGW(TAG, "evento: pulsacion_larga rechazada (%s)",
                 e->a == 1 ? "fuga_activa" : e->a == 3 ? "sondas_en_falla" : e->a == 4 ? "nivel_bajo" :
                 e->a == 5 ? "bateria_critica" : e->a == 6 ? "estado" : "sin_agua");
        break;
    case PEV_FLOTADOR_FALLA:
        if (e->a) ESP_LOGW(TAG, "evento: flotador_falla"); else ESP_LOGI(TAG, "bom: flotador sin falla");
        break;
    case PEV_VREF: ESP_LOGW(TAG, "son: vref %s", e->a ? "nominal" : "medida"); break;
    case PEV_PORTAL: ESP_LOGW(TAG, "loc: reentrada_portal"); break;                    /* A10 */
    case PEV_ESTADO:                                                                     /* A11 */
        ESP_LOGI(TAG, "est: cerrada_por=%s bloqueo_fuga=%d intentos=%d", NOMBRE_CP[e->a], e->b, e->c);
        break;
    case PEV_FALLA_VREF:
        if (e->a) ESP_LOGW(TAG, "evento: falla_vref"); else ESP_LOGI(TAG, "son: vref sin falla");
        break;
    }
}

static void log_ana(const prot_t *p)   /* A4 */
{
    char niv[16], pre[16];
    if (p->nivel_valido) snprintf(niv, sizeof niv, "%.0f", p->nivel_cm); else strcpy(niv, "falla");
    if (p->presion_valida) snprintf(pre, sizeof pre, "%.2f", p->presion_bar); else strcpy(pre, "falla");
    ESP_LOGI(TAG, "ana: niv=%s p=%s i=%.2f vbat=%.2f red=%d", niv, pre, p->i_rms, p->vbat, p->red_presente);
}

/* ---- Tarea de protección: cada 10 ms ---- */
static void tarea_proteccion(void *arg)
{
    (void)arg;
    esp_task_wdt_add(NULL);                                    /* NFR-WDT-A1 */
    uint32_t seq_med = 0, seq_pin = 0, ultimo_ana = ms();
    static prot_entrada_t e;
    static prot_salida_t s;
    TickType_t ultimo = xTaskGetTickCount();
    for (;;) {
        memset(&e, 0, sizeof e);
        e.t_ms = ms();
        e.t_s = seg();
        es_hw_digitales_t d = es_hw_digitales();
        e.fin_abierta = d.fin_abierta; e.fin_cerrada = d.fin_cerrada; e.flotador_agua = d.flotador_agua;
        e.red_presente = d.red_presente; e.boton = d.boton;
        xSemaphoreTake(s_mux, portMAX_DELAY);
        if (s_med.seq != seq_med) {
            seq_med = s_med.seq;
            e.hay_medidas = true;
            memcpy(e.ads_ok, s_med.ads_ok, sizeof e.ads_ok);
            memcpy(e.v_sonda, s_med.v, sizeof e.v_sonda);
            e.v_nivel = s_med.v[8]; e.v_presion = s_med.v[9]; e.v_bat = s_med.v[10]; e.v_ref = s_med.v[11];
        }
        if (s_pin.seq != seq_pin) { seq_pin = s_pin.seq; e.hay_corriente = true; e.i_rms = s_pin.i_rms; }
        xSemaphoreGive(s_mux);
        if (e.hay_medidas) e.hora_valida = s_g.hora && s_g.hora(&e.hora_local, &e.ahora_s);
        e.conectado = s_g.conectado && s_g.conectado();
        prot_paso(&s_prot, &e, &s);
        es_hw_salidas(s.in1, s.in2, s.rele, s.led_verde, s.led_rojo, s.zumbador);
        for (int i = 0; i < s.n_ev; i++) registrar(&s.ev[i]);
        if (s.persistir) { prot_persistente_t p = prot_persistente(&s_prot); nvs_escribir(&p); }
        for (int i = 0; i < s.n_ev; i++)                       /* FR-LOC-06, tras guardar FR-PER-01 */
            if (s.ev[i].tipo == PEV_PORTAL && s_g.reentrada_portal) s_g.reentrada_portal();
        if (e.t_ms - ultimo_ana >= ANA_MS) { ultimo_ana = e.t_ms; log_ana(&s_prot); }
        if (!(s_g.wdt_bloquear && *s_g.wdt_bloquear)) esp_task_wdt_reset();
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PROT_PERIODO_MS));
    }
}

void proteccion_iniciar(const proteccion_ganchos_t *g)
{
    s_g = *g;
    s_mux = xSemaphoreCreateMutex();
    prot_config_t cfg;
    prot_config_fabrica(&cfg);
    prot_persistente_t nvs = nvs_leer();                       /* FR-PER-02 */
    prot_iniciar(&s_prot, &cfg, &nvs, seg());
    es_hw_salidas(false, false, prot_persistente(&s_prot).bomba != BOM_PERMITIDA, false, false, false);   /* FR-BOM-02 */
    esp_log_level_set("i2c.master", ESP_LOG_NONE);             /* un ADS ausente se informa con falla_adc */
    char ack[32];
    es_hw_i2c_iniciar(ack, sizeof ack);
    ESP_LOGI(TAG, "i2c: scan sda=%d scl=%d hz=%d ack=%s", ES_GPIO_SDA, ES_GPIO_SCL, ES_I2C_HZ, ack);   /* A7 */
    xTaskCreate(tarea_medidas, "medidas", 3072, NULL, 5, NULL);
    xTaskCreate(tarea_pinza, "pinza", 3072, NULL, 2, NULL);
    xTaskCreate(tarea_proteccion, "proteccion", 4096, NULL, 7, NULL);
}
