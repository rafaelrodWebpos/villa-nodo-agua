/* Adaptador de la protección (FSD §5–§9, §12): tareas, NVS y log. */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool (*hora)(int *hora_local, uint32_t *ahora_s);   /* hora válida (clima:FR-TIME-02) */
    bool (*conectado)(void);                            /* OPERATIVO (LED, prioridad 4) */
    volatile bool *wdt_bloquear;                        /* E13 wdt-test (NFR-WDT-A1) */
    void (*reentrada_portal)(void);                     /* FR-LOC-06: borra Wi-Fi/MQTT/zona/sitio y reinicia */
} proteccion_ganchos_t;

/* Tras nvs_flash_init: restaura los bloqueos (FR-PER-02), escanea el I²C (A7) y arranca
 * las tareas de medidas, de la pinza y de protección (esta, suscrita al TWDT). */
void proteccion_iniciar(const proteccion_ganchos_t *g);

/* FR-MED-03: guarda ya el total y los pulsos del medidor (antes de un reinicio ordenado) */
void medidor_guardar_ya(void);

#include "cola_eventos.h"
#include "medidor.h"
/* Eventos MQTT de la protección y del medidor (§13.1) para el publicador; false si no hay */
bool proteccion_evento(cev_t *ev, uint32_t espera_ms);

/* Foto del estado para los tópicos de §13.1 (FR-PUB-01/02/03) */
typedef struct {
    int val_estado; bool bloqueo_fuga; int cerrada_por; int32_t t_cierre_ms, t_apertura_ms;
    int bom_estado, bom_motivo, intentos; bool calibrada;
    int sonda[8];                      /* son_clase_t; -1 ADS en falla; -2 sin lectura; -3 inactiva */
    bool flotador_agua, red_presente;
    bool bateria_conocida; int bateria; float vbat;
    bool nivel_valido; float nivel_cm, nivel_pct;
    bool presion_valida; float presion_bar;
    float i_rms;
} prot_foto_t;
void proteccion_foto(prot_foto_t *f);

/* Órdenes de valvula/set y bomba/set (PORD_*): se aplican en el siguiente paso de 10 ms */
bool proteccion_orden(int pord);
typedef struct { bool bomba; int fila; const char *rechazo; } prot_resp_t;
bool proteccion_respuesta(prot_resp_t *r, uint32_t espera_ms);
/* Lectura para el tópico `medidor` (FR-MED-04) */
void medidor_lectura(med_lectura_t *l);
/* `permitir_consumo` (FR-MED-06/10) con el reloj y el margen del nodo */
med_resultado_t medidor_permitir(int min, int *hasta_min);
