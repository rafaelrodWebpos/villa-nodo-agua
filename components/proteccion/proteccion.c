/* L2 proteccion — FSD §5–§9 y §12 sobre los núcleos probados. Núcleo puro. */
#include "proteccion.h"
#include <string.h>

void prot_config_fabrica(prot_config_t *c)
{
    *c = (prot_config_t){
        .t_carrera_max_s = 30, .ejercicio_dias = 30, .ejercicio_hora = 3,
        .nivel_critico_pct = 10, .nivel_rearme_pct = 30, .intentos_max = 3, .t_rearme_s = 10 * 60,
        .rango_transductor_cm = 500, .altura_util_cm = 200,
        .i_marcha_a = 0.5f, .i_seco_a = 0.0f, .p_min_bar = 0.5f, .t_seco_s = 10,
        .ciclado_max = 6, .ciclado_ventana_s = 60 * 60,
        .zumbador_activo = true, .zumbador_min = 5,
    };
    for (int i = 0; i < PROT_N_SONDAS; i++) c->sonda_activa[i] = true;
}

void prot_iniciar(prot_t *p, const prot_config_t *cfg, const prot_persistente_t *nvs, uint32_t t_s)
{
    memset(p, 0, sizeof *p);
    p->cfg = *cfg;
    val_iniciar(&p->val, nvs->bloqueo_fuga, nvs->cerrada_por, cfg->t_carrera_max_s);
    bom_iniciar(&p->bom, cfg->intentos_max, cfg->nivel_critico_pct);
    p->bom.estado = nvs->bomba;
    p->bom.motivo = nvs->bomba == BOM_CORTADA ? nvs->motivo : BMOT_NINGUNO;
    /* Los instantes de antes del reinicio no se conocen en este reloj: se cuentan como
     * ocurridos ahora, que es lo más conservador (siguen contando 24 h más). */
    for (int i = 0; i < nvs->n_cortes && i < BOM_MAX_CORTES; i++) p->bom.cortes[p->bom.n_cortes++] = t_s;
    for (int i = 0; i < PROT_N_SONDAS; i++) p->clase[i] = -2;      /* sin lectura todavía */
    for (int k = 0; k < ES_N_ADS; k++) p->ads_ok[k] = true;
    p->motor = ES_MOTOR_PARADO;
    p->guardado = prot_persistente(p);
}

prot_persistente_t prot_persistente(const prot_t *p)
{
    return (prot_persistente_t){ p->val.bloqueo_fuga, p->val.cerrada_por, p->bom.estado,
                                 p->bom.estado == BOM_CORTADA ? p->bom.motivo : BMOT_NINGUNO, p->bom.n_cortes };
}

bool prot_sonda_mojada(const prot_t *p)
{
    for (int i = 0; i < PROT_N_SONDAS; i++) if (p->cfg.sonda_activa[i] && p->mojada[i]) return true;
    return false;
}

static void emitir(prot_salida_t *s, prot_ev_tipo_t t, int a, int b, int c, int d)
{
    if (s->n_ev < PROT_MAX_EVENTOS) s->ev[s->n_ev++] = (prot_evento_t){ t, a, b, c, d };
}

static void rechazo(prot_t *p, uint32_t t_ms) { p->rechazo_hasta_ms = t_ms + 2000u; }

static void aplicar_val(prot_t *p, const prot_entrada_t *e, val_evento_t ev, prot_salida_t *s)
{
    val_estado_t desde = p->val.estado;
    val_entrada_t ve = { ev, p->finales, !prot_sonda_mojada(p), e->t_ms };
    val_accion_t a = val_evento(&p->val, &ve);
    if (a.fila == 0) return;
    emitir(s, PEV_VAL, a.fila, desde, p->val.estado, ev);
    if (a.motor == VMOT_CERRAR) p->motor = ES_MOTOR_CERRAR;
    else if (a.motor == VMOT_ABRIR) p->motor = ES_MOTOR_ABRIR;
    else if (a.motor == VMOT_PARAR) p->motor = ES_MOTOR_PARADO;
    if (a.evento == VPUB_FALLA_VALVULA) emitir(s, PEV_FALLA_VALVULA, 0, 0, 0, 0);
    if (a.evento == VPUB_MOVIMIENTO_MANUAL) emitir(s, PEV_MOVIMIENTO_MANUAL, 0, 0, 0, 0);
    if (a.patron_rechazo) { rechazo(p, e->t_ms); emitir(s, PEV_RECHAZO, 1, 0, 0, 0); }
    if ((a.fila == 6 || a.fila == 12) && e->hora_valida) p->ultimo_mov_s = e->ahora_s;   /* movimiento completo */
}

