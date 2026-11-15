/* Adaptador de la protección (FSD §5–§9, §12): tareas, NVS y log. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool (*hora)(int *hora_local, uint32_t *ahora_s);   /* hora válida (clima:FR-TIME-02) */
    bool (*conectado)(void);                            /* OPERATIVO (LED, prioridad 4) */
    volatile bool *wdt_bloquear;                        /* E13 wdt-test (NFR-WDT-A1) */
} proteccion_ganchos_t;

/* Tras nvs_flash_init: restaura los bloqueos (FR-PER-02), escanea el I²C (A7) y arranca
 * las tareas de medidas, de la pinza y de protección (esta, suscrita al TWDT). */
void proteccion_iniciar(const proteccion_ganchos_t *g);
