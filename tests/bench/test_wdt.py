"""TC-WDT-01 — clima:NFR-WDT-01 (fila bench): 24 h de estabilidad. Portado del piloto
(villa-nodo-clima-piloto tests/bench/test_wdt.py, FR-HER-01) y adaptado a este nodo. Atómico y
muy largo (≈ 24 h + preparación): graba la imagen de la CI, aprovisiona contra un AP desechable
nuevo, deja el nodo en OPERATIVO y lo observa 24 h con 3 cortes de broker y 2 de Wi-Fi (AP del
banco apagado) de 5 min, todos por API: sin acción física ni bench-serial-write. Deja el AP y el
broker apagados al terminar. Solo el slot del DUT (dut.slot); nunca otro.

Observación (análisis en estabilidad.py, probado en host):
- Fuente principal, los status en vivo (uno por ciclo de 60 s en OPERATIVO): un uptime_s que no
  crece es un reinicio y el reset del status siguiente dice su causa. reset=usb/jtag
  (rst:0x15 USB_UART_CHIP_RESET) es un reinicio del banco (upstream #83, #86; PR #87): se informa
  y no cuenta como fallo del firmware. Cualquier otra causa (task_wdt incluida) falla.
- Consola, de apoyo: ninguna línea con task_wdt y ningún rst: cuyo código no sea del banco
  (la consola puede perder la salida anterior a un reinicio, upstream #80: por eso manda el
  status).
- Tras cada corte, un status en vivo en ≤ 150 s; fuera de los cortes y de los reinicios del
  banco, ningún hueco entre status > 75 s.
- Diferencias con el piloto: este nodo no tiene aún sensores (lecturas = 0 y sin línea med:
  periódica), así que no hay Δlecturas ni control med: de la consola; el control positivo es el
  status de cada ciclo.
- Si el AP o el broker cambian de manos durante la prueba (otro proyecto del banco), la prueba
  para como not done sin tocar lo ajeno.

Corrida de humo (no cuenta como resultado): NODO_WDT_HUMO=1 → 15 min con un corte de broker."""
import os
import time

import pytest

import estabilidad as est
from benchlib import case
from test_mqtt import _a_operativo, _nodo, _suscribir, configurado

CICLO_S = 60
CORTE_S = 300
VUELTA_S = 150
HOLGURA_S = 15
LECTURA_CADA_S = 30

HUMO = os.environ.get("NODO_WDT_HUMO") == "1"
if HUMO:
    DURACION_S = 15 * 60
    CORTES = [(5 * 60, "broker")]
else:
    DURACION_S = 24 * 3600
    CORTES = [(2 * 3600, "broker"), (6 * 3600, "wifi"), (10 * 3600, "broker"),
              (14 * 3600, "wifi"), (18 * 3600, "broker")]


def _hora(t):
    return time.strftime("%Y-%m-%d %H:%M:%S %z", time.localtime(t))


try:
    from testbench_driver import CommandTimeout
    TRANSITORIAS = (OSError, CommandTimeout)
except ImportError:          # sin el driver (lectura en host): solo errores de red
    TRANSITORIAS = (OSError,)


def _vigilar(tb, banco, ssid, en_corte, vigia):
    """None si el AP y el broker siguen como los dejó la prueba (o si el banco no respondió
    tras los reintentos: eso lo decide vigia.caido()); si no, el motivo."""
    ok_ap, ap = vigia.llamar("GET /api/wifi/ap_status", tb.ap_status)
    ok_mq, mq = vigia.llamar("GET /api/mqtt/status", tb.mqtt_status)
    if not (ok_ap and ok_mq):
        return None
    if en_corte != "wifi" and (not ap.get("active") or ap.get("ssid") != ssid):
        return "el AP del banco ya no es el de la prueba"
    if en_corte != "broker" and not mq.get("running"):
        return "el broker del banco se apagó sin la prueba"
    return None


