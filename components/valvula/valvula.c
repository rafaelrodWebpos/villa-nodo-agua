/* L2 valvula — V01–V30 (FSD §5.3, 0.8.10). Núcleo puro. */
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
    v->palanca = false;            /* el nodo mueve el motor: la posición ya no es de la palanca */
    v->epi_bat = v->epi_ciego = false;
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

/* V15: un final cambia sin orden. Al llegar a CERRADA, cerrada_por = ninguno (S-01): ni V23 ni
 * V30 reabren y solo una orden abre; un final tardío tras V07 cuenta igual (U-03) */
static val_accion_t manual(val_t *v, uint8_t f)
{
    v->estado = segun_finales(f);
    v->palanca = true;
    v->epi_bat = v->epi_ciego = false;
    if (v->estado == VAL_CERRADA) v->cerrada_por = VCP_NINGUNO;
    return (val_accion_t){ .fila = 15,
                           .evento = v->estado == VAL_FALLA ? VPUB_FALLA_VALVULA : VPUB_MOVIMIENTO_MANUAL };
}

void val_episodios(val_t *v, bool g_bateria, bool g_ciego)
{
    if (!g_bateria) v->epi_bat = false;
    if (!g_ciego) v->epi_ciego = false;
}

static val_accion_t rechazar(int fila, val_rechazo_t r, bool patron)
{
    return (val_accion_t){ .fila = fila, .rechazo = r, .patron_rechazo = patron };
}

