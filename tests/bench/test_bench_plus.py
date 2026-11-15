"""Nivel bench⁺ heredado del piloto (FR-HER-01): contratos de hora que necesitan la salida a
Internet del AP del banco (bench-ap-internet) y el bloqueo de SNTP (bench-sntp-block). Sin E/S
todavía, se observan en `status` (hora_valida) y en el log; la parte de las medidas con ts
(clima:FR-MQTT-03, clima:FR-TIME-04 «todas las medidas llevan ts») llega con los drivers de E/S.
Atómicos: graban la imagen, aprovisionan contra un AP desechable nuevo y dejan el AP, el broker,
la salida a Internet y el bloqueo de SNTP apagados al terminar."""
import json
import os
import re
import time
import urllib.error
import urllib.request

from benchlib import case
from test_mqtt import _a_operativo, _ms, _nodo, _suscribir, configurado

WIFI_UP = "con: CONECTANDO_WIFI -> CONECTANDO_BROKER (wifi_up)"
SINC = re.compile(r"sntp: sincronizado ts=(\d+)")
SINC_LIMITE_S = 30          # clima:FR-TIME-01
BLOQUEO_S = 7200            # clima:FR-TIME-04: SNTP bloqueado 2 h
LECTURA_CADA_S = 30


def _api(method, path, body=None, timeout=20):
    data = json.dumps(body).encode() if body is not None else None
    cab = {"Content-Type": "application/json"} if data else {}
    if os.environ.get("TESTBENCH_TOKEN"):
        cab["X-Harness-Token"] = os.environ["TESTBENCH_TOKEN"]
    req = urllib.request.Request(os.environ["TESTBENCH_URL"].rstrip("/") + path, data=data, method=method, headers=cab)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, json.loads(r.read() or b"{}")
    except urllib.error.HTTPError as e:
        return e.code, json.loads(e.read() or b"{}")


def _estado():
    code, st = _api("GET", "/api/wifi/internet")
    assert code == 200 and st.get("ok"), f"GET /api/wifi/internet → {code} {st}"
    return st


def _sntp_block(on):
    code, st = _api("POST", "/api/wifi/sntp_block", {"blocked": on})
    assert code == 200 and st.get("ok"), f"POST sntp_block {on} → {code} {st}"
    return st


def _apagar_fork():
    _api("POST", "/api/wifi/sntp_block", {"blocked": False})
    _api("POST", "/api/wifi/internet", {"enabled": False})


def _status(msgs, base, desde=0.0):
    out = []
    for m in msgs:
        if m["t"] >= desde and not m["retain"] and m["topic"] == f"{base}/status" and m["payload"]:
            try:
                p = json.loads(m["payload"])
            except ValueError:
                continue
            if p.get("estado") == "online":
                out.append((m, p))
    return out


def _guardar(c, reg, msgs, t0):
    ent = reg.leer()
    c.save_log("serie", [e["text"] for e in ent])
    c.save_log("mqtt", [f"{m['t'] - t0:+.2f} s retain={int(m['retain'])} qos={m['qos']} {m['topic']} {m['payload']}"
                        for m in msgs])
    return ent


