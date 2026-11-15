"""TC-TIME-06 heredado del piloto (ddf8016, FR-HER-01) — sin hora válida (clima §13). El AP
del banco no tiene salida a Internet: SNTP es inalcanzable desde el arranque. TC-TIME-05 (la
primera medida con ts null) llega con los drivers de E/S. Atómico: graba la imagen de la CI,
aprovisiona contra un AP desechable nuevo y deja el AP y el broker apagados al terminar."""
import json
import re
import time

from benchlib import case
from test_mqtt import _a_operativo, _ms, _nodo, _suscribir, configurado, _ultimo_arranque

CON_IP_S = 300                  # FR-TIME-06: DUT con IP durante 5 min
SNTP_MAX_S = 15                 # FR-TIME-06
WIFI_UP = "con: CONECTANDO_WIFI -> CONECTANDO_BROKER (wifi_up)"

def test_tc_time_06_sntp_retry_without_time(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-TIME-06 · FR-TIME-06 — SNTP inalcanzable; DUT con IP durante 5 min: los intervalos
    entre líneas «sntp: inicio <srv>» consecutivas (marcas del DUT) son ≤ 15 s, también después
    de un «sntp: fallo»."""
    tb = testbench
    nodo = _nodo(dut_mac)
    with case(tb, "TC-TIME-06", "Reintento SNTP sin hora", run_dir, spec="TC-TIME heredado (villa-nodo-clima-piloto)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        c.note("- AP del banco sin salida a Internet: SNTP inalcanzable", "")
        banco.broker_on()
        with _suscribir(tb, nodo):
            t0, reg = _a_operativo(tb, dut_slot, banco, c)
            c.step(f"Observar la consola {CON_IP_S // 60} min con IP")
            time.sleep(CON_IP_S + 20)
        ent = _ultimo_arranque(reg)
        c.save_log("serie", [e["text"] for e in ent])
        textos = [e["text"].strip() for e in ent]
        up = next((t for t in textos if WIFI_UP in t), None)
        assert up, f"sin {WIFI_UP} en el último arranque"
        t_up = _ms(up)
        inicios = [(_ms(t), t) for t in textos if "sntp: inicio " in t and _ms(t) is not None and _ms(t) >= t_up]
        fallos = [t for t in textos if "sntp: fallo" in t]
        # Ventana con IP: de wifi_up a wifi_up + 5 min (marcas del DUT)
        ventana = [x for x in inicios if x[0] <= t_up + CON_IP_S * 1000]
        intervalos = [round((b[0] - a[0]) / 1000, 1) for a, b in zip(ventana, ventana[1:])]
        primero = (ventana[0][0] - t_up) / 1000 if ventana else None
        c.note("", "## Resultado", "",
               f"- primera «sntp: inicio» a {primero if primero is not None else '-'} s de wifi_up",
               f"- «sntp: inicio» en los 5 min: {len(ventana)}; intervalos (s): {intervalos}",
               f"- «sntp: fallo» en la captura: {len(fallos)}",
               f"- servidor: {ventana[0][1].split('sntp: inicio ')[-1] if ventana else '-'}")
        assert ventana, "ninguna «sntp: inicio» con IP"
        assert primero <= SNTP_MAX_S, f"la primera petición llegó a {primero} s de wifi_up"
        assert intervalos and max(intervalos) <= SNTP_MAX_S, f"intervalo > {SNTP_MAX_S} s: {intervalos}"
        assert len(ventana) >= CON_IP_S // SNTP_MAX_S, f"solo {len(ventana)} peticiones en {CON_IP_S} s"
        c.passed(f"{len(ventana)} peticiones en 5 min, intervalo máximo {max(intervalos)} s")