def test_tc_wdt_01_24h_stability(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-WDT-01 · clima:NFR-WDT-01 (bench) — 24 h con 3 cortes de broker y 2 de Wi-Fi de 5 min:
    ningún reinicio del nodo (los del banco, reset=usb, se informan aparte), ningún task_wdt,
    status en vivo tras cada corte en ≤ 150 s y sin huecos fuera de los cortes."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    nombre = "Estabilidad 24 h" if not HUMO else "Estabilidad (corrida de humo, 15 min)"
    with case(tb, "TC-WDT-01", nombre, run_dir, spec="TC-WDT-01 villa-nodo-agua") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        if HUMO:
            c.note("- **corrida de humo** (NODO_WDT_HUMO=1): no cuenta como resultado de TC-WDT-01", "")
        c.note(f"- ventana: {DURACION_S} s; cortes de {CORTE_S} s: "
               + ", ".join(f"{k} a t={t} s" for t, k in CORTES), "")
        banco.broker_on()
        cortes, ajeno = [], None
        vigia = est.Vigia(TRANSITORIAS)   # reintentos 5/10/20 s; caído tras > 2 min seguidos
        with _suscribir(tb, nodo) as sub:
            t0, reg = _a_operativo(tb, dut_slot, banco, c)
            fin = time.time() + 150
            while time.time() < fin and len(est.status_vivos(sub.mensajes(), base, t0)) < 2:
                time.sleep(1)
            assert est.status_vivos(sub.mensajes(), base, t0), "precondición: ningún status en vivo tras T11"
            t_ini = time.time()
            t_fin = t_ini + DURACION_S
            c.step(f"Ventana de {DURACION_S // 3600} h {DURACION_S % 3600 // 60} min con {len(CORTES)} cortes; "
                   f"inicio {_hora(t_ini)}")
            c.note(f"- inicio de la ventana: {_hora(t_ini)}; fin previsto: {_hora(t_fin)}", "")
            pendientes = list(CORTES)
            caido = False

            def sin_banco():
                if vigia.caido():
                    return (f"el banco no respondió durante más de {est.SIN_BANCO_MAX_S} s seguidos "
                            f"(desde {_hora(vigia.sin_respuesta_desde)})")
                return None

            try:
                while time.time() < t_fin and not ajeno:
                    vigia.llamar("GET /api/serial/output", reg.leer)
                    ajeno = _vigilar(tb, banco, banco.ssid, None, vigia) or sin_banco()
                    if ajeno:
                        break
                    ahora = time.time()
                    if pendientes and ahora - t_ini >= pendientes[0][0]:
                        _, tipo = pendientes.pop(0)
                        c.step(f"Corte de {tipo} {CORTE_S // 60} min (t={ahora - t_ini:.0f} s, {_hora(ahora)})")
                        t_c = time.time()
                        banco.broker_off() if tipo == "broker" else banco.ap_off()
                        while time.time() < t_c + CORTE_S:
                            vigia.llamar("GET /api/serial/output", reg.leer)
                            time.sleep(min(LECTURA_CADA_S, max(1.0, t_c + CORTE_S - time.time())))
                        t_v = time.time()
                        banco.broker_on() if tipo == "broker" else banco.ap_on()
                        cortes.append({"tipo": tipo, "t_corte": t_c, "t_vuelta": t_v})
                        continue
                    time.sleep(min(LECTURA_CADA_S, max(1.0, t_fin - time.time())))
            except TRANSITORIAS as exc:
                # Una orden de corte o de vuelta que el banco no atiende: es del banco, no del nodo
                ajeno = f"error del banco: {type(exc).__name__}: {exc}"[:300]
            caido = bool(ajeno) and ajeno.startswith(("el banco no respondió", "error del banco"))
            t_real_fin = time.time()
            time.sleep(5)
            vigia.llamar("GET /api/serial/output", reg.leer)
            msgs = sub.mensajes()
        if ajeno and not caido:
            # Lo ajeno no se toca: limpiar() no apaga lo que ya no es de la prueba
            ok, ap = vigia.llamar("GET /api/wifi/ap_status", tb.ap_status)
            banco.ap = banco.ap and ok and ap.get("ssid") == banco.ssid
            banco.broker = False
        elif caido:
            # El AP y el broker son nuestros: se intenta apagarlos; si el banco sigue sin
            # responder, se deja anotado y el --check del siguiente arranque lo detecta
            try:
                banco.limpiar()
            except TRANSITORIAS as exc:
                banco.ap = banco.broker = False
                c.note(f"- limpieza del AP y del broker no hecha (banco sin responder: {type(exc).__name__})")
        ok, _ = vigia.llamar("GET /api/serial/output", reg.leer)
        ent = reg.entradas
        c.save_log("serie", [e["text"] for e in ent])
        c.save_log("mqtt", [f"{m['t'] - t0:+.2f} s retain={int(m['retain'])} qos={m['qos']} {m['topic']} {m['payload']}"
                            for m in msgs])
        try:
            desde = time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime(t0))
            c.save_log("banco", [f"{e.get('ts')} {e.get('msg')}" for e in tb.get_log(since=desde)
                                 if dut_slot in str(e.get("msg", ""))])
        except Exception as exc:   # el registro del banco es apoyo, no resultado
            c.note(f"- registro de actividad del banco no disponible: {type(exc).__name__}")

        c.note(f"- fin de la ventana: {_hora(t_real_fin)} ({(t_real_fin - t_ini) / 3600:.2f} h)", "")
        fallidos = [x for x in vigia.eventos if not x.get("recuperado")]
        c.note(f"- errores transitorios del banco (reintentos 5/10/20 s; no cuentan para el nodo): "
               f"{len(fallidos)} intentos fallidos, "
               f"{sum(1 for x in vigia.eventos if x.get('recuperado'))} recuperaciones", "")
        c.save_log("banco-transitorios",
                   [f"{_hora(x['t'])} {x['llamada']} " + ("recuperado" if x.get("recuperado")
                                                         else f"intento {x['intento']}: {x['error']}")
                    for x in vigia.eventos] or ["ninguno"])
        if ajeno:
            origen = "fallo de comunicación con el banco" if caido else "otro uso del banco"
            pytest.skip(f"not done: {ajeno}, a las {_hora(t_real_fin)} ({origen}); no es fallo del firmware")

        st = est.status_vivos(msgs, base, t0)
        ventana = [x for x in st if t_ini - CICLO_S <= x[0] <= t_fin + 5]
        a, d = ventana[0][1], ventana[-1][1]
        reinicios = est.reinicios_status(st)
        del_nodo = [r for r in reinicios if r["origen"] == "nodo"]
        del_banco = [r for r in reinicios if r["origen"] == "banco"]
        ajenos = est.resets_ajenos(st, st[0][1].get("reset"))
        textos = [e["text"] for e in ent if e.get("ts", 0) >= t_ini - CICLO_S]
        rst = est.rst_consola(textos)
        rst_nodo = [x for x in rst if x[3] == "nodo"]
        wdt = [t.strip() for t in textos if "task_wdt" in t]
        huecos = est.huecos(ventana, cortes, CICLO_S, HOLGURA_S, VUELTA_S)
        vueltas = []
        for k in cortes:
            nxt = next((t for t, _ in st if t > k["t_vuelta"]), None)
            vueltas.append((k["tipo"], round(k["t_corte"] - t_ini), None if nxt is None else round(nxt - k["t_vuelta"], 1)))
        c.note("", "## Resultado", "",
               f"- status en vivo: {len(st)} ({len(ventana)} en la ventana); primero: uptime_s={a.get('uptime_s')}, "
               f"reset={a.get('reset')}; último: uptime_s={d.get('uptime_s')}, reset={d.get('reset')}",
               *[f"- corte de {tipo} a t={tc} s: primer status en vivo a "
                 + (f"{v} s de devolverlo" if v is not None else "— (ninguno)") for tipo, tc, v in vueltas],
               f"- reinicios del nodo: {[(_hora(r['t']), r['reset']) for r in del_nodo] or 'ninguno'}",
               f"- reinicios del banco (reset=usb/jtag, no cuentan; #83/#86): "
               f"{[(_hora(r['t']), r['reset']) for r in del_banco] or 'ninguno'}",
               f"- reset que cambia sin reinicio: {ajenos or 'ninguno'}",
               f"- consola: rst: {[(x[1], x[2]) for x in rst] or 'ninguno'}; líneas con task_wdt: {len(wdt)}",
               f"- huecos entre status fuera de cortes y reinicios del banco: {huecos or 'ninguno'}")
        assert not wdt, f"reinicio por watchdog (consola): {wdt[:3]}"
        assert not del_nodo, f"reinicio del nodo: {[(r['reset'], r['uptime_antes'], r['uptime_despues']) for r in del_nodo]}"
        assert not rst_nodo, f"rst: que no es del banco en la consola: {rst_nodo[:3]}"
        assert not ajenos, f"reset cambió sin reinicio: {ajenos}"
        assert len(cortes) == len(CORTES), f"solo {len(cortes)} de {len(CORTES)} cortes hechos"
        assert all(v is not None and v <= VUELTA_S for _, _, v in vueltas), f"recuperación tras un corte > {VUELTA_S} s: {vueltas}"
        assert not huecos, f"huecos entre status (ciclos sin publicar) fuera de los cortes: {huecos[:5]}"
        c.passed(f"{(t_real_fin - t_ini) / 3600:.2f} h ({_hora(t_ini)} → {_hora(t_real_fin)}), {len(cortes)} cortes: "
                 f"0 reinicios del nodo, {len(del_banco)} del banco, reset={a.get('reset')}")
