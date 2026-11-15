"""Heredados del piloto (villa-nodo-clima-piloto, FR-HER-01) que no necesitan E/S:
autenticación del broker y log UDP. Atómicos: cada test graba la imagen de la CI y
aprovisiona contra un AP desechable nuevo con el broker del banco.

TC-MQTT-05 · clima:FR-MQTT-07: broker con autenticación; con credenciales que no acepta, el DUT
  sigue en CONECTANDO_BROKER con backoff; con las suyas, OPERATIVO.
TC-LOG-07 · clima:FR-LOG-02: con IP, cada línea serie llega al receptor UDP del banco en ≤ 1 s
  durante 3 ciclos (la comprobación que el piloto hace en su JRN-01.3).
TC-LOG-08 · clima:FR-LOG-02 (fila de captura): imagen de banco; los datagramas de log van por
  unicast solo a 192.168.4.1:5555.
TC-LOG-04 · clima:FR-LOG-01: imagen de PRODUCCIÓN; en el arranque y 3 ciclos solo DHCP, DNS y
  SNTP por UDP (ninguno al 5555). Se lanza aparte, con --firmware-dir de la imagen de producción."""
import re
import secrets
import time

import pytest

from benchlib import Registro, case
from test_mqtt import configurado

T08 = "con: CONECTANDO_WIFI -> CONECTANDO_BROKER (wifi_up)"
T11 = "con: CONECTANDO_BROKER -> OPERATIVO (mqtt_up)"
LOG_HOST, LOG_PORT = "192.168.4.1", 5555
UDP_PERMITIDOS = {53, 67, 68, 123}       # DNS, DHCP y SNTP (clima:FR-LOG-01)
UDP_RETARDO_S = 1.0                      # clima:FR-LOG-02
CICLO_S, CICLOS = 60, 3


