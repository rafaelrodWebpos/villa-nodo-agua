idf_component_register(SRCS "es.c" "es_hw.c" INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_driver_gpio esp_driver_i2c esp_adc esp_rom freertos)