/* V25, V26, V08/V10 y V09/V11 desde CERRADA (orden `abrir` o pulsación larga) */
static val_accion_t abrir_cerrada(val_t *v, const val_entrada_t *e, bool pulsacion)
{
    if (!e->g_seco) return pulsacion ? rechazar(11, VRECH_NINGUNO, true) : rechazar(9, VRECH_FUGA_ACTIVA, false);
    if (e->g_ciego) return rechazar(25, VRECH_SONDAS_EN_FALLA, pulsacion);
    if (e->g_bateria) return rechazar(26, VRECH_BATERIA_CRITICA, pulsacion);
    return abrir(v, e, pulsacion ? 10 : 8);
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
        if (e->evento == VEV_BATERIA_CIERRE && !v->palanca) { v->cerrada_por = VCP_BATERIA; return mover(v, e, 22, VAL_CERRANDO); }
        if (e->evento == VEV_CIEGO && !v->palanca) { v->cerrada_por = VCP_SONDAS; return mover(v, e, 24, VAL_CERRANDO); }
        if (e->evento == VEV_ABRIR || e->evento == VEV_PULSACION_LARGA)     /* V28 */
            return rechazar(28, VRECH_ESTADO, e->evento == VEV_PULSACION_LARGA);
        if (e->evento == VEV_FINALES && f != 0) return manual(v, f);        /* V15 */
        return ignorar;

    case VAL_ABIERTA:
        switch (e->evento) {
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return mover(v, e, 4, VAL_CERRANDO);
        case VEV_CERRAR:
            v->cerrada_por = VCP_ORDEN;
            return mover(v, e, 5, VAL_CERRANDO);
        case VEV_EJERCICIO:                                             /* V13: no con g_ciego (FSD 0.8.4) */
            if (!e->g_seco || v->bloqueo_fuga || e->g_ciego) return ignorar;
            v->cerrada_por = VCP_EJERCICIO;
            return mover(v, e, 13, VAL_CERRANDO);
        case VEV_BATERIA_CIERRE:                                        /* V22: no tras la palanca (U-02) */
            if (v->palanca) return ignorar;
            v->cerrada_por = VCP_BATERIA;
            return mover(v, e, 22, VAL_CERRANDO);
        case VEV_CIEGO:                                                 /* V24: bloqueo_fuga sin cambio */
            if (v->palanca) return ignorar;                             /* abierta a mano (P12) */
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
            return abrir_cerrada(v, e, false);
        case VEV_PULSACION_LARGA:
            return abrir_cerrada(v, e, true);
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return (val_accion_t){ .fila = 17 };
        case VEV_BATERIA_REAPERTURA:
            if (v->cerrada_por != VCP_BATERIA || v->bloqueo_fuga || !e->g_seco || e->g_ciego) return ignorar;
            v->cerrada_por = VCP_NINGUNO;
            return mover(v, e, 23, VAL_ABRIENDO);
        case VEV_FINALES:
            if (f == VFIN_CERRADA) {                                    /* final «cerrada» confirmado */
                if (v->cerrada_por != VCP_EJERCICIO) return ignorar;
                if (e->g_ciego) { v->cerrada_por = VCP_SONDAS; return (val_accion_t){ .fila = 29 }; }    /* V29 */
                if (e->g_bateria) { v->cerrada_por = VCP_BATERIA; return (val_accion_t){ .fila = 27 }; } /* V27 */
                if (!e->g_seco) return ignorar;                         /* sigue CERRADA; V30 la reevalúa */
                v->cerrada_por = VCP_NINGUNO;                           /* V14: sale abierta */
                return mover(v, e, 14, VAL_ABRIENDO);
            }
            return manual(v, f);
        case VEV_CICLO:                                                 /* V30, de nivel (Q-12) */
            if (v->cerrada_por != VCP_EJERCICIO || f != VFIN_CERRADA) return ignorar;
            if (e->g_ciego) { v->cerrada_por = VCP_SONDAS; return (val_accion_t){ .fila = 30 }; }
            if (e->g_bateria) { v->cerrada_por = VCP_BATERIA; return (val_accion_t){ .fila = 30 }; }
            if (!e->g_seco) return ignorar;
            v->cerrada_por = VCP_NINGUNO;
            return mover(v, e, 30, VAL_ABRIENDO);
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
        case VEV_ABRIR:                                                 /* V16: no con g_ciego (FSD 0.8.4) */
            if (e->g_seco && e->g_ciego) return (val_accion_t){ .fila = 16, .rechazo = VRECH_SONDAS_EN_FALLA };
            if (e->g_seco && e->g_bateria) return ignorar;              /* guarda de V08 sin cumplir: par no listado */
            if (e->g_seco) return abrir(v, e, 16);
            return (val_accion_t){ .fila = 16, .rechazo = VRECH_FUGA_ACTIVA };
        case VEV_PULSACION_LARGA:
            if (e->g_seco && e->g_ciego)
                return (val_accion_t){ .fila = 16, .rechazo = VRECH_SONDAS_EN_FALLA, .patron_rechazo = true };
            if (e->g_seco && e->g_bateria) return ignorar;              /* guarda de V10 sin cumplir */
            if (e->g_seco) return abrir(v, e, 16);
            return (val_accion_t){ .fila = 16, .patron_rechazo = true };
        case VEV_FUGA:
            v->bloqueo_fuga = true; v->cerrada_por = VCP_FUGA;
            return mover(v, e, 20, VAL_CERRANDO);
        case VEV_BATERIA_CIERRE:                                        /* V22 desde FALLA: un reintento por episodio */
            if (v->palanca || v->epi_bat) return ignorar;
            v->cerrada_por = VCP_BATERIA;
            { val_accion_t a = mover(v, e, 22, VAL_CERRANDO); v->epi_bat = true; return a; }
        case VEV_CIEGO:                                                 /* V24 desde FALLA: un reintento por episodio */
            if (v->palanca || v->epi_ciego) return ignorar;
            v->cerrada_por = VCP_SONDAS;
            { val_accion_t a = mover(v, e, 24, VAL_CERRANDO); v->epi_ciego = true; return a; }
        case VEV_FINALES:                                               /* V15 desde FALLA (U-03) */
            if (f == (VFIN_ABIERTA | VFIN_CERRADA)) return ignorar;
            return manual(v, f);
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
                                       "bateria_cierre", "bateria_reapertura", "ciego", "ciclo" };
    return e < VEV_N_EVENTOS ? N[e] : "?";
}

bool eje_vencido(const eje_entrada_t *e)
{
    if (!e->hora_valida || !e->g_seco || e->bloqueo_fuga) return false;
    if (e->hora_local != e->ejercicio_hora || e->ejercicio_dias <= 0) return false;
    if (e->ahora_s < e->ultimo_mov_s) return false;
    return e->ahora_s - e->ultimo_mov_s >= (uint32_t)e->ejercicio_dias * 86400u;
}
