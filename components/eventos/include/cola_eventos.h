/* L1 eventos — cola de eventos críticos (FSD 0.7.1, FR-PUB-04/05, P23). Núcleo puro.
 * Un evento crítico que ocurre fuera de OPERATIVO se guarda en RAM y sale al entrar en
 * OPERATIVO, después de los tópicos con retain, en orden de llegada y con `edad_s`.
 * Llena (32), se descarta el más antiguo que no sea `fuga` (si todos son `fuga`, el `fuga`
 * más antiguo) y se cuenta en `perdidos` (para `eventos_perdidos` del status). */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CEV_CAP 32                 /* P23 */
#define CEV_EXTRA_LEN 64

typedef struct {
    const char *tipo;              /* nombre de §13.1 (cadena estática) */
    char extra[CEV_EXTRA_LEN];     /* campos JSON adicionales ya formados, sin llaves (p. ej. "\"sonda\":3"), o "" */
    uint32_t t_ms;                 /* reloj monótono al ocurrir (para edad_s) */
    bool ts_valido;                /* la hora era válida al ocurrir */
    uint32_t ts;                   /* época Unix al ocurrir, si ts_valido */
} cev_t;

typedef struct {
    cev_t e[CEV_CAP];
    int ini, n;
    uint32_t perdidos;
} cola_ev_t;

/* ¿Es un evento crítico de FR-PUB-04? (los demás no se guardan) */
bool cev_es_critico(const char *tipo);
void cev_iniciar(cola_ev_t *c);
/* Guarda un evento crítico; false si no es crítico (no se guarda) */
bool cev_poner(cola_ev_t *c, const cev_t *ev);
/* Saca el más antiguo; false si la cola está vacía */
bool cev_sacar(cola_ev_t *c, cev_t *ev);
/* Payload del tópico `evento`: {"tipo":…,<extra>,"ts":…,"edad_s":…}; `ts` solo si era
 * válida. Devuelve la longitud o -1 si no cabe. */
int cev_json(char *buf, size_t cap, const cev_t *ev, uint32_t ahora_ms);
