/* Tablas normativas y núcleos de §5, §6 y §8 en host:
 *   val  · TC-VAL-A1 · V01–V20 (tabla parametrizada + completitud de §5.3)
 *   eje  · TC-EJE-A1 · FR-EJE-01 (reloj simulado) y la secuencia V13 → V06 → V14 → V12
 *   bom  · TC-BOM-A1 · B01–B09 (tabla parametrizada + completitud), FR-BOM-05 y FR-BOM-07 (parte host)
 *   seco · TC-BOM-A2 · FR-BOM-03 (series de corriente y presión)
 *   loc  · TC-LOC-A1 · FR-LOC-04 (todas las combinaciones de §8.2)
 *   son  · TC-SON-A1 · FR-SON-01 (bandas de P4 y sus bordes, FSD 0.4.1)
 * Uso: test_tablas <caso> */
#include <stdio.h>
#include <string.h>
#include "bomba.h"
#include "fugas.h"
#include "interfaz.h"
#include "valvula.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ======================= Válvula ======================= */
enum { A = VFIN_ABIERTA, C = VFIN_CERRADA, AC = VFIN_ABIERTA | VFIN_CERRADA, N0 = 0 };
#define T_MAX_S 30
typedef struct {
    int fila;
    val_estado_t desde; bool bloq0; val_cerrada_por_t cp0;
    val_evento_t ev; uint8_t fin; bool seco; uint32_t t;
    val_estado_t hacia; val_motor_t motor; val_rechazo_t rech; val_publicar_t pub; bool patron;
    bool bloq; val_cerrada_por_t cp;
} vcaso_t;