static void aplicar_bom(prot_t *p, const prot_entrada_t *e, bom_evento_t ev, prot_salida_t *s)
{
    bom_estado_t desde = p->bom.estado;
    bom_entrada_t be = { ev, p->flotador_agua, p->nivel_valido, p->nivel_pct, e->t_s };
    bom_accion_t a = bom_evento(&p->bom, &be);
    if (a.fila == 0) return;
    emitir(s, PEV_BOM, a.fila, desde, p->bom.estado, ev);
    if (a.evento_bloqueada) emitir(s, PEV_BOMBA_BLOQUEADA, 0, 0, 0, 0);
    if (a.rechazo == BRECH_SIN_AGUA && ev == BEV_REARMAR) { rechazo(p, e->t_ms); emitir(s, PEV_RECHAZO, 2, 0, 0, 0); }
}

static bool clase_falla(int c) { return c == -1 || c == SON_ABIERTA || c == SON_CORTO || c == SON_INDETERMINADA; }

static void medidas(prot_t *p, const prot_entrada_t *e, prot_salida_t *s)
{
    for (int k = 0; k < ES_N_ADS; k++) {                      /* FR-IO-06 */
        if (p->ads_ok[k] && !e->ads_ok[k]) emitir(s, PEV_FALLA_ADC, k, 0, 0, 0);
        p->ads_ok[k] = e->ads_ok[k];
    }
    /* Referencia de los lazos: A3 del 0x4A o la nominal (FSD 0.6.0) */
    bool nominal;
    p->vref = es_vref(p->ads_ok[2], e->v_ref, &nominal);
    if (nominal != p->vref_nominal) emitir(s, PEV_VREF, nominal, 0, 0, 0);
    p->vref_nominal = nominal;
    /* Sondas: clase (FR-SON-01) y falla (FR-SON-02); las inactivas no cuentan (FR-SON-03) */
    for (int i = 0; i < PROT_N_SONDAS; i++) {
        if (!p->cfg.sonda_activa[i]) {
            p->mojada[i] = p->confirmada[i] = p->en_falla[i] = p->falla_avisada[i] = false;
            p->fug[i].seguidas = 0;
            continue;
        }
        int c = p->ads_ok[i / 4] ? (int)son_clasificar(es_sonda_fraccion(e->v_sonda[i], p->vref)) : -1;
        if (c != p->clase[i]) emitir(s, PEV_SON, i + 1, c, 0, 0);
        p->clase[i] = c;
        p->mojada[i] = c == SON_MOJADA;                         /* P13: en falla cuenta como no mojada */
        if (clase_falla(c)) {
            if (!p->en_falla[i]) { p->en_falla[i] = true; p->falla_desde_ms[i] = e->t_ms; }
            uint32_t umbral = c == -1 ? 0u : PROT_FALLA_SONDA_MS;   /* ADS caído: ya (FR-IO-06, ≤ 2 s) */
            if (!p->falla_avisada[i] && e->t_ms - p->falla_desde_ms[i] >= umbral) {
                p->falla_avisada[i] = true;
                emitir(s, PEV_FALLA_SONDA, i + 1, c, 0, 0);
            }
        } else {
            p->en_falla[i] = p->falla_avisada[i] = false;
        }
    }
    for (int i = 0; i < PROT_N_SONDAS; i++) {                  /* FR-FUG-01 → V04/V17/V18/V20/V21 */
        if (!p->cfg.sonda_activa[i]) continue;
        bool conf = fug_muestra(&p->fug[i], p->mojada[i]);
        if (conf && !p->confirmada[i]) { emitir(s, PEV_FUGA, i + 1, 0, 0, 0); p->confirmada[i] = true; aplicar_val(p, e, VEV_FUGA, s); }
        p->confirmada[i] = conf;
    }
    /* Nivel, presión y batería (FR-NIV-01/02, FR-PRE-01, FR-ENE-02) */
    bool ok = p->ads_ok[2];
    p->nivel_valido = ok && bom_nivel(es_nivel_ma(e->v_nivel), p->cfg.rango_transductor_cm, p->cfg.altura_util_cm,
                                      &p->nivel_cm, &p->nivel_pct);
    p->presion_valida = ok && bom_presion(es_presion_sensor_v(e->v_presion), &p->presion_bar);
    if (ok) {
        p->vbat = es_vbat_v(e->v_bat);
        ene_estado_t n = ene_clasificar(p->bateria_conocida ? p->bateria : ENE_OK, p->vbat);
        if (n != p->bateria || !p->bateria_conocida) {
            if (p->bateria_conocida || n != ENE_OK) emitir(s, PEV_BATERIA, n, 0, 0, 0);
            p->bateria = n;
            p->bateria_conocida = true;
        }
    }
    /* FR-BOM-10: flotador sin agua 10 min con el transductor válido y nivel suficiente → en falla */
    bool cond = p->nivel_valido && p->nivel_pct >= (float)p->cfg.nivel_rearme_pct && !p->flotador_real;
    if (p->flotador_falla && (p->flotador_real || !p->nivel_valido)) {
        p->flotador_falla = false;
        emitir(s, PEV_FLOTADOR_FALLA, 0, 0, 0, 0);
    }
    if (!cond) p->flot_en_curso = false;
    else if (!p->flot_en_curso) { p->flot_en_curso = true; p->flot_desde_s = e->t_s; }
    bool declarar = cond && !p->flotador_falla && e->t_s - p->flot_desde_s >= PROT_FLOTADOR_FALLA_S;
    if (declarar) { p->flotador_falla = true; emitir(s, PEV_FLOTADOR_FALLA, 1, 0, 0, 0); }
    p->flotador_agua = p->flotador_real || p->flotador_falla;
    if (declarar) aplicar_bom(p, e, BEV_REARME_CUMPLIDO, s);   /* el nivel ya llevaba 10 min suficiente */
    aplicar_bom(p, e, BEV_NIVEL, s);                           /* B03 */
    if (rea_tick(&p->rearme, e->t_s, p->nivel_valido, p->nivel_pct, p->flotador_agua,
                 p->cfg.nivel_rearme_pct, p->cfg.t_rearme_s))
        aplicar_bom(p, e, BEV_REARME_CUMPLIDO, s);             /* B05/B06 */
    if (p->bateria_conocida && ene_cierre(p->bateria, p->red_presente)) aplicar_val(p, e, VEV_BATERIA_CIERRE, s);
    if (ok && ene_reapertura(&p->reap, e->t_s, p->red_presente, p->vbat)) aplicar_val(p, e, VEV_BATERIA_REAPERTURA, s);
    if (p->val.estado == VAL_CERRADA && p->val.cerrada_por == VCP_EJERCICIO) aplicar_val(p, e, VEV_FINALES, s);   /* V14 */
    if (e->hora_valida) {                                      /* FR-EJE-01 */
        if (p->ultimo_mov_s == 0) p->ultimo_mov_s = e->ahora_s;   /* cuenta desde la primera hora válida */
        eje_entrada_t je = { true, e->hora_local, e->ahora_s, p->ultimo_mov_s, p->cfg.ejercicio_dias,
                             p->cfg.ejercicio_hora, !prot_sonda_mojada(p), p->val.bloqueo_fuga };
        if (eje_vencido(&je)) aplicar_val(p, e, VEV_EJERCICIO, s);
    }
}

