"""Pruebas de banco del medidor testigo (FSD §6.7) que no necesitan el generador de pulsos
(CR-TB-27). Atómicas: graban la imagen de la CI, aprovisionan contra un AP desechable y el broker
del banco, y dejan el AP y el broker apagados al terminar."""
import json
import re
import time

from benchlib import case
from test_mqtt import _a_operativo, _guardar, _nodo, _sin_reinicio, _suscribir, _ultimo_arranque, configurado

MARGEN_MIN = 120          # consumo_margen_min de fábrica (§19)


def test_tc_med_a10_consumo_permitido(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-MED-A10 · FR-MED-10 — suscriptor a #: permitir_consumo 30 min; a los 5 min, 60 min; a los
    6 min, 10 min → un evento consumo_permitido por cada una de las dos primeras en ≤ 1 s, con
    hasta_min = 30 + margen y 60 + margen (± 1); ninguno con la tercera (no mueve el fin)."""
    tb = testbench
    nodo = _nodo(dut_mac)
    base = f"sitio1/zona1/{nodo}"
    with case(tb, "TC-MED-A10", "Evento consumo_permitido", run_dir) as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        with _suscribir(tb, nodo) as sub:
            t0, reg = _a_operativo(tb, dut_slot, banco, c)
            time.sleep(5)                                    # suscripción del nodo a medidor/set (T11)
            ordenes = []
            for espera_s, minutos in ((0, 30), (300, 60), (60, 10)):
                if espera_s:
                    time.sleep(espera_s)
                c.step(f"Publicar medidor/set permitir_consumo min={minutos}")
                t = time.time()
                sub.publicar(f"{base}/medidor/set", json.dumps({"accion": "permitir_consumo", "min": minutos}))
                ordenes.append((t, minutos))
            time.sleep(5)
            msgs = sub.mensajes()
        ent = _ultimo_arranque(reg)
        _guardar(c, ent, msgs, t0)
        eventos = []
        for m in msgs:
            if m["topic"] == f"{base}/evento" and not m["retain"]:
                try:
                    p = json.loads(m["payload"])
                except ValueError:
                    continue
                if p.get("tipo") == "consumo_permitido":
                    eventos.append((m["t"], p))
        por_orden = []
        for k, (t, minutos) in enumerate(ordenes):
            fin = ordenes[k + 1][0] if k + 1 < len(ordenes) else t + 5
            por_orden.append([(te, p) for te, p in eventos if t <= te < fin])
        c.note("", "## Resultado", "",
               *[f"- orden {k + 1} (min={mn}): {len(ev)} evento(s)"
                 + (f", a {ev[0][0] - t:+.2f} s, hasta_min={ev[0][1].get('hasta_min')}" if ev else "")
                 for k, ((t, mn), ev) in enumerate(zip(ordenes, por_orden))])
        a9 = [e["text"] for e in ent if re.search(r"eve: consumo_permitido hasta_min=\d+", e["text"])]
        c.note(f"- líneas A9 de consumo_permitido: {len(a9)}")
        _sin_reinicio(ent)
        for k, esperado in ((0, 30 + MARGEN_MIN), (1, 60 + MARGEN_MIN)):
            ev = por_orden[k]
            assert len(ev) == 1, f"orden {k + 1}: {len(ev)} eventos consumo_permitido (esperado 1)"
            te, p = ev[0]
            assert te - ordenes[k][0] <= 1.0, f"orden {k + 1}: evento a {te - ordenes[k][0]:.2f} s (límite 1 s)"
            assert abs(int(p.get("hasta_min", -999)) - esperado) <= 1, f"orden {k + 1}: hasta_min {p.get('hasta_min')} (esperado {esperado} ± 1)"
        assert not por_orden[2], f"la orden que no mueve el fin dio evento: {por_orden[2]}"
        assert len(a9) == 2, f"líneas A9 de consumo_permitido: {len(a9)} (esperadas 2)"
        c.passed(f"hasta_min {por_orden[0][0][1]['hasta_min']} y {por_orden[1][0][1]['hasta_min']} en ≤ 1 s; "
                 "ninguno con la orden de 10 min")