static const vcaso_t VC[] = {
    /* fila desde          bloq0  cp0            evento              fin  seco  t        hacia          motor         rechazo            publicar                patrón bloq   cp */
    { 1, VAL_ARRANQUE,    false, VCP_NINGUNO,   VEV_FINALES,         A,  true, 0,       VAL_ABIERTA,    VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, false, false, VCP_NINGUNO },
    { 2, VAL_ARRANQUE,    true,  VCP_FUGA,      VEV_FINALES,         C,  true, 0,       VAL_CERRADA,    VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, false, true,  VCP_FUGA },
    { 3, VAL_ARRANQUE,    false, VCP_NINGUNO,   VEV_FINALES,         N0, true, 0,       VAL_DESCONOCIDA,VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, false, false, VCP_NINGUNO },
    { 3, VAL_ARRANQUE,    false, VCP_NINGUNO,   VEV_FINALES,         AC, true, 0,       VAL_DESCONOCIDA,VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, false, false, VCP_NINGUNO },
    { 4, VAL_ABIERTA,     false, VCP_NINGUNO,   VEV_FUGA,            A,  false,0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, true,  VCP_FUGA },
    { 4, VAL_DESCONOCIDA, false, VCP_NINGUNO,   VEV_FUGA,            N0, false,0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, true,  VCP_FUGA },
    { 5, VAL_ABIERTA,     false, VCP_NINGUNO,   VEV_CERRAR,          A,  true, 0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_ORDEN },
    { 5, VAL_DESCONOCIDA, false, VCP_NINGUNO,   VEV_CERRAR,          N0, true, 0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_ORDEN },
    { 6, VAL_CERRANDO,    false, VCP_ORDEN,     VEV_FINALES,         C,  true, 4000,    VAL_CERRADA,    VMOT_PARAR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_ORDEN },
    { 7, VAL_CERRANDO,    false, VCP_ORDEN,     VEV_TICK,            N0, true, 30001,   VAL_FALLA,      VMOT_PARAR,   VRECH_NINGUNO,    VPUB_FALLA_VALVULA, false, false, VCP_ORDEN },
    { 7, VAL_ABRIENDO,    false, VCP_NINGUNO,   VEV_TICK,            N0, true, 30001,   VAL_FALLA,      VMOT_PARAR,   VRECH_NINGUNO,    VPUB_FALLA_VALVULA, false, false, VCP_NINGUNO },
    { 8, VAL_CERRADA,     true,  VCP_FUGA,      VEV_ABRIR,           C,  true, 0,       VAL_ABRIENDO,   VMOT_ABRIR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_NINGUNO },
    { 9, VAL_CERRADA,     true,  VCP_FUGA,      VEV_ABRIR,           C,  false,0,       VAL_CERRADA,    VMOT_SIN_CAMBIO, VRECH_FUGA_ACTIVA, VPUB_NINGUNO, false, true, VCP_FUGA },
    {10, VAL_CERRADA,     true,  VCP_FUGA,      VEV_PULSACION_LARGA, C,  true, 0,       VAL_ABRIENDO,   VMOT_ABRIR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_NINGUNO },
    {11, VAL_CERRADA,     true,  VCP_FUGA,      VEV_PULSACION_LARGA, C,  false,0,       VAL_CERRADA,    VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, true,  true,  VCP_FUGA },
    {12, VAL_ABRIENDO,    false, VCP_NINGUNO,   VEV_FINALES,         A,  true, 5000,    VAL_ABIERTA,    VMOT_PARAR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_NINGUNO },
    {13, VAL_ABIERTA,     false, VCP_NINGUNO,   VEV_EJERCICIO,       A,  true, 0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_EJERCICIO },
    {14, VAL_CERRADA,     false, VCP_EJERCICIO, VEV_FINALES,         C,  true, 0,       VAL_ABRIENDO,   VMOT_ABRIR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_NINGUNO },
    {15, VAL_ABIERTA,     false, VCP_NINGUNO,   VEV_FINALES,         C,  true, 0,       VAL_CERRADA,    VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_MOVIMIENTO_MANUAL, false, false, VCP_NINGUNO },
    {15, VAL_CERRADA,     false, VCP_ORDEN,     VEV_FINALES,         A,  true, 0,       VAL_ABIERTA,    VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_MOVIMIENTO_MANUAL, false, false, VCP_ORDEN },
    {15, VAL_ABIERTA,     false, VCP_NINGUNO,   VEV_FINALES,         N0, true, 0,       VAL_DESCONOCIDA,VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_MOVIMIENTO_MANUAL, false, false, VCP_NINGUNO },
    {16, VAL_FALLA,       false, VCP_NINGUNO,   VEV_CERRAR,          N0, true, 0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_ORDEN },
    {16, VAL_FALLA,       true,  VCP_FUGA,      VEV_ABRIR,           N0, true, 0,       VAL_ABRIENDO,   VMOT_ABRIR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_NINGUNO },
    {16, VAL_FALLA,       true,  VCP_FUGA,      VEV_ABRIR,           N0, false,0,       VAL_FALLA,      VMOT_SIN_CAMBIO, VRECH_FUGA_ACTIVA, VPUB_NINGUNO, false, true, VCP_FUGA },
    {16, VAL_FALLA,       false, VCP_NINGUNO,   VEV_PULSACION_LARGA, N0, true, 0,       VAL_ABRIENDO,   VMOT_ABRIR,   VRECH_NINGUNO,    VPUB_NINGUNO, false, false, VCP_NINGUNO },
    {16, VAL_FALLA,       false, VCP_NINGUNO,   VEV_PULSACION_LARGA, N0, false,0,       VAL_FALLA,      VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, true,  false, VCP_NINGUNO },
    {17, VAL_CERRADA,     false, VCP_ORDEN,     VEV_FUGA,            C,  false,0,       VAL_CERRADA,    VMOT_SIN_CAMBIO, VRECH_NINGUNO, VPUB_NINGUNO, false, true,  VCP_FUGA },
    {18, VAL_ABRIENDO,    false, VCP_NINGUNO,   VEV_FUGA,            N0, false,0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, true,  VCP_FUGA },
    {19, VAL_CERRANDO,    false, VCP_ORDEN,     VEV_ABRIR,           N0, true, 0,       VAL_CERRANDO,   VMOT_SIN_CAMBIO, VRECH_OCUPADO, VPUB_NINGUNO, false, false, VCP_ORDEN },
    {19, VAL_ABRIENDO,    false, VCP_NINGUNO,   VEV_CERRAR,          N0, true, 0,       VAL_ABRIENDO,   VMOT_SIN_CAMBIO, VRECH_OCUPADO, VPUB_NINGUNO, false, false, VCP_NINGUNO },
    {20, VAL_FALLA,       false, VCP_ORDEN,     VEV_FUGA,            N0, false,0,       VAL_CERRANDO,   VMOT_CERRAR,  VRECH_NINGUNO,    VPUB_NINGUNO, false, true,  VCP_ORDEN },
};

