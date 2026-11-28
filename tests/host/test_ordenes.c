/* TC-ORD-A1 · FR-ORD-02 (host): análisis estricto de valvula/set, bomba/set y config/set */
#include <stdio.h>
#include <string.h>
#include "ordenes.h"

static int fallos;
#define OK(c, ...) do { if (!(c)) { fallos++; printf("FALLO %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
#define L(s) s, strlen(s)

int main(void)
{
    ord_valvula_t a; ord_motivo_t m; ord_bomba_t b; char p[ORD_TXT]; ord_campo_t v;
    OK(ord_valvula(L("{\"accion\":\"cerrar\",\"motivo\":\"fuga_remota\"}"), &a, &m) == ORD_OK && a == OV_CERRAR && m == OM_FUGA_REMOTA, "cerrar fuga_remota");
    OK(ord_valvula(L("{\"accion\":\"abrir\"}"), &a, &m) == ORD_OK && a == OV_ABRIR && m == OM_NINGUNO, "abrir");
    OK(ord_valvula(L("{\"accion\":\"posponer_ejercicio\"}"), &a, &m) == ORD_OK && a == OV_POSPONER, "posponer");
    OK(ord_valvula(L("{\"accion\":\"ejercicio\",\"motivo\":\"manual\"}"), &a, &m) == ORD_OK && a == OV_EJERCICIO && m == OM_MANUAL, "ejercicio");
    /* FR-ORD-02: 300 bytes, `{`, acción desconocida, acción numérica */
    static char largo[301]; memset(largo, ' ', 300); memcpy(largo, "{\"accion\":\"abrir\"}", 18);
    const struct { const char *p; ord_res_t r; } V[] = {
        { "{", ORD_PAYLOAD }, { "{\"accion\":\"x\"}", ORD_ACCION }, { "{\"accion\":5}", ORD_PAYLOAD },
        { "", ORD_PAYLOAD }, { "{}", ORD_PAYLOAD }, { "{\"accion\":\"abrir\",\"motivo\":\"otro\"}", ORD_PAYLOAD },
        { "{\"accion\":\"abrir\",\"motivo\":3}", ORD_PAYLOAD }, { "{\"accion\":\"abrir\",\"extra\":1}", ORD_PAYLOAD },
        { "{\"accion\":\"abrir\",\"accion\":\"cerrar\"}", ORD_PAYLOAD }, { "{\"accion\":\"abrir\"} x", ORD_PAYLOAD },
        { "{\"accion\":true}", ORD_PAYLOAD }, { "[\"abrir\"]", ORD_PAYLOAD }, { "{\"accion\":\"ab\\\"rir\"}", ORD_PAYLOAD },
    };
    for (size_t k = 0; k < sizeof V / sizeof V[0]; k++)
        OK(ord_valvula(L(V[k].p), &a, &m) == V[k].r, "valvula «%s»: %d (esperado %d)", V[k].p, ord_valvula(L(V[k].p), &a, &m), V[k].r);
    OK(ord_valvula(largo, 300, &a, &m) == ORD_PAYLOAD, "300 bytes aceptados");
    OK(ord_bomba(L("{\"accion\":\"rearmar\"}"), &b) == ORD_OK && b == OB_REARMAR, "rearmar");
    OK(ord_bomba(L("{\"accion\":\"cortar\"}"), &b) == ORD_OK && b == OB_CORTAR, "cortar");
    OK(ord_bomba(L("{\"accion\":\"permitir\"}"), &b) == ORD_OK && b == OB_PERMITIR, "permitir");
    OK(ord_bomba(L("{\"accion\":\"abrir\"}"), &b) == ORD_ACCION, "abrir en bomba/set");
    OK(ord_bomba(L("{\"accion\":\"cortar\",\"motivo\":\"manual\"}"), &b) == ORD_PAYLOAD, "campo extra en bomba/set");
    /* config/set: forma */
    OK(ord_config(L("{\"param\":\"t_seco_s\",\"v\":15}"), p, &v) == ORD_OK && !strcmp(p, "t_seco_s") && v.tipo == ORDV_NUMERO && v.num == 15, "t_seco_s=15");
    OK(ord_config(L("{\"v\":false,\"param\":\"casa_cerrada\"}"), p, &v) == ORD_OK && v.tipo == ORDV_BOOL && !v.b, "casa_cerrada=false");
    OK(ord_config(L("{\"param\":\"p_min_bar\",\"v\":0.75}"), p, &v) == ORD_OK && v.num == 0.75, "decimal");
    OK(ord_config(L("{\"param\":\"x\",\"v\":\"1\"}"), p, &v) == ORD_PAYLOAD, "valor de texto");
    OK(ord_config(L("{\"param\":5,\"v\":1}"), p, &v) == ORD_PAYLOAD, "param numérico");
    OK(ord_config(L("{\"param\":\"x\"}"), p, &v) == ORD_PAYLOAD, "sin v");
    OK(ord_config(L("{\"param\":\"x\",\"v\":1,\"w\":2}"), p, &v) == ORD_PAYLOAD, "campo extra");
    OK(ord_config(L("{\"param\":\"x\",\"v\":1e}"), p, &v) == ORD_PAYLOAD, "número mal formado");
    OK(!strcmp(ord_motivo(ORD_PAYLOAD), "payload") && !strcmp(ord_motivo(ORD_ACCION), "accion"), "motivos");
    printf("ordenes: %d fallos\n", fallos);
    return fallos != 0;
}
