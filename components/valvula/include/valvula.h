/* L2 valvula — máquina de estados de la válvula general (FSD §5.2–§5.3, V01–V20).
 * Núcleo puro, sin ESP-IDF; lo compila tests/host. La tarea de protección le pasa un
 * evento cada vez y aplica la acción que devuelve (motor, rechazo, evento publicado). */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum { VAL_ARRANQUE, VAL_DESCONOCIDA, VAL_ABIERTA, VAL_CERRADA, VAL_CERRANDO,
               VAL_ABRIENDO, VAL_FALLA, VAL_N_ESTADOS } val_estado_t;

typedef enum { VCP_NINGUNO, VCP_FUGA, VCP_ORDEN, VCP_BATERIA, VCP_EJERCICIO } val_cerrada_por_t;

typedef enum {
    VEV_FINALES,           /* lectura (con antirrebote) de los finales; en ARRANQUE, «fin de lectura».
                            * En CERRADA, una lectura con solo «cerrada» es la confirmación de V14;
                            * en los demás casos, una lectura sin cambio es fila 0 y no hace falta
                            * registrarla. */
    VEV_FUGA,              /* fuga confirmada (FR-FUG-01) */
    VEV_CERRAR,            /* orden `cerrar` válida */
    VEV_ABRIR,             /* orden `abrir` (válida salvo por la guarda g_seco) */
    VEV_PULSACION_LARGA,   /* pulsación ≥ 3 s (FR-LOC-03) */
    VEV_EJERCICIO,         /* ejercicio vencido (FR-EJE-01) */
    VEV_TICK,              /* paso del tiempo: vigila el tiempo de carrera (P3) */
    VEV_N_EVENTOS
} val_evento_t;

#define VFIN_ABIERTA 1u    /* bit del final «abierta» */
#define VFIN_CERRADA 2u    /* bit del final «cerrada» */

typedef struct {
    val_evento_t evento;
    uint8_t finales;       /* VFIN_* activos en este instante */
    bool g_seco;           /* ninguna sonda activa en `mojada` (§5.2, P13) */
    uint32_t t_ms;         /* reloj monótono */
} val_entrada_t;

typedef enum { VMOT_SIN_CAMBIO, VMOT_PARAR, VMOT_CERRAR, VMOT_ABRIR } val_motor_t;   /* nunca IN1 e IN2 a la vez (FR-VAL-11) */
typedef enum { VRECH_NINGUNO, VRECH_FUGA_ACTIVA, VRECH_OCUPADO } val_rechazo_t;
typedef enum { VPUB_NINGUNO, VPUB_FALLA_VALVULA, VPUB_MOVIMIENTO_MANUAL } val_publicar_t;

typedef struct {
    int fila;              /* 1…20 = V01…V20; 0 = par ignorado (`val: ignorado <estado> <evento>`) */
    val_motor_t motor;
    val_rechazo_t rechazo;
    val_publicar_t evento;
    bool patron_rechazo;   /* §8.2: pulsación larga rechazada (V11) */
    bool publicar_estado;  /* V03 */
    uint32_t t_carrera_ms; /* V06/V12/V14: duración del movimiento que acaba de terminar */
} val_accion_t;

typedef struct {
    val_estado_t estado;
    bool bloqueo_fuga;             /* persistente (§9) */
    val_cerrada_por_t cerrada_por; /* persistente (§9) */
    uint32_t t_inicio_ms;          /* inicio del movimiento en curso */
    uint32_t t_carrera_max_ms;     /* P3: t_carrera_max_s × 1000 */
} val_t;

/* Estado inicial (ARRANQUE) con las marcas restauradas de NVS */
void val_iniciar(val_t *v, bool bloqueo_fuga, val_cerrada_por_t cerrada_por, uint32_t t_carrera_max_s);

/* Aplica un evento y devuelve la acción. Un par (estado × evento) no listado en §5.3
 * no cambia nada y devuelve fila 0 (completitud). */
val_accion_t val_evento(val_t *v, const val_entrada_t *e);

const char *val_nombre_estado(val_estado_t s);
const char *val_nombre_evento(val_evento_t e);

/* FR-EJE-01 (D12): ¿toca el ejercicio? Solo con la hora válida (clima:FR-TIME-02), en la
 * hora local `ejercicio_hora`, con `ejercicio_dias` cumplidos desde el último movimiento
 * completo y con las guardas de V13 (g_seco y bloqueo_fuga = 0). */
typedef struct {
    bool hora_valida;
    int hora_local;            /* 0–23 */
    uint32_t ahora_s;          /* hora UTC en s */
    uint32_t ultimo_mov_s;     /* fin del último movimiento completo */
    int ejercicio_dias;
    int ejercicio_hora;
    bool g_seco;
    bool bloqueo_fuga;
} eje_entrada_t;
bool eje_vencido(const eje_entrada_t *e);