/* Filas que la tabla de §5.3 admite para cada (estado × evento); 0 = no listado */
static bool val_admitida(val_estado_t s, val_evento_t e, int fila)
{
    static const int M[VAL_N_ESTADOS][VEV_N_EVENTOS][3] = {
        /*              finales      fuga       cerrar     abrir      larga      ejercicio  tick */
        [VAL_ARRANQUE]    = {{1,2,3}, {0},       {0},       {0},       {0},       {0},       {0}},
        [VAL_DESCONOCIDA] = {{0},     {4},       {5},       {0},       {0},       {0},       {0}},
        [VAL_ABIERTA]     = {{15},    {4},       {5},       {0},       {0},       {13},      {0}},
        [VAL_CERRADA]     = {{14,15}, {17},      {0},       {8,9},     {10,11},   {0},       {0}},
        [VAL_CERRANDO]    = {{6},     {0},       {19},      {19},      {0},       {0},       {7}},
        [VAL_ABRIENDO]    = {{12},    {18},      {19},      {19},      {0},       {0},       {7}},
        [VAL_FALLA]       = {{0},     {20},      {16},      {16},      {16},      {0},       {0}},
    };
    for (int i = 0; i < 3; i++) if (M[s][e][i] == fila && fila) return true;
    return false;
}

static void val(void)
{
    for (size_t i = 0; i < sizeof VC / sizeof VC[0]; i++) {
        const vcaso_t *k = &VC[i];
        val_t v;
        val_iniciar(&v, k->bloq0, k->cp0, T_MAX_S);
        v.estado = k->desde;
        v.t_inicio_ms = 0;
        val_entrada_t e = { k->ev, k->fin, k->seco, k->t };
        val_accion_t a = val_evento(&v, &e);
        OK(a.fila == k->fila && v.estado == k->hacia && a.motor == k->motor && a.rechazo == k->rech &&
           a.evento == k->pub && a.patron_rechazo == k->patron && v.bloqueo_fuga == k->bloq && v.cerrada_por == k->cp,
           "V%02d (%s × %s): fila %d, hacia %s, motor %d, rechazo %d, publica %d, patrón %d, bloqueo %d, cerrada_por %d",
           k->fila, val_nombre_estado(k->desde), val_nombre_evento(k->ev), a.fila, val_nombre_estado(v.estado),
           a.motor, a.rechazo, a.evento, a.patron_rechazo, v.bloqueo_fuga, v.cerrada_por);
    }
    /* tiempo de carrera medido y límite exacto de P3 */
    val_t v; val_iniciar(&v, false, VCP_NINGUNO, T_MAX_S); v.estado = VAL_CERRANDO; v.t_inicio_ms = 1000;
    val_entrada_t t = { VEV_TICK, 0, true, 31000 };
    OK(val_evento(&v, &t).fila == 0 && v.estado == VAL_CERRANDO, "FALLA con exactamente P3 (no superado)");
    val_entrada_t f = { VEV_FINALES, C, true, 4200 };
    OK(val_evento(&v, &f).t_carrera_ms == 3200, "tiempo de carrera de V06");
    /* completitud: cada (estado × evento × finales × g_seco) da una fila admitida o se ignora sin efecto */
    int pares = 0, ignorados = 0;
    for (int s = 0; s < VAL_N_ESTADOS; s++) for (int ev = 0; ev < VEV_N_EVENTOS; ev++)
    for (int fin = 0; fin < 4; fin++) for (int seco = 0; seco < 2; seco++) for (int cp = 0; cp <= VCP_EJERCICIO; cp++)
    for (int bl = 0; bl < 2; bl++) {
        val_iniciar(&v, bl, (val_cerrada_por_t)cp, T_MAX_S);
        v.estado = (val_estado_t)s; v.t_inicio_ms = 0;
        val_t antes = v;
        val_entrada_t e = { (val_evento_t)ev, (uint8_t)fin, seco, ev == VEV_TICK ? 30001u : 100u };
        val_accion_t a = val_evento(&v, &e);
        pares++;
        if (a.fila == 0) {
            ignorados++;
            OK(!memcmp(&antes, &v, sizeof v) && a.motor == VMOT_SIN_CAMBIO && a.rechazo == VRECH_NINGUNO &&
               a.evento == VPUB_NINGUNO && !a.patron_rechazo,
               "par ignorado con efecto: %s × %s fin=%d seco=%d", val_nombre_estado(s), val_nombre_evento(ev), fin, seco);
        } else {
            OK(val_admitida(s, ev, a.fila), "transición no listada: %s × %s → V%02d", val_nombre_estado(s), val_nombre_evento(ev), a.fila);
        }
    }
    printf("val: %zu filas, %d combinaciones (%d ignoradas)\n", sizeof VC / sizeof VC[0], pares, ignorados);
}