static void corriente(prot_t *p, const prot_entrada_t *e, prot_salida_t *s)
{
    p->i_rms = e->i_rms;
    float bar = p->presion_valida ? p->presion_bar : 1e9f;     /* sin presión válida no se declara seco */
    if (sec_muestra(&p->seco, e->t_ms, e->i_rms, bar, p->cfg.i_marcha_a, p->cfg.i_seco_a, p->cfg.p_min_bar, p->cfg.t_seco_s))
        aplicar_bom(p, e, BEV_SECO, s);                        /* B01 */
    bool m = e->i_rms >= p->cfg.i_marcha_a;                    /* P7: arranque = ≥ i_marcha 2 s tras ≥ 2 s por debajo */
    if (m != p->marcha_cand) { p->marcha_cand = m; p->marcha_cambio_ms = e->t_ms; }
    if (p->marcha_cand != p->marcha && e->t_ms - p->marcha_cambio_ms >= 2000u) {
        p->marcha = p->marcha_cand;
        int n;
        if (p->marcha && cic_arranque(&p->cic, e->t_s, p->cfg.ciclado_ventana_s, p->cfg.ciclado_max, &n))
            emitir(s, PEV_CICLADO, n, p->val.estado == VAL_ABIERTA, 0, 0);   /* FR-CIC-02 */
    }
}

