/* L1 ordenes — FR-ORD-02. Núcleo puro. */
#include "ordenes.h"
#include <stdlib.h>
#include <string.h>

typedef struct { const char *p, *fin; } lex_t;

static void blancos(lex_t *l)
{
    while (l->p < l->fin && (*l->p == ' ' || *l->p == '\t' || *l->p == '\n' || *l->p == '\r')) l->p++;
}

static bool texto(lex_t *l, char *out, size_t cap)
{
    if (l->p >= l->fin || *l->p != '"') return false;
    size_t k = 0;
    for (l->p++; l->p < l->fin && *l->p != '"'; l->p++) {
        if (*l->p == '\\' || (unsigned char)*l->p < 0x20 || k + 1 >= cap) return false;
        out[k++] = *l->p;
    }
    if (l->p >= l->fin) return false;
    l->p++;
    out[k] = 0;
    return true;
}

static bool palabra(lex_t *l, const char *w)
{
    size_t n = strlen(w);
    if ((size_t)(l->fin - l->p) < n || memcmp(l->p, w, n)) return false;
    l->p += n;
    return true;
}

static bool numero(lex_t *l, double *v)
{
    char b[24];
    size_t k = 0;
    const char *ini = l->p;
    if (l->p < l->fin && *l->p == '-') b[k++] = *l->p++;
    if (l->p >= l->fin || *l->p < '0' || *l->p > '9') return false;
    while (l->p < l->fin && k + 1 < sizeof b &&
           ((*l->p >= '0' && *l->p <= '9') || *l->p == '.' || *l->p == 'e' || *l->p == 'E' || *l->p == '+' || *l->p == '-'))
        b[k++] = *l->p++;
    if (k + 1 >= sizeof b) return false;
    b[k] = 0;
    char *e;
    *v = strtod(b, &e);
    return e == b + k && l->p > ini;
}

bool ord_objeto(const char *p, size_t n, ord_obj_t *o)
{
    memset(o, 0, sizeof *o);
    if (!p || n > ORD_MAX_BYTES) return false;
    lex_t l = { p, p + n };
    blancos(&l);
    if (l.p >= l.fin || *l.p++ != '{') return false;
    blancos(&l);
    if (l.p < l.fin && *l.p == '}') { l.p++; blancos(&l); return l.p == l.fin; }
    for (;;) {
        if (o->n == ORD_MAX_CAMPOS) return false;
        ord_campo_t *c = &o->c[o->n];
        blancos(&l);
        if (!texto(&l, c->clave, sizeof c->clave)) return false;
        for (int i = 0; i < o->n; i++) if (!strcmp(o->c[i].clave, c->clave)) return false;   /* clave repetida */
        blancos(&l);
        if (l.p >= l.fin || *l.p++ != ':') return false;
        blancos(&l);
        if (l.p < l.fin && *l.p == '"') { c->tipo = ORDV_TEXTO; if (!texto(&l, c->texto, sizeof c->texto)) return false; }
        else if (palabra(&l, "true")) { c->tipo = ORDV_BOOL; c->b = true; }
        else if (palabra(&l, "false")) { c->tipo = ORDV_BOOL; c->b = false; }
        else { c->tipo = ORDV_NUMERO; if (!numero(&l, &c->num)) return false; }
        o->n++;
        blancos(&l);
        if (l.p < l.fin && *l.p == ',') { l.p++; continue; }
        if (l.p < l.fin && *l.p == '}') { l.p++; break; }
        return false;
    }
    blancos(&l);
    return l.p == l.fin;
}

const ord_campo_t *ord_campo(const ord_obj_t *o, const char *clave)
{
    for (int i = 0; i < o->n; i++) if (!strcmp(o->c[i].clave, clave)) return &o->c[i];
    return NULL;
}

const char *ord_motivo(ord_res_t r) { return r == ORD_PAYLOAD ? "payload" : r == ORD_ACCION ? "accion" : ""; }

static int indice(const char *v, const char *const *lista, int n)
{
    for (int i = 0; i < n; i++) if (!strcmp(v, lista[i])) return i;
    return -1;
}

/* accion de texto obligatoria y, salvo `permitidas`, ningún otro campo */
static ord_res_t accion(const ord_obj_t *o, const char *const *acciones, int na, int *ia, const char *otra)
{
    const ord_campo_t *a = ord_campo(o, "accion");
    if (!a || a->tipo != ORDV_TEXTO) return ORD_PAYLOAD;
    for (int i = 0; i < o->n; i++)
        if (strcmp(o->c[i].clave, "accion") && (!otra || strcmp(o->c[i].clave, otra))) return ORD_PAYLOAD;
    *ia = indice(a->texto, acciones, na);
    return *ia < 0 ? ORD_ACCION : ORD_OK;
}

ord_res_t ord_valvula(const char *p, size_t n, ord_valvula_t *a, ord_motivo_t *m)
{
    static const char *const A[] = { "cerrar", "abrir", "ejercicio", "posponer_ejercicio" };
    static const char *const M[] = { "fuga_remota", "casa_cerrada", "llegada", "manual" };
    ord_obj_t o; int ia;
    if (!ord_objeto(p, n, &o)) return ORD_PAYLOAD;
    ord_res_t r = accion(&o, A, 4, &ia, "motivo");
    if (r != ORD_OK) return r;
    *m = OM_NINGUNO;
    const ord_campo_t *mo = ord_campo(&o, "motivo");
    if (mo) {
        int im = mo->tipo == ORDV_TEXTO ? indice(mo->texto, M, 4) : -1;
        if (im < 0) return ORD_PAYLOAD;
        *m = (ord_motivo_t)(im + 1);
    }
    *a = (ord_valvula_t)ia;
    return ORD_OK;
}

ord_res_t ord_bomba(const char *p, size_t n, ord_bomba_t *a)
{
    static const char *const A[] = { "cortar", "permitir", "rearmar" };
    ord_obj_t o; int ia;
    if (!ord_objeto(p, n, &o)) return ORD_PAYLOAD;
    ord_res_t r = accion(&o, A, 3, &ia, NULL);
    if (r == ORD_OK) *a = (ord_bomba_t)ia;
    return r;
}

ord_res_t ord_config(const char *p, size_t n, char param[ORD_TXT], ord_campo_t *v)
{
    ord_obj_t o;
    if (!ord_objeto(p, n, &o) || o.n != 2) return ORD_PAYLOAD;
    const ord_campo_t *pa = ord_campo(&o, "param"), *va = ord_campo(&o, "v");
    if (!pa || !va || pa->tipo != ORDV_TEXTO || va->tipo == ORDV_TEXTO) return ORD_PAYLOAD;
    strcpy(param, pa->texto);
    *v = *va;
    return ORD_OK;
}