/* ======================= Ejercicio ======================= */
static void eje(void)
{
    const uint32_t D = 86400, T0 = 1800000000u;     /* T0 es una hora UTC cualquiera */
    eje_entrada_t b = { true, 3, T0 + 30 * D, T0, 30, 3, true, false };
    OK(eje_vencido(&b), "no dispara con todo cumplido");
    eje_entrada_t e = b; e.hora_local = 4;          OK(!eje_vencido(&e), "dispara fuera de ejercicio_hora");
    e = b; e.ahora_s = T0 + 30 * D - 1;             OK(!eje_vencido(&e), "dispara 1 s antes de cumplir los días");
    e = b; e.hora_valida = false;                   OK(!eje_vencido(&e), "dispara sin hora válida");
    e = b; e.bloqueo_fuga = true;                   OK(!eje_vencido(&e), "dispara con bloqueo_fuga");
    e = b; e.g_seco = false;                        OK(!eje_vencido(&e), "dispara con sonda mojada");
    int horas = 0;                                  /* reloj simulado: 40 días hora a hora */
    for (uint32_t t = T0; t < T0 + 40 * D; t += 3600) {
        e = b; e.ahora_s = t; e.hora_local = (int)((t / 3600) % 24);
        horas += eje_vencido(&e);
    }
    OK(horas == 10, "días 30–39 a la hora 3: %d disparos (esperado 10 sin mover la válvula)", horas);
    /* secuencia V13 → V06 → V14 → V12 con tiempos de carrera */
    val_t v; val_iniciar(&v, false, VCP_NINGUNO, T_MAX_S); v.estado = VAL_ABIERTA;
    int filas[4]; uint32_t tc[4];
    val_entrada_t s[4] = { { VEV_EJERCICIO, A, true, 1000 }, { VEV_FINALES, C, true, 4000 },
                           { VEV_FINALES, C, true, 4100 }, { VEV_FINALES, A, true, 7300 } };
    for (int i = 0; i < 4; i++) { val_accion_t a = val_evento(&v, &s[i]); filas[i] = a.fila; tc[i] = a.t_carrera_ms; }
    OK(filas[0] == 13 && filas[1] == 6 && filas[2] == 14 && filas[3] == 12 && v.estado == VAL_ABIERTA &&
       v.cerrada_por == VCP_NINGUNO && tc[1] == 3000 && tc[3] == 3200,
       "ejercicio: filas %d %d %d %d, cierre %u ms, apertura %u ms", filas[0], filas[1], filas[2], filas[3], tc[1], tc[3]);
}

