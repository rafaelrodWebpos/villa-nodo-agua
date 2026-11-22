/* L2 valvula — V01–V25 (FSD §5.3, 0.7.1). Núcleo puro. */
#include "valvula.h"

void val_iniciar(val_t *v, bool bloqueo_fuga, val_cerrada_por_t cerrada_por, uint32_t t_carrera_max_s)
{
    *v = (val_t){ .estado = VAL_ARRANQUE, .bloqueo_fuga = bloqueo_fuga, .cerrada_por = cerrada_por,
                  .t_carrera_max_ms = t_carrera_max_s * 1000u };
}

static val_accion_t mover(val_t *v, const val_entrada_t *e, int fila, val_estado_t hacia)
{
    v->estado = hacia;
    v->t_inicio_ms = e->t_ms;
    return (val_accion_t){ .fila = fila, .motor = hacia == VAL_CERRANDO ? VMOT_CERRAR : VMOT_ABRIR };
}

static val_accion_t llegar(val_t *v, const val_entrada_t *e, int fila, val_estado_t hacia)
{
    v->estado = hacia;
    return (val_accion_t){ .fila = fila, .motor = VMOT_PARAR, .t_carrera_ms = e->t_ms - v->t_inicio_ms };
}

/* V08/V10: abrir con g_seco limpia las marcas */
static val_accion_t abrir(val_t *v, const val_entrada_t *e, int fila)
{
    v->bloqueo_fuga = false;
    v->cerrada_por = VCP_NINGUNO;
    return mover(v, e, fila, VAL_ABRIENDO);
}

static val_estado_t segun_finales(uint8_t f)
{
    return f == VFIN_ABIERTA ? VAL_ABIERTA : f == VFIN_CERRADA ? VAL_CERRADA
         : f == (VFIN_ABIERTA | VFIN_CERRADA) ? VAL_FALLA : VAL_DESCONOCIDA;
}

/* V15: un final cambia sin orden */
static val_accion_t manual(val_t *v, uint8_t f)
{
    v->estado = segun_finales(f);
    return (val_accion_t){ .fila = 15,
                           .evento = v->estado == VAL_FALLA ? VPUB_FALLA_VALVULA : VPUB_MOVIMIENTO_MANUAL };
}

