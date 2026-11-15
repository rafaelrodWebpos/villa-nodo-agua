/* L1 es — parte con hardware (FSD §2.2 C-HW-22…30, §12). */
#include "es_hw.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_ads[ES_N_ADS];
static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;

static const int SALIDAS[] = { ES_GPIO_IN1, ES_GPIO_IN2, ES_GPIO_RELE, ES_GPIO_ZUMBADOR, ES_GPIO_LED_VERDE, ES_GPIO_LED_ROJO };

void es_hw_gpio_iniciar(void)
{
    for (size_t i = 0; i < sizeof SALIDAS / sizeof SALIDAS[0]; i++) {
        gpio_set_level(SALIDAS[i], 0);
        gpio_config_t c = { .pin_bit_mask = 1ULL << SALIDAS[i], .mode = GPIO_MODE_OUTPUT };
        gpio_config(&c);
        gpio_set_level(SALIDAS[i], 0);
    }
    gpio_config_t pu = { .pin_bit_mask = (1ULL << ES_GPIO_FIN_ABIERTA) | (1ULL << ES_GPIO_FIN_CERRADA) |
                                         (1ULL << ES_GPIO_FLOTADOR) | (1ULL << ES_GPIO_BOTON),
                         .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE };
    gpio_config(&pu);
    /* Red presente: sin señal cuenta como red ausente */
    gpio_config_t pd = { .pin_bit_mask = 1ULL << ES_GPIO_RED, .mode = GPIO_MODE_INPUT,
                         .pull_down_en = GPIO_PULLDOWN_ENABLE };
    gpio_config(&pd);
}

void es_hw_i2c_iniciar(char *ack, int cap)
{
    i2c_master_bus_config_t bc = { .i2c_port = -1, .sda_io_num = ES_GPIO_SDA, .scl_io_num = ES_GPIO_SCL,
                                   .clk_source = I2C_CLK_SRC_DEFAULT, .glitch_ignore_cnt = 7,
                                   .flags.enable_internal_pullup = true };
    int n = 0;
    if (cap > 0) ack[0] = '\0';
    if (i2c_new_master_bus(&bc, &s_bus) != ESP_OK) return;
    const uint8_t dir[ES_N_ADS] = ES_ADS_DIRS;
    for (int k = 0; k < ES_N_ADS; k++) {
        i2c_device_config_t dc = { .dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = dir[k],
                                   .scl_speed_hz = ES_I2C_HZ };
        i2c_master_bus_add_device(s_bus, &dc, &s_ads[k]);
        if (i2c_master_probe(s_bus, dir[k], 20) == ESP_OK && n < cap)
            n += snprintf(ack + n, (size_t)(cap - n), "%s0x%02x", n ? "," : "", dir[k]);
    }
}

bool es_hw_ads_leer(int k, int canal, int16_t *cuentas)
{
    if (k < 0 || k >= ES_N_ADS || !s_ads[k]) return false;
    uint16_t cfg = es_ads_config(canal);
    uint8_t w[3] = { 0x01, (uint8_t)(cfg >> 8), (uint8_t)cfg };
    if (i2c_master_transmit(s_ads[k], w, sizeof w, 20) != ESP_OK) return false;
    vTaskDelay(pdMS_TO_TICKS(2));                       /* 860 muestras/s: 1,2 ms por conversión */
    uint8_t reg = 0x00, r[2];
    if (i2c_master_transmit_receive(s_ads[k], &reg, 1, r, 2, 20) != ESP_OK) return false;
    *cuentas = (int16_t)((r[0] << 8) | r[1]);
    return true;
}

bool es_hw_pinza_iniciar(void)
{
    adc_oneshot_unit_init_cfg_t uc = { .unit_id = ADC_UNIT_1 };
    if (adc_oneshot_new_unit(&uc, &s_adc) != ESP_OK) return false;
    adc_oneshot_chan_cfg_t cc = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    if (adc_oneshot_config_channel(s_adc, ADC_CHANNEL_3, &cc) != ESP_OK) return false;
    adc_cali_curve_fitting_config_t kc = { .unit_id = ADC_UNIT_1, .chan = ADC_CHANNEL_3,
                                           .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    return adc_cali_create_scheme_curve_fitting(&kc, &s_cali) == ESP_OK;
}

bool es_hw_pinza_muestras(int16_t *mv, int n)
{
    if (!s_adc || !s_cali) return false;
    for (int i = 0; i < n; i++) {
        int raw = 0, v = 0;
        if (adc_oneshot_read(s_adc, ADC_CHANNEL_3, &raw) != ESP_OK) return false;
        adc_cali_raw_to_voltage(s_cali, raw, &v);
        mv[i] = (int16_t)v;
        esp_rom_delay_us(ES_RMS_PERIODO_US);
    }
    return true;
}

es_hw_digitales_t es_hw_digitales(void)
{
    return (es_hw_digitales_t){
        .fin_abierta = gpio_get_level(ES_GPIO_FIN_ABIERTA) == 0,
        .fin_cerrada = gpio_get_level(ES_GPIO_FIN_CERRADA) == 0,
        .flotador_agua = gpio_get_level(ES_GPIO_FLOTADOR) == 0,
        .red_presente = gpio_get_level(ES_GPIO_RED) == 1,
        .boton = gpio_get_level(ES_GPIO_BOTON) == 0,
    };
}

void es_hw_salidas(bool in1, bool in2, bool rele, bool led_verde, bool led_rojo, bool zumbador)
{
    if (in1 && in2) in1 = in2 = false;
    gpio_set_level(ES_GPIO_IN1, in1);
    gpio_set_level(ES_GPIO_IN2, in2);
    gpio_set_level(ES_GPIO_RELE, rele);
    gpio_set_level(ES_GPIO_LED_VERDE, led_verde);
    gpio_set_level(ES_GPIO_LED_ROJO, led_rojo);
    gpio_set_level(ES_GPIO_ZUMBADOR, zumbador);
}