/* ======================= Bomba ======================= */
typedef struct {
    int fila; bom_estado_t desde; bom_motivo_t m0; int cortes0;
    bom_evento_t ev; bool agua; bool nv; float pct;
    bom_estado_t hacia; bom_motivo_t m; bom_rechazo_t rech; bool bloq_ev; int intentos;
} bcaso_t;
static const bcaso_t BC[] = {
    /* fila desde          m0            cortes evento                  agua   nv     pct   hacia          motivo        rechazo          ev     intentos */
    { 1, BOM_PERMITIDA, BMOT_NINGUNO,  0, BEV_SECO,               true,  true,  50, BOM_CORTADA,   BMOT_SECO,     BRECH_NINGUNO, false, 1 },
    { 2, BOM_PERMITIDA, BMOT_NINGUNO,  0, BEV_FLOTADOR_SIN_AGUA,  false, true,  80, BOM_CORTADA,   BMOT_FLOTADOR, BRECH_NINGUNO, false, 0 },
    { 3, BOM_PERMITIDA, BMOT_NINGUNO,  0, BEV_NIVEL,              true,  true,   9, BOM_CORTADA,   BMOT_NIVEL,    BRECH_NINGUNO, false, 0 },
    { 0, BOM_PERMITIDA, BMOT_NINGUNO,  0, BEV_NIVEL,              true,  false,  9, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },  /* transductor en falla: sin B03 */
    { 0, BOM_PERMITIDA, BMOT_NINGUNO,  0, BEV_NIVEL,              true,  true,  10, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },  /* = crítico: no es < */
    { 4, BOM_PERMITIDA, BMOT_NINGUNO,  0, BEV_CORTAR,             true,  true,  50, BOM_CORTADA,   BMOT_ORDEN,    BRECH_NINGUNO, false, 0 },
    { 5, BOM_CORTADA,   BMOT_SECO,     2, BEV_REARME_CUMPLIDO,    true,  true,  35, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 2 },
    { 5, BOM_CORTADA,   BMOT_NIVEL,    0, BEV_REARME_CUMPLIDO,    true,  true,  35, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },
    { 5, BOM_CORTADA,   BMOT_FLOTADOR, 0, BEV_REARME_CUMPLIDO,    true,  true,  35, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },
    { 6, BOM_CORTADA,   BMOT_SECO,     3, BEV_REARME_CUMPLIDO,    true,  true,  35, BOM_BLOQUEADA, BMOT_NINGUNO,  BRECH_NINGUNO, true,  3 },
    { 0, BOM_CORTADA,   BMOT_ORDEN,    0, BEV_REARME_CUMPLIDO,    true,  true,  35, BOM_CORTADA,   BMOT_ORDEN,    BRECH_NINGUNO, false, 0 },
    { 7, BOM_CORTADA,   BMOT_ORDEN,    1, BEV_REARMAR,            true,  false,  0, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },
    { 7, BOM_BLOQUEADA, BMOT_NINGUNO,  3, BEV_REARMAR,            true,  true,  50, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },
    { 8, BOM_BLOQUEADA, BMOT_NINGUNO,  3, BEV_REARMAR,            false, true,  50, BOM_BLOQUEADA, BMOT_NINGUNO,  BRECH_SIN_AGUA, false, 3 },
    { 8, BOM_CORTADA,   BMOT_SECO,     1, BEV_REARMAR,            false, true,  50, BOM_CORTADA,   BMOT_SECO,     BRECH_SIN_AGUA, false, 1 },
    { 9, BOM_CORTADA,   BMOT_ORDEN,    0, BEV_PERMITIR,           true,  true,  10, BOM_PERMITIDA, BMOT_NINGUNO,  BRECH_NINGUNO, false, 0 },
    { 9, BOM_CORTADA,   BMOT_ORDEN,    0, BEV_PERMITIR,           false, true,  50, BOM_CORTADA,   BMOT_ORDEN,    BRECH_SIN_AGUA, false, 0 },  /* FR-BOM-08 */
    { 0, BOM_CORTADA,   BMOT_SECO,     0, BEV_PERMITIR,           true,  true,  50, BOM_CORTADA,   BMOT_SECO,     BRECH_NINGUNO, false, 0 },
};