val_accion_t val_evento(val_t *v, const val_entrada_t *e)
{
    const val_accion_t ignorar = { .fila = 0 };
    const uint8_t f = e->finales & (VFIN_ABIERTA | VFIN_CERRADA);
    switch (v->estado) {
    case VAL_ARRANQUE:
        if (e->evento != VEV_FINALES) return ignorar;
        if (f == VFIN_ABIERTA) { v->estado = VAL_ABIERTA; return (val_accion_t){ .fila = 1 }; }
        if (f == VFIN_CERRADA) { v->estado = VAL_CERRADA; return (val_accion_t){ .fila = 2 }; }
        if (f == (VFIN_ABIERTA | VFIN_CERRADA)) {                       /* V03b */
            v->estado = VAL_FALLA;
            return (val_accion_t){ .fila = 3, .publicar_estado = true, .evento = VPUB_FALLA_VALVULA };
        }
        v->estado = VAL_DESCONOCIDA;
        return (val_accion_t){ .fila = 3, .publicar_estado = true };

    case VAL_DESCONOCIDA:
        if (e->evento == VEV_FUGA) {
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return mover(v, e, 4, VAL_CERRANDO);
        }
        if (e->evento == VEV_CERRAR) { v->cerrada_por = VCP_ORDEN; return mover(v, e, 5, VAL_CERRANDO); }
        if (e->evento == VEV_BATERIA_CIERRE) { v->cerrada_por = VCP_BATERIA; return mover(v, e, 22, VAL_CERRANDO); }
        if (e->evento == VEV_CIEGO) { v->cerrada_por = VCP_SONDAS; return mover(v, e, 24, VAL_CERRANDO); }
        return ignorar;

    case VAL_ABIERTA:
        switch (e->evento) {
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return mover(v, e, 4, VAL_CERRANDO);
        case VEV_CERRAR:
            v->cerrada_por = VCP_ORDEN;
            return mover(v, e, 5, VAL_CERRANDO);
        case VEV_EJERCICIO:
            if (!e->g_seco || v->bloqueo_fuga) return ignorar;
            v->cerrada_por = VCP_EJERCICIO;
            return mover(v, e, 13, VAL_CERRANDO);
        case VEV_BATERIA_CIERRE:
            v->cerrada_por = VCP_BATERIA;
            return mover(v, e, 22, VAL_CERRANDO);
        case VEV_CIEGO:                                                 /* V24: bloqueo_fuga sin cambio */
            v->cerrada_por = VCP_SONDAS;
            return mover(v, e, 24, VAL_CERRANDO);
        case VEV_FINALES:
            if (f == VFIN_ABIERTA) return ignorar;                      /* sin cambio */
            return manual(v, f);
        default:
            return ignorar;
        }

    case VAL_CERRADA:
        switch (e->evento) {
        case VEV_ABRIR:
            if (e->g_seco && e->g_ciego)                                /* V25: antes que V08 */
                return (val_accion_t){ .fila = 25, .rechazo = VRECH_SONDAS_EN_FALLA };
            if (e->g_seco) return abrir(v, e, 8);
            return (val_accion_t){ .fila = 9, .rechazo = VRECH_FUGA_ACTIVA };
        case VEV_PULSACION_LARGA:
            if (e->g_seco && e->g_ciego)                                /* V25: antes que V10 */
                return (val_accion_t){ .fila = 25, .rechazo = VRECH_SONDAS_EN_FALLA, .patron_rechazo = true };
            if (e->g_seco) return abrir(v, e, 10);
            return (val_accion_t){ .fila = 11, .patron_rechazo = true };
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return (val_accion_t){ .fila = 17 };
        case VEV_BATERIA_REAPERTURA:
            if (v->cerrada_por != VCP_BATERIA || v->bloqueo_fuga || !e->g_seco || e->g_ciego) return ignorar;
            v->cerrada_por = VCP_NINGUNO;
            return mover(v, e, 23, VAL_ABRIENDO);
        case VEV_FINALES:
            if (f == VFIN_CERRADA) {                                    /* final «cerrada» confirmado */
                if (v->cerrada_por != VCP_EJERCICIO || !e->g_seco) return ignorar;
                v->cerrada_por = VCP_NINGUNO;                           /* sale abierta: ya no está cerrada por nada */
                return mover(v, e, 14, VAL_ABRIENDO);
            }
            return manual(v, f);
        default:
            return ignorar;
        }

    case VAL_CERRANDO:
    case VAL_ABRIENDO: {
        const bool cerrando = v->estado == VAL_CERRANDO;
        switch (e->evento) {
        case VEV_FINALES:
            if (cerrando && (f & VFIN_CERRADA)) return llegar(v, e, 6, VAL_CERRADA);
            if (!cerrando && (f & VFIN_ABIERTA)) return llegar(v, e, 12, VAL_ABIERTA);
            return ignorar;
        case VEV_TICK:
            if (e->t_ms - v->t_inicio_ms <= v->t_carrera_max_ms) return ignorar;
            v->estado = VAL_FALLA;
            return (val_accion_t){ .fila = 7, .motor = VMOT_PARAR, .evento = VPUB_FALLA_VALVULA };
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            if (cerrando) return (val_accion_t){ .fila = 21 };          /* V21: sigue cerrando */
            return mover(v, e, 18, VAL_CERRANDO);
        case VEV_CERRAR:
        case VEV_ABRIR:
            return (val_accion_t){ .fila = 19, .rechazo = VRECH_OCUPADO };
        default:
            return ignorar;
        }
    }

    case VAL_FALLA:
        switch (e->evento) {
        case VEV_CERRAR:
            v->cerrada_por = VCP_ORDEN;
            return mover(v, e, 16, VAL_CERRANDO);
        case VEV_ABRIR:
            if (e->g_seco) return abrir(v, e, 16);
            return (val_accion_t){ .fila = 16, .rechazo = VRECH_FUGA_ACTIVA };
        case VEV_PULSACION_LARGA:
            if (e->g_seco) return abrir(v, e, 16);
            return (val_accion_t){ .fila = 16, .patron_rechazo = true };
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return mover(v, e, 20, VAL_CERRANDO);
        default:
            return ignorar;
        }

    default:
        return ignorar;
    }
}

const char *val_nombre_estado(val_estado_t s)
{
    static const char *const N[] = { "ARRANQUE", "DESCONOCIDA", "ABIERTA", "CERRADA", "CERRANDO", "ABRIENDO", "FALLA" };
    return s < VAL_N_ESTADOS ? N[s] : "?";
}

const char *val_nombre_evento(val_evento_t e)
{
    static const char *const N[] = { "finales", "fuga", "cerrar", "abrir", "pulsacion_larga", "ejercicio", "tick",
                                       "bateria_cierre", "bateria_reapertura", "ciego" };
    return e < VEV_N_EVENTOS ? N[e] : "?";
}

bool eje_vencido(const eje_entrada_t *e)
{
    if (!e->hora_valida || !e->g_seco || e->bloqueo_fuga) return false;
    if (e->hora_local != e->ejercicio_hora || e->ejercicio_dias <= 0) return false;
    if (e->ahora_s < e->ultimo_mov_s) return false;
    return e->ahora_s - e->ultimo_mov_s >= (uint32_t)e->ejercicio_dias * 86400u;
}
