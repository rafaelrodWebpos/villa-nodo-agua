"""Pruebas de banco de §13 y §16 que solo necesitan el AP y el broker del banco (sin estímulos
de E/S). Atómicas: graban la imagen de la CI, aprovisionan (P-NVS-ERASE: la configuración de
campo parte de fábrica) y dejan el AP, el broker y los retenidos que publican como estaban."""
import json
import time

from benchlib import case
from test_mqtt import _a_operativo, _guardar, _nodo, _sin_reinicio, _suscribir, _ultimo_arranque, configurado


def _eventos(msgs, base, tipo, desde=0):
    out = []
    for m in msgs:
        if m["topic"] == f"{base}/evento" and not m["retain"] and m["t"] >= desde:
            try:
                p = json.loads(m["payload"])
            except ValueError:
                continue
            if p.get("tipo") == tipo:
                out.append((m["t"], p))
    return out


def _ultimo(msgs, topic, desde=0):
    ms = [m for m in msgs if m["topic"] == topic and m["t"] >= desde and m["payload"]]
    return json.loads(ms[-1]["payload"]) if ms else None


def test_tc_ord_a3_payloads(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-ORD-A3 · FR-ORD-02 (bench) — 300 bytes, `{`, {"accion":"x"} y {"accion":5} en valvula/set y
    bomba/set → orden_rechazada (payload, accion, payload) por cada una, sin movimiento ni reinicio."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    cargas = [("x" * 300, "payload"), ("{", "payload"), ('{"accion":"x"}', "accion"), ('{"accion":5}', "payload")]
    with case(tb, "TC-ORD-A3", "Órdenes mal formadas", run_dir) as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        with _suscribir(tb, nodo) as sub:
            t0, reg = _a_operativo(tb, dut_slot, banco, c)
            time.sleep(5)
            t1 = time.time()
            for top in ("valvula/set", "bomba/set"):
                for carga, _ in cargas:
                    c.step(f"Publicar en {top}: {carga[:20]}{'…' if len(carga) > 20 else ''}")
                    sub.publicar(f"{base}/{top}", carga)
                    time.sleep(1.5)
            time.sleep(3)
            msgs = sub.mensajes()
        ent = _ultimo_arranque(reg)
        _guardar(c, ent, msgs, t0)
        rech = _eventos(msgs, base, "orden_rechazada", t1)
        motivos = [(p.get("orden"), p.get("motivo")) for _, p in rech]
        esperados = [(top, m) for top in ("valvula/set", "bomba/set") for _, m in cargas]
        movs = [m for m in msgs if m["t"] >= t1 and m["topic"] in (f"{base}/valvula/state", f"{base}/bomba/state")]
        c.note("", "## Resultado", "", f"- orden_rechazada: {motivos}", f"- estados publicados tras las órdenes: {len(movs)}")
        _sin_reinicio(ent)
        assert motivos == esperados, f"rechazos {motivos} ≠ {esperados}"
        assert not movs, f"una orden mal formada cambió un estado: {[m['payload'] for m in movs]}"
        c.passed("8 órdenes mal formadas → 8 orden_rechazada con su motivo; sin cambios ni reinicio")


def test_tc_ord_a4_retain(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-ORD-A4 · FR-ORD-03 (bench) — abrir, permitir_consumo y casa_cerrada=false publicados con retain
    y reconexión del nodo → sin efecto: ni consumo_permitido ni casa_cerrada, config/state sin cambio."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    retenidos = [("valvula/set", '{"accion":"abrir"}'), ("medidor/set", '{"accion":"permitir_consumo","min":30}'),
                 ("config/set", '{"param":"casa_cerrada","v":false}')]
    with case(tb, "TC-ORD-A4", "Órdenes con retain", run_dir) as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        with _suscribir(tb, nodo) as sub:
            try:
                c.step("Publicar las tres órdenes con retain antes de que el nodo se conecte")
                for top, carga in retenidos:
                    sub.publicar(f"{base}/{top}", carga, retain=True)
                t0, reg = _a_operativo(tb, dut_slot, banco, c)
                time.sleep(15)
                msgs = sub.mensajes()
            finally:
                for top, _ in retenidos:                     # limpieza: borrar los retenidos
                    sub.publicar(f"{base}/{top}", "", retain=True)
        ent = _ultimo_arranque(reg)
        _guardar(c, ent, msgs, t0)
        textos = [e["text"] for e in ent]
        ignoradas = [t for t in textos if "ignorada (retain)" in t]
        efectos = _eventos(msgs, base, "consumo_permitido", t0) + _eventos(msgs, base, "casa_cerrada", t0)
        cfg = _ultimo(msgs, f"{base}/config/state", t0)
        c.note("", "## Resultado", "", f"- líneas «ignorada (retain)»: {len(ignoradas)}",
               f"- eventos con efecto: {len(efectos)}", f"- config/state: casa_cerrada={cfg.get('casa_cerrada') if cfg else None}")
        _sin_reinicio(ent)
        assert len(ignoradas) == 3, f"órdenes retenidas ignoradas: {len(ignoradas)} (esperadas 3)"
        assert not efectos, f"una orden retenida tuvo efecto: {efectos}"
        assert cfg and cfg.get("casa_cerrada") is True, f"config/state cambió: {cfg}"
        c.passed("las tres órdenes retenidas se ignoraron; casa_cerrada sigue true")


def test_tc_cfg_a2_config_set(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-CFG-A2 · FR-CFG-A1, FR-CFG-A2, FR-CFG-A3 (config/set), FR-CFG-A4, FR-CFG-A5 (bench) —
    t_seco_s=15 → config/state con 15, que persiste tras un reinicio; t_seco_s=0, 9999, xyz=1 y los
    del portal → rechazo sin cambio; casa_cerrada true→false, false→false, false→true → evento solo
    en el 1.º y el 3.º (v y antes correctos) y config/state en los tres."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    top = f"{base}/config/set"
    with case(tb, "TC-CFG-A2", "config/set", run_dir) as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        with _suscribir(tb, nodo) as sub:
            t0, reg = _a_operativo(tb, dut_slot, banco, c)
            time.sleep(5)
            c.step("t_seco_s=15")
            t1 = time.time()
            sub.publicar(top, '{"param":"t_seco_s","v":15}'); time.sleep(2)
            cfg15 = _ultimo(sub.mensajes(), f"{base}/config/state", t1)
            c.step("Rechazos: t_seco_s=0, 9999; xyz=1; sonda_activa[1], mqtt_host, wifi_ssid, i_marcha_a, i_seco_a, rango, altura")
            t2 = time.time()
            malos = ['{"param":"t_seco_s","v":0}', '{"param":"t_seco_s","v":9999}', '{"param":"xyz","v":1}',
                     '{"param":"sonda_activa[1]","v":false}', '{"param":"mqtt_host","v":1}', '{"param":"wifi_ssid","v":1}',
                     '{"param":"i_marcha_a","v":1}', '{"param":"i_seco_a","v":2}', '{"param":"rango_transductor_cm","v":300}',
                     '{"param":"altura_util_cm","v":100}']
            for m in malos:
                sub.publicar(top, m); time.sleep(1)
            time.sleep(2)
            c.step("casa_cerrada: true → false, false → false, false → true")
            t3 = time.time()
            for v in ("false", "false", "true"):
                sub.publicar(top, f'{{"param":"casa_cerrada","v":{v}}}'); time.sleep(2)
            c.step("Reiniciar el nodo y esperar T11: config/state debe traer t_seco_s=15")
            t4 = time.time()
            tb.serial_reset(dut_slot)
            reg.esperar(r"con: CONECTANDO_BROKER -> OPERATIVO \(mqtt_up\)", 90, desde_ts=t4)
            time.sleep(5)
            msgs = sub.mensajes()
        ent = reg.leer()
        _guardar(c, ent, msgs, t0)
        rech = _eventos(msgs, base, "orden_rechazada", t2)
        estados_malos = [m for m in msgs if m["topic"] == f"{base}/config/state" and t2 <= m["t"] < t3]
        cc = _eventos(msgs, base, "casa_cerrada", t3)
        estados_cc = [m for m in msgs if m["topic"] == f"{base}/config/state" and t3 <= m["t"] < t4]
        tras = _ultimo(msgs, f"{base}/config/state", t4)
        c.note("", "## Resultado", "", f"- config/state tras t_seco_s=15: t_seco_s={cfg15.get('t_seco_s') if cfg15 else None}",
               f"- rechazos: {len(rech)} de {len(malos)}; config/state publicados con los rechazos: {len(estados_malos)}",
               f"- eventos casa_cerrada: {[(p.get('v'), p.get('antes')) for _, p in cc]}; config/state: {len(estados_cc)}",
               f"- tras el reinicio: t_seco_s={tras.get('t_seco_s') if tras else None}, casa_cerrada={tras.get('casa_cerrada') if tras else None}")
        assert cfg15 and cfg15.get("t_seco_s") == 15, f"config/state tras t_seco_s=15: {cfg15}"
        assert len(rech) == len(malos) and not estados_malos, f"rechazos {len(rech)}/{len(malos)}; estados {len(estados_malos)}"
        assert [(p.get("v"), p.get("antes")) for _, p in cc] == [(False, True), (True, False)], f"eventos casa_cerrada: {cc}"
        assert len(estados_cc) == 3, f"config/state con casa_cerrada: {len(estados_cc)} (esperados 3)"
        assert tras and tras.get("t_seco_s") == 15 and tras.get("casa_cerrada") is True, f"tras el reinicio: {tras}"
        c.passed("t_seco_s=15 aceptado y persistente; 10 rechazos sin cambio; casa_cerrada con 2 eventos y 3 config/state")


RETENIDOS = ["valvula/state", "bomba/state", *(f"fuga/{n}" for n in range(1, 9)), "flotador", "red", "nivel",
             "presion", "corriente_bomba", "medidor", "config/state", "status"]


def test_tc_pub_a2_retenidos_y_medidas(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-PUB-A2 · FR-PUB-02, FR-PUB-03 (bench) — al reiniciar el broker todos los tópicos con retain de
    §13.1 reaparecen tras reconectar (bateria solo si el nodo ya la conoce); en 5 min de OPERATIVO,
    5 ± 1 publicaciones de nivel, presion, corriente_bomba y medidor."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    with case(tb, "TC-PUB-A2", "Retenidos y medidas", run_dir) as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        with _suscribir(tb, nodo) as sub:
            t0, reg = _a_operativo(tb, dut_slot, banco, c)
            c.step("5 min en OPERATIVO")
            t1 = time.time()
            time.sleep(305)
            c.step("Reiniciar el broker y esperar la reconexión")
            banco.broker_off(); time.sleep(5)
            t2 = time.time()
            banco.broker_on()
            reg.esperar(r"con: CONECTANDO_BROKER -> OPERATIVO \(mqtt_up\)", 150, desde_ts=t2)
            time.sleep(10)
            msgs = sub.mensajes()
        ent = _ultimo_arranque(reg)
        _guardar(c, ent, msgs, t0)
        cuenta = {m: sum(1 for x in msgs if x["topic"] == f"{base}/{m}" and t1 <= x["t"] < t1 + 300 and not x["retain"])
                  for m in ("nivel", "presion", "corriente_bomba", "medidor")}
        vuelta = {x["topic"][len(base) + 1:] for x in msgs if x["t"] >= t2 and x["topic"].startswith(base + "/") and x["payload"]}
        faltan = [t for t in RETENIDOS if t not in vuelta]
        c.note("", "## Resultado", "", f"- publicaciones en 5 min: {cuenta}", f"- tópicos tras reiniciar el broker: {sorted(vuelta)}",
               f"- faltan: {faltan}")
        _sin_reinicio(ent)
        assert not faltan, f"no reaparecen tras reconectar: {faltan}"
        for m, n in cuenta.items():
            assert 4 <= n <= 6, f"{m}: {n} publicaciones en 5 min (esperadas 5 ± 1)"
        c.passed(f"{len(RETENIDOS)} tópicos retenidos reaparecen; medidas {cuenta}")
