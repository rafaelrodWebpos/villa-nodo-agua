"""Análisis de TC-WDT-01 (clima:NFR-WDT-01, fila bench): reinicios en una ventana larga y
de quién son. Núcleo puro (solo biblioteca estándar); lo prueba tests/host/test_estabilidad.py.

Fuente principal: los `status` en vivo del nodo (uptime_s, reset; uno por ciclo de 60 s en
OPERATIVO, clima:FR-MQTT-08). Un reinicio es un uptime_s que no crece; su causa es el `reset`
del status siguiente (§9.3), que el nodo lee de esp_reset_reason() y no depende de la consola
(upstream #80: la salida serie anterior a un reinicio puede perderse).

Reinicios del banco (upstream #83, #86; arreglo propuesto en el PR #87): en una ESP32-S3 con
USB nativo, el banco reinicia el chip al resetear por /api/serial/reset (dos veces) y al abrir
el puerto. Llegan como rst:0x15 (USB_UART_CHIP_RESET) → reset=usb; el reset por el JTAG del USB
(reset=jtag) también lo provoca solo el host. Esos no cuentan como fallo del firmware; se
informan. Cualquier otra causa (task_wdt, int_wdt, wdt, panic, brownout, sw, ext, poweron, otro)
cuenta como reinicio del nodo y la prueba falla.

Consola, de apoyo: líneas rst:0x.. (su código dice quién reinició) y task_wdt."""
import json
import re

CAUSAS_BANCO = {"usb", "jtag"}
RST = re.compile(r"rst:(0x[0-9a-fA-F]+)\s*\(([A-Z0-9_]+)\)")
RST_BANCO = {"USB_UART_CHIP_RESET", "USB_JTAG_CHIP_RESET"}


def status_vivos(msgs, base, desde=0.0, hasta=float("inf")):
    """[(t, status)] de los status en vivo (retain = 0, estado online) en [desde, hasta]."""
    out = []
    for m in msgs:
        if m["topic"] == f"{base}/status" and not m["retain"] and m["payload"] and desde <= m["t"] <= hasta:
            try:
                st = json.loads(m["payload"])
            except json.JSONDecodeError:
                continue
            if isinstance(st, dict) and st.get("estado") == "online":
                out.append((m["t"], st))
    return out


def reinicios_status(st):
    """[{t, uptime_antes, uptime_despues, reset, origen}] por cada uptime_s que no crece.
    origen: 'banco' si el reset es usb/jtag; 'nodo' en cualquier otro caso (y si falta)."""
    out = []
    for (t0, a), (t1, b) in zip(st, st[1:]):
        ua, ub = a.get("uptime_s"), b.get("uptime_s")
        if isinstance(ua, int) and isinstance(ub, int) and ub > ua:
            continue
        reset = b.get("reset")
        out.append({"t": t1, "uptime_antes": ua, "uptime_despues": ub, "reset": reset,
                    "origen": "banco" if reset in CAUSAS_BANCO else "nodo"})
    return out


def resets_ajenos(st, reset_inicial):
    """Valores de reset en los status que no son ni el inicial ni del banco, sin reinicio que
    los explique (un reset que cambia sin que uptime_s baje también es un fallo)."""
    vistos = set()
    previo = reset_inicial
    reinicio_en = {r["t"] for r in reinicios_status(st)}
    for t, s in st:
        r = s.get("reset")
        if r != previo and t not in reinicio_en:
            vistos.add(r)
        previo = r
    return sorted(vistos, key=str)


def rst_consola(lineas):
    """[(texto, código, nombre, origen)] de las líneas rst:0x.. de la consola."""
    out = []
    for ln in lineas:
        m = RST.search(ln)
        if m:
            out.append((ln.strip(), m.group(1).lower(), m.group(2),
                        "banco" if m.group(2) in RST_BANCO else "nodo"))
    return out


def huecos(st, cortes, ciclo_s=60, holgura_s=15, vuelta_s=150):
    """Intervalos entre status consecutivos > ciclo + holgura que no cubre ningún corte
    (un corte [t_corte, t_vuelta + vuelta_s] o un reinicio del banco explica el hueco)."""
    ventanas = [(k["t_corte"] - ciclo_s, k["t_vuelta"] + vuelta_s) for k in cortes]
    banco = {r["t"] for r in reinicios_status(st) if r["origen"] == "banco"}
    out = []
    for (t0, _), (t1, _) in zip(st, st[1:]):
        if t1 - t0 <= ciclo_s + holgura_s or t1 in banco:
            continue
        if any(a <= t0 and t1 <= b for a, b in ventanas):
            continue
        out.append((round(t0, 1), round(t1 - t0, 1)))
    return out


REINTENTOS_S = (5, 10, 20)
SIN_BANCO_MAX_S = 120


class Vigia:
    """Llamadas de vigilancia al banco en una prueba larga, tolerantes a errores transitorios
    (conexión reiniciada, timeout): hasta 3 reintentos tras 5, 10 y 20 s. Cada intento fallido
    queda en `eventos` para la evidencia. El banco cuenta como caído (not done, no fallo del
    nodo) solo si no responde durante más de SIN_BANCO_MAX_S seguidos. No toca los criterios
    de PASS/FAIL del nodo."""

    def __init__(self, transitorias, reintentos=REINTENTOS_S, max_sin_banco_s=SIN_BANCO_MAX_S,
                 reloj=None, dormir=None):
        import time as _t
        self.transitorias = tuple(transitorias)
        self.reintentos = tuple(reintentos)
        self.max_sin_banco_s = max_sin_banco_s
        self.reloj = reloj or _t.time
        self.dormir = dormir or _t.sleep
        self.eventos = []               # {t, llamada, intento, error} y {t, llamada, recuperado}
        self.sin_respuesta_desde = None

    def llamar(self, nombre, fn, *args):
        """(True, valor) si alguna de las 1 + 3 tentativas responde; (False, None) si no."""
        t0 = self.reloj()
        fallos = 0
        for i, espera in enumerate((0,) + self.reintentos):
            if espera:
                self.dormir(espera)
            try:
                valor = fn(*args)
            except self.transitorias as exc:
                fallos += 1
                self.eventos.append({"t": self.reloj(), "llamada": nombre, "intento": i + 1,
                                     "error": f"{type(exc).__name__}: {exc}"[:200]})
                continue
            if fallos or self.sin_respuesta_desde is not None:
                self.eventos.append({"t": self.reloj(), "llamada": nombre, "recuperado": True})
            self.sin_respuesta_desde = None
            return True, valor
        if self.sin_respuesta_desde is None:
            self.sin_respuesta_desde = t0
        return False, None

    def caido(self):
        """True si el banco lleva más de max_sin_banco_s seguidos sin responder."""
        return (self.sin_respuesta_desde is not None
                and self.reloj() - self.sin_respuesta_desde > self.max_sin_banco_s)