def test_tc_time_01_sntp_sync(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-TIME-01 · clima:FR-TIME-01 (bench⁺) — AP con salida a Internet: «sntp: sincronizado»
    ≤ 30 s después de wifi_up (marcas del DUT) y hora_valida = true en el primer status posterior
    (la parte de hora de la JRN-01.6 del piloto)."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    with case(tb, "TC-TIME-01", "Hora por SNTP", run_dir, spec="TC-TIME (villa-nodo-agua)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        try:
            with _suscribir(tb, nodo) as sub:
                t0, reg = _a_operativo(tb, dut_slot, banco, c, internet=True)
                st = _estado()
                c.step("Esperar la sincronización y un status del ciclo posterior (≤ 140 s)")
                fin = time.time() + 140
                while time.time() < fin:
                    if any(p.get("hora_valida") is True for _, p in _status(sub.mensajes(), base, t0)):
                        break
                    time.sleep(2)
                time.sleep(2)
                msgs = sub.mensajes()
        finally:
            _apagar_fork()
        ent = _guardar(c, reg, msgs, t0)
        textos = [e["text"].strip() for e in ent]
        up = next((t for t in reversed(textos) if WIFI_UP in t), None)
        sinc = next((t for t in textos if SINC.search(t) and up and (_ms(t) or 0) >= (_ms(up) or 0)), None)
        dt_sinc = (_ms(sinc) - _ms(up)) / 1000 if up and sinc else None
        st_tras = next(((m, p) for m, p in _status(msgs, base, t0) if sinc and p.get("uptime_s", 0) * 1000 >= _ms(sinc)), None)
        c.note("", "## Resultado", "",
               f"- salida del AP: {st['internet']} (rules_ok {st['rules_ok']})",
               f"- wifi_up: `{up}`", f"- sincronización: `{sinc}` → {dt_sinc} s desde wifi_up (≤ {SINC_LIMITE_S} s)",
               "- primer status posterior: " + (f"uptime_s={st_tras[1]['uptime_s']}, hora_valida={st_tras[1].get('hora_valida')}" if st_tras else "ninguno"))
        assert up and sinc, "sin wifi_up o sin «sntp: sincronizado» en el último arranque"
        assert dt_sinc <= SINC_LIMITE_S, f"sincronizó a {dt_sinc} s de wifi_up"
        assert st_tras and st_tras[1].get("hora_valida") is True, f"primer status tras sincronizar: {st_tras}"
        c.passed(f"sincroniza a {dt_sinc} s de wifi_up; hora_valida true en el status siguiente")


def test_tc_time_04_resync_failure_keeps_time(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-TIME-04 · clima:FR-TIME-04 (bench⁺) — hora válida; SNTP bloqueado 2 h: hora_valida sigue
    en true en todos los status; el log registra «sntp: fallo» en el intento de resincronización;
    sin reinicios. (Las medidas con ts entero, con los drivers de E/S.)"""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    with case(tb, "TC-TIME-04", "Resincronización fallida sin perder la hora", run_dir,
              spec="TC-TIME heredado (villa-nodo-clima-piloto)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        try:
            with _suscribir(tb, nodo) as sub:
                t0, reg = _a_operativo(tb, dut_slot, banco, c, internet=True)
                c.step("Esperar la sincronización")
                fin = time.time() + 90
                sinc = None
                while time.time() < fin and not sinc:
                    sinc = next((e for e in reg.leer() if SINC.search(e.get("text", ""))), None)
                    time.sleep(1)
                assert sinc, "no sincronizó con la salida encendida"
                time.sleep(65)              # un status con hora válida antes del bloqueo
                c.step(f"Bloquear SNTP {BLOQUEO_S // 3600} h")
                t_b = time.time()
                est_b = _sntp_block(True)
                while time.time() < t_b + BLOQUEO_S:
                    reg.leer()
                    time.sleep(min(LECTURA_CADA_S, max(1.0, t_b + BLOQUEO_S - time.time())))
                c.step("Desbloquear SNTP")
                est_u = _sntp_block(False)
                time.sleep(5)
                msgs = sub.mensajes()
        finally:
            _apagar_fork()
        ent = _guardar(c, reg, msgs, t0)
        st = [p for _, p in _status(msgs, base, t_b)]
        falsas = [p.get("hora_valida") for p in st if p.get("hora_valida") is not True]
        tb_ent = [e for e in ent if e.get("ts", 0) >= t_b]
        fallos = [e["text"].strip() for e in tb_ent if "sntp: fallo" in e["text"]]
        inicios = [e["text"].strip() for e in tb_ent if "sntp: inicio" in e["text"]]
        sincs = [e["text"].strip() for e in tb_ent if SINC.search(e["text"])]
        boots = [e["text"].strip() for e in tb_ent if "boot: " in e["text"] and " fw=" in e["text"]]
        c.note("", "## Resultado", "",
               f"- sincronización inicial: `{sinc['text'].strip()}`",
               f"- bloqueo: {est_b['sntp_blocked']} (rules_ok {est_b['rules_ok']}); desbloqueo: {not est_u['sntp_blocked']}",
               f"- en las {BLOQUEO_S // 3600} h: {len(st)} status, {len(falsas)} con hora_valida ≠ true",
               f"- «sntp: inicio» {len(inicios)}: {inicios[:4]}", f"- «sntp: fallo» {len(fallos)}: {fallos[:4]}",
               f"- sincronizaciones durante el bloqueo: {len(sincs)}; arranques: {len(boots)}")
        assert len(st) >= BLOQUEO_S // 60 - 5, f"solo {len(st)} status en {BLOQUEO_S // 60} min"
        assert not falsas, f"hora_valida dejó de ser true: {falsas[:3]}"
        assert fallos, "ningún «sntp: fallo» en el intento de resincronización"
        assert not sincs, f"sincronizó con SNTP bloqueado: {sincs}"
        assert not boots, f"el nodo se reinició: {boots}"
        c.passed(f"{len(st)} status con hora_valida = true en 2 h de bloqueo; {len(fallos)} «sntp: fallo»")