def test_tc_mqtt_05_broker_authentication(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-MQTT-05 · clima:FR-MQTT-07 — credenciales rechazadas: backoff sin OPERATIVO; aceptadas: OPERATIVO."""
    tb = testbench
    usuario, clave, otra = "nodo-prueba", secrets.token_hex(12), secrets.token_hex(12)
    with case(tb, "TC-MQTT-05", "Broker con autenticación", run_dir, spec="TC-MQTT heredado (villa-nodo-clima-piloto)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path, mqtt_user=usuario, mqtt_pass=clave) as banco:
        c.secretos = list(c.secretos) + [(otra, "<otra-clave-de-prueba>")]
        try:
            c.step("Broker con autenticación que NO acepta las credenciales del DUT; 90 s")
            tb.mqtt_start(usuario, otra)
            banco.broker = True
            t0 = time.time()
            reg = Registro(tb, dut_slot, t0)
            tb.serial_reset(dut_slot)
            banco.ap_on()
            time.sleep(90)
            txt1 = [e.get("text", "") for e in reg.leer()]
            intentos = [t for t in txt1 if re.search(r"nodo: mqtt: intento \d+", t)]
            op1 = [t for t in txt1 if T11 in t]
            c.note(f"- con credenciales rechazadas: {len(intentos)} intentos MQTT, T11: {len(op1)}")
            c.step("Broker con las credenciales del DUT; esperar T11")
            tb.mqtt_stop()
            t1 = time.time()
            tb.mqtt_start(usuario, clave)
            ok = reg.esperar(re.escape(T11), 120, desde_ts=t1)
            txt = [e.get("text", "") for e in reg.leer()]
        finally:
            try:
                tb.mqtt_stop()
            except Exception:
                pass
            banco.broker = False
        c.save_log("serie", txt)
        ms = [int(re.match(r"\s*I \((\d+)\)", t).group(1)) for t in intentos if re.match(r"\s*I \((\d+)\)", t)]
        gaps = [round((b - a) / 1000, 1) for a, b in zip(ms, ms[1:])]
        c.note(f"- intervalos entre intentos (s): {gaps}", f"- con las credenciales aceptadas: T11 {'sí' if ok else 'no'}")
        assert not op1, "llegó a OPERATIVO con credenciales rechazadas"
        assert len(intentos) >= 3 and gaps and gaps[-1] > gaps[0], f"sin backoff creciente: {gaps}"
        assert ok, "sin T11 con las credenciales aceptadas"
        assert not any(clave in t or otra in t for t in txt), "una clave salió por la consola (clima:FR-SEC-03)"
        c.passed(f"rechazadas: {len(intentos)} intentos con backoff {gaps}; aceptadas: OPERATIVO")


def test_tc_log_07_udp_copy(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-LOG-07 · clima:FR-LOG-02 — con IP, cada línea serie tiene copia en el receptor UDP del
    banco en ≤ 1 s durante 3 ciclos; E11 «UDP logging -> host:port» llega al receptor."""
    tb = testbench
    if firmware["variant"] == "production":
        pytest.skip("not done: TC-LOG-07 necesita la imagen de banco")
    with case(tb, "TC-LOG-07", "Copia del log por UDP", run_dir, spec="TC-LOG (villa-nodo-agua)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        c.step("AP del banco arriba (sin broker) y reset; observar 3 ciclos")
        t0 = time.time()
        reg = Registro(tb, dut_slot, t0)
        tb.serial_reset(dut_slot)
        banco.ap_on()
        t_ip = reg.esperar(re.escape(T08), 60)
        assert t_ip, "sin T08 en 60 s"
        time.sleep(CICLOS * CICLO_S + 5)
        vistas = reg.leer()
        udp = tb.udplog(limit=3000)
        c.save_log("serie", [e.get("text", "") for e in vistas])
        c.save_log("udp", [str(u.get("line", u) if isinstance(u, dict) else u) for u in udp])

        def txt(u): return str(u.get("line", u.get("text", "")) if isinstance(u, dict) else u).strip()
        def ts(u): return float(u.get("ts", 0)) if isinstance(u, dict) else 0.0
        assert any("UDP logging -> " in txt(u) for u in udp if ts(u) >= t0), "el receptor UDP no recibió E11"
        i = next(k for k, e in enumerate(vistas) if T08 in e.get("text", ""))
        tras = [e for e in vistas[i:] if e.get("text", "").strip()]
        perdidas, tarde = [], []
        for e in tras:
            cuerpo = e["text"].strip()
            m = next((u for u in udp if ts(u) >= t0 and txt(u) == cuerpo), None)
            if not m:
                perdidas.append(cuerpo)
            elif ts(m) - e["ts"] > UDP_RETARDO_S:
                tarde.append(cuerpo)
        c.note(f"- {len(tras)} líneas serie desde T08; sin copia {len(perdidas)}; tarde {len(tarde)}")
        assert len(tras) >= 5, f"control positivo: solo {len(tras)} líneas serie tras T08"
        assert not perdidas, f"{len(perdidas)} líneas sin copia UDP: {perdidas[:3]}"
        assert not tarde, f"{len(tarde)} líneas llegaron por UDP más de {UDP_RETARDO_S} s tarde: {tarde[:3]}"
        c.passed(f"{len(tras)} líneas con copia UDP en ≤ {UDP_RETARDO_S} s durante {CICLOS} ciclos")


def _captura_arranque(tb, dut_slot, dut_mac, banco, c, segundos=240):
    """Arranque y 3 ciclos con la captura en marcha; devuelve los flujos UDP del DUT."""
    banco.broker_on()
    banco.ap_on()
    tb.capture_start(dut_mac.lower(), segundos)
    reg = Registro(tb, dut_slot, time.time())
    tb.serial_reset(dut_slot)
    assert reg.esperar(re.escape(T11), 150), "sin T11"
    fin = time.time() + segundos + 30
    r = {}
    while time.time() < fin:
        r = tb.capture_result()
        if r.get("state") == "done":
            break
        time.sleep(5)
    assert r.get("state") == "done", f"la captura no terminó: {r.get('state')}"
    flujos = r.get("flows") or []
    c.note("", "## Flujos del DUT (captura de cabeceras)", "",
           *[f"- {f['proto']} → {f['dst']}:{f['dport']} ({f['kind']}): {f['packets']}" for f in flujos])
    return [f for f in flujos if f["proto"] == "UDP"], flujos


def test_tc_log_08_udp_log_unicast_only(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-LOG-08 · clima:FR-LOG-02 (captura) — el log UDP va por unicast solo a 192.168.4.1:5555."""
    tb = testbench
    if firmware["variant"] == "production":
        pytest.skip("not done: TC-LOG-08 necesita la imagen de banco")
    with case(tb, "TC-LOG-08", "Log UDP solo por unicast al receptor", run_dir, spec="TC-LOG heredado (villa-nodo-clima-piloto)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        c.step("Captura del tráfico del DUT durante el arranque y 3 ciclos")
        udp, _ = _captura_arranque(tb, dut_slot, dut_mac, banco, c)
        log = [f for f in udp if f["dport"] == LOG_PORT]
        malos = [f for f in log if f["dst"] != LOG_HOST or f["kind"] != "unicast"]
        assert log and sum(f["packets"] for f in log) > 10, f"control positivo: casi no hay log UDP: {log}"
        assert not malos, f"log UDP por broadcast o a otro destino: {malos}"
        c.passed(f"{sum(f['packets'] for f in log)} datagramas de log, todos unicast a {LOG_HOST}:{LOG_PORT}")


def test_tc_log_04_no_udp_log_in_production(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-LOG-04 · clima:FR-LOG-01 — imagen de producción: por UDP solo DHCP, DNS y SNTP."""
    tb = testbench
    if firmware["variant"] != "production":
        pytest.skip("not done: TC-LOG-04 necesita la imagen de producción (--firmware-dir)")
    with case(tb, "TC-LOG-04", "Sin log UDP en producción", run_dir, spec="TC-LOG heredado (villa-nodo-clima-piloto)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        c.step("Captura del tráfico del DUT durante el arranque y 3 ciclos (imagen de producción)")
        udp, flujos = _captura_arranque(tb, dut_slot, dut_mac, banco, c)
        otros = [f for f in udp if f["dport"] not in UDP_PERMITIDOS]
        assert any(f["proto"] == "TCP" and f["dport"] == 1883 for f in flujos), "control positivo: no se ve el MQTT del DUT"
        assert not otros, f"UDP fuera de DHCP/DNS/SNTP: {otros}"
        c.passed("por UDP solo " + ", ".join(sorted({str(f['dport']) for f in udp})) + "; ningún datagrama al 5555")