static void salidas(prot_t *p, const prot_entrada_t *e, prot_salida_t *s)
{
    es_puente(p->motor, &s->in1, &s->in2);
    s->rele = bom_rele(&p->bom);
    bool fuga = false, sonda_falla = false;
    for (int i = 0; i < PROT_N_SONDAS; i++) { fuga |= p->confirmada[i]; sonda_falla |= p->falla_avisada[i]; }
    if (fuga && !p->fuga_activa) { p->fuga_desde_ms = e->t_ms; p->silenciado = false; }
    p->fuga_activa = fuga;
    loc_entrada_t le = { fuga, p->val.estado == VAL_FALLA, p->bom.estado == BOM_BLOQUEADA, sonda_falla,
                         p->bateria_conocida && p->bateria == ENE_CRITICA, p->val.estado == VAL_CERRADA,
                         p->bom.estado == BOM_CORTADA, !e->conectado };
    loc_patron_t pt = loc_patron(&le);
    if (pt.prioridad == 2 && p->prio_anterior != 2) p->prio2_desde_ms = e->t_ms;
    p->prio_anterior = pt.prioridad;
    const uint32_t t = e->t_ms;
    s->led_verde = pt.led == LED_VERDE_FIJO || (pt.led == LED_VERDE_1HZ && es_onda_hz(t, 1000));
    s->led_rojo = pt.led == LED_ROJO_FIJO || (pt.led == LED_ROJO_1HZ && es_onda_hz(t, 1000)) ||
                  (pt.led == LED_ROJO_025HZ && es_onda_hz(t, 4000));
    bool z = false;
    if (pt.zumbador == ZUM_1HZ) z = !p->silenciado && t - p->fuga_desde_ms < p->cfg.zumbador_min * 60000u && es_onda_hz(t, 1000);
    else if (pt.zumbador == ZUM_3_PITIDOS_10MIN) z = es_onda_3_pitidos(t, p->prio2_desde_ms);
    int32_t queda = (int32_t)(p->rechazo_hasta_ms - t);
    if (queda > 0) {                                           /* pulsación larga rechazada: 2 s a 5 Hz y un pitido largo */
        s->led_verde = false;
        s->led_rojo = es_onda_hz(t, 200);
        z = queda > 1000;
    }
    s->zumbador = z && p->cfg.zumbador_activo;                 /* FR-LOC-05 */
}

void prot_paso(prot_t *p, const prot_entrada_t *e, prot_salida_t *s)
{
    s->n_ev = 0;
    s->persistir = false;
    /* Entradas digitales con antirrebote (FR-IO-05) */
    bool fa = es_antirrebote(&p->ar_abierta, e->t_ms, e->fin_abierta);
    bool fc = es_antirrebote(&p->ar_cerrada, e->t_ms, e->fin_cerrada);
    p->flotador_real = es_antirrebote(&p->ar_flotador, e->t_ms, e->flotador_agua);
    p->flotador_agua = p->flotador_real || p->flotador_falla;   /* FR-BOM-10 */
    p->red_presente = es_antirrebote(&p->ar_red, e->t_ms, e->red_presente);
    bool bt = es_antirrebote(&p->ar_boton, e->t_ms, e->boton);
    uint8_t f = (fa ? VFIN_ABIERTA : 0) | (fc ? VFIN_CERRADA : 0);
    if (!p->t0_fijado) { p->t0_fijado = true; p->t0_ms = e->t_ms; }
    if (!p->arrancado) {
        if (e->t_ms - p->t0_ms < ES_ANTIRREBOTE_MS) { salidas(p, e, s); return; }   /* motor parado; relé según NVS */
        p->finales = f;
        p->arrancado = true;
        aplicar_val(p, e, VEV_FINALES, s);                      /* V01–V03b, sin mover el motor (FR-VAL-06) */
    } else if (f != p->finales) {
        p->finales = f;
        aplicar_val(p, e, VEV_FINALES, s);                      /* V06/V12/V15 */
    }
    if (!p->flotador_agua) aplicar_bom(p, e, BEV_FLOTADOR_SIN_AGUA, s);   /* B02 (FR-BOM-04) */
    es_boton_ev_t b = es_boton(&p->boton, e->t_ms, bt);
    if (b == ES_BOTON_CORTA) p->silenciado = true;              /* FR-LOC-02 */
    if (b == ES_BOTON_LARGA) {                                  /* FR-LOC-03: V10/V11 y B07/B08 */
        aplicar_val(p, e, VEV_PULSACION_LARGA, s);
        aplicar_bom(p, e, BEV_REARMAR, s);
    }
    if (e->hay_medidas) medidas(p, e, s);
    if (e->hay_corriente) corriente(p, e, s);
    aplicar_val(p, e, VEV_TICK, s);                             /* V07 */
    salidas(p, e, s);
    prot_persistente_t ahora = prot_persistente(p);             /* FR-PER-01: solo en los cambios */
    const prot_persistente_t *g = &p->guardado;
    if (ahora.bloqueo_fuga != g->bloqueo_fuga || ahora.cerrada_por != g->cerrada_por || ahora.bomba != g->bomba ||
        ahora.motivo != g->motivo || ahora.n_cortes != g->n_cortes) { p->guardado = ahora; s->persistir = true; }
}