static bool bom_admitida(bom_estado_t s, bom_motivo_t m, bom_evento_t e, int fila)
{
    if (s == BOM_PERMITIDA) return (e == BEV_SECO && fila == 1) || (e == BEV_FLOTADOR_SIN_AGUA && fila == 2) ||
                                   (e == BEV_NIVEL && fila == 3) || (e == BEV_CORTAR && fila == 4);
    if (e == BEV_REARMAR) return fila == 7 || fila == 8;
    if (s == BOM_CORTADA && e == BEV_REARME_CUMPLIDO && m != BMOT_ORDEN) return fila == 5 || (fila == 6 && m == BMOT_SECO);
    if (s == BOM_CORTADA && m == BMOT_ORDEN && e == BEV_PERMITIR) return fila == 9;
    return false;
}

static void bom(void)
{
    const uint32_t T = 100000;
    for (size_t i = 0; i < sizeof BC / sizeof BC[0]; i++) {
        const bcaso_t *k = &BC[i];
        bom_t b; bom_iniciar(&b, 3, 10);
        b.estado = k->desde; b.motivo = k->m0;
        for (int j = 0; j < k->cortes0; j++) b.cortes[b.n_cortes++] = T - 3600 * (j + 1);
        bom_entrada_t e = { k->ev, k->agua, k->nv, k->pct, T };
        bom_accion_t a = bom_evento(&b, &e);
        OK(a.fila == k->fila && b.estado == k->hacia && b.motivo == k->m && a.rechazo == k->rech &&
           a.evento_bloqueada == k->bloq_ev && bom_intentos_24h(&b, T) == k->intentos && bom_rele(&b) == (k->hacia != BOM_PERMITIDA),
           "caso %zu (B%02d): fila %d, estado %d, motivo %d, rechazo %d, bloqueada %d, intentos %d",
           i, k->fila, a.fila, b.estado, b.motivo, a.rechazo, a.evento_bloqueada, bom_intentos_24h(&b, T));
    }
    /* completitud */
    int pares = 0;
    for (int s = 0; s <= BOM_BLOQUEADA; s++) for (int m = 0; m <= BMOT_ORDEN; m++) for (int ev = 0; ev < BEV_N_EVENTOS; ev++)
    for (int agua = 0; agua < 2; agua++) for (int nv = 0; nv < 2; nv++) for (int pct = 0; pct <= 100; pct += 5)
    for (int c = 0; c <= 3; c++) {
        if ((s == BOM_CORTADA) != (m != BMOT_NINGUNO)) continue;     /* motivo solo en CORTADA */
        bom_t b; bom_iniciar(&b, 3, 10); b.estado = (bom_estado_t)s; b.motivo = (bom_motivo_t)m;
        for (int j = 0; j < c; j++) b.cortes[b.n_cortes++] = T - 60 * (j + 1);
        bom_t antes = b;
        bom_entrada_t e = { (bom_evento_t)ev, agua, nv, (float)pct, T };
        bom_accion_t a = bom_evento(&b, &e);
        pares++;
        if (a.fila == 0) OK(!memcmp(&antes, &b, sizeof b) && a.rechazo == BRECH_NINGUNO && !a.evento_bloqueada,
                            "par ignorado con efecto: estado %d motivo %d evento %d", s, m, ev);
        else OK(bom_admitida(s, m, ev, a.fila), "transición no listada: estado %d motivo %d evento %d → B%02d", s, m, ev, a.fila);
        /* FR-BOM-08: nunca PERMITIDA con el flotador sin agua por una orden */
        if (!agua && (ev == BEV_REARMAR || ev == BEV_PERMITIR)) OK(b.estado != BOM_PERMITIDA || s == BOM_PERMITIDA, "orden a PERMITIDA sin agua");
    }
    printf("bom: %zu filas, %d combinaciones\n", sizeof BC / sizeof BC[0], pares);

    /* FR-BOM-05 (host): 35 %, flotador con agua 10 min → rearme a los 600 s; una interrupción del flotador a los 5 min reinicia */
    rea_t r = {0}; int a_los = -1;
    for (uint32_t t = 0; t <= 700 && a_los < 0; t++) if (rea_tick(&r, t, true, 35, true, 30, 600)) a_los = (int)t;
    OK(a_los == 600, "rearme a los %d s (esperado 600)", a_los);
    memset(&r, 0, sizeof r); a_los = -1;
    for (uint32_t t = 0; t <= 1000 && a_los < 0; t++) if (rea_tick(&r, t, true, 35, t != 300, 30, 600)) a_los = (int)t;
    OK(a_los == 901, "con interrupción a los 300 s: rearme a los %d s (esperado 901)", a_los);
    memset(&r, 0, sizeof r); int n = 0;
    for (uint32_t t = 0; t <= 1300; t++) n += rea_tick(&r, t, false, 35, true, 30, 600);   /* FR-BOM-06 */
    OK(n == 0, "rearme con el transductor en falla");

    /* FR-BOM-07 (host): 3 cortes por seco en 24 h simuladas → BLOQUEADA; solo rearmar la saca */
    bom_t b; bom_iniciar(&b, 3, 10);
    int bloqueada = 0;
    for (int k = 0; k < 3; k++) {
        bom_entrada_t s = { BEV_SECO, true, true, 50, (uint32_t)(1000 + k * 7200) };
        bom_evento(&b, &s);
        bom_entrada_t re = { BEV_REARME_CUMPLIDO, true, true, 50, (uint32_t)(1000 + k * 7200 + 600) };
        bloqueada += bom_evento(&b, &re).evento_bloqueada;
    }
    OK(b.estado == BOM_BLOQUEADA && bloqueada == 1, "3 cortes en 24 h: estado %d, avisos %d", b.estado, bloqueada);
    bom_entrada_t re = { BEV_REARME_CUMPLIDO, true, true, 50, 100000 };
    OK(bom_evento(&b, &re).fila == 0 && b.estado == BOM_BLOQUEADA, "rearme automático desde BLOQUEADA");
    bom_entrada_t rm = { BEV_REARMAR, true, true, 50, 100001 };
    OK(bom_evento(&b, &rm).fila == 7 && b.estado == BOM_PERMITIDA && bom_intentos_24h(&b, 100001) == 0, "rearmar no saca de BLOQUEADA");
    /* los cortes de hace más de 24 h no cuentan */
    bom_iniciar(&b, 3, 10);
    for (int k = 0; k < 3; k++) {
        bom_entrada_t s = { BEV_SECO, true, true, 50, (uint32_t)(k * 50000) };
        bom_evento(&b, &s);
        bom_entrada_t r2 = { BEV_REARME_CUMPLIDO, true, true, 50, (uint32_t)(k * 50000 + 600) };
        bom_evento(&b, &r2);
    }
    OK(b.estado == BOM_PERMITIDA, "3 cortes en más de 24 h bloquean (estado %d)", b.estado);
}

/* FR-BOM-03: corriente 0,8 A (i_seco 1,0), presión 0,2 bar → seco a los > 10 s; nunca con presión alta, sin calibrar o 1 s de menos */
static void seco(void)
{
    sec_t s = {0}; int a_los = -1;
    for (uint32_t t = 0; t <= 12000 && a_los < 0; t += 100) if (sec_muestra(&s, t, 0.8f, 0.2f, 0.5f, 1.0f, 0.5f, 10)) a_los = (int)t;
    OK(a_los == 10100, "seco a los %d ms (esperado el primer instante > 10 s)", a_los);
    memset(&s, 0, sizeof s); int n = 0;
    for (uint32_t t = 0; t <= 10000; t += 100) n += sec_muestra(&s, t, 0.8f, 0.2f, 0.5f, 1.0f, 0.5f, 10);
    OK(n == 0, "seco con exactamente t_seco_s (1 s de menos que > 10 s ya da 9,x)");
    memset(&s, 0, sizeof s); n = 0;
    for (uint32_t t = 0; t <= 30000; t += 100) n += sec_muestra(&s, t, 0.8f, 2.0f, 0.5f, 1.0f, 0.5f, 10);
    OK(n == 0, "seco con presión alta");
    memset(&s, 0, sizeof s); n = 0;
    for (uint32_t t = 0; t <= 30000; t += 100) n += sec_muestra(&s, t, 0.8f, 0.2f, 0.5f, 0.0f, 0.5f, 10);
    OK(n == 0, "seco con i_seco_a sin configurar");
    memset(&s, 0, sizeof s); n = 0;
    for (uint32_t t = 0; t <= 30000; t += 100) n += sec_muestra(&s, t, 1.2f, 0.2f, 0.5f, 1.0f, 0.5f, 10);
    OK(n == 0, "seco con corriente normal (≥ i_seco)");
    memset(&s, 0, sizeof s); n = 0;
    for (uint32_t t = 0; t <= 30000; t += 100) n += sec_muestra(&s, t, 0.3f, 0.2f, 0.5f, 1.0f, 0.5f, 10);
    OK(n == 0, "seco con la bomba parada (< i_marcha)");
    memset(&s, 0, sizeof s); n = 0;                 /* una muestra con presión alta a los 6 s reinicia la cuenta */
    for (uint32_t t = 0; t <= 15000; t += 100) n += sec_muestra(&s, t, 0.8f, t == 6000 ? 2.0f : 0.2f, 0.5f, 1.0f, 0.5f, 10);
    OK(n == 0, "la cuenta no se reinicia con una interrupción");
}

/* FR-LOC-04: las 256 combinaciones de condiciones */
static void loc(void)
{
    for (int m = 0; m < 256; m++) {
        loc_entrada_t e = { m & 1, m & 2, m & 4, m & 8, m & 16, m & 32, m & 64, m & 128 };
        int esp = (m & 1) ? 1 : (m & 30) ? 2 : (m & 96) ? 3 : (m & 128) ? 4 : 5;
        static const loc_led_t LED[] = { 0, LED_ROJO_FIJO, LED_ROJO_1HZ, LED_ROJO_025HZ, LED_VERDE_1HZ, LED_VERDE_FIJO };
        static const loc_zumbador_t ZUM[] = { 0, ZUM_1HZ, ZUM_3_PITIDOS_10MIN, ZUM_NINGUNO, ZUM_NINGUNO, ZUM_NINGUNO };
        loc_patron_t p = loc_patron(&e);
        OK(p.prioridad == esp && p.led == LED[esp] && p.zumbador == ZUM[esp], "combinación 0x%02x: prioridad %d (esperado %d)", m, p.prioridad, esp);
    }
}

/* FR-SON-01: cada banda y sus bordes (cada banda incluye su borde superior) */
static void son(void)
{
    const struct { float x; son_clase_t c; } K[] = {
        {0.00f, SON_CORTO}, {0.05f, SON_CORTO}, {0.0501f, SON_MOJADA}, {0.20f, SON_MOJADA}, {0.40f, SON_MOJADA},
        {0.4001f, SON_SECA}, {0.50f, SON_SECA}, {0.60f, SON_SECA}, {0.6001f, SON_INDETERMINADA}, {0.75f, SON_INDETERMINADA},
        {0.90f, SON_INDETERMINADA}, {0.9001f, SON_ABIERTA}, {1.00f, SON_ABIERTA},
    };
    for (size_t i = 0; i < sizeof K / sizeof K[0]; i++)
        OK(son_clasificar(K[i].x) == K[i].c, "%.4f → %s (esperado %s)", K[i].x, son_nombre(son_clasificar(K[i].x)), son_nombre(K[i].c));
}

int main(int argc, char **argv)
{
    const char *c = argc > 1 ? argv[1] : "";
    if (!strcmp(c, "val")) val();
    else if (!strcmp(c, "eje")) eje();
    else if (!strcmp(c, "bom")) bom();
    else if (!strcmp(c, "seco")) seco();
    else if (!strcmp(c, "loc")) loc();
    else if (!strcmp(c, "son")) son();
    else { printf("caso desconocido: %s\n", c); return 2; }
    printf("%s: %d fallos\n", c, fallos);
    return fallos != 0;
}
