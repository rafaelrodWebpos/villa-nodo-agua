"""Pruebas que usan los comandos serie de la imagen de banco (clima:E13 en la 0.9.10, E13-USJ;
FSD §20), heredadas del piloto (tests/bench/test_villa2.py). Atómicos: cada test graba la
imagen de la CI y aprovisiona contra un AP desechable nuevo con el broker del banco.

TC-WDT-02 · clima:NFR-WDT-01: `wdt-test` → «wdt: bloqueando ciclo» y, en ≤ 32 s,
  `boot: … reset=task_wdt` (FR-HER-01: el ciclo es la tarea vigilada mientras no hay protección).
TC-TIME-08 · clima:FR-TIME-02 (bench⁺): tras sincronizar, SNTP bloqueado y `reiniciar` por la
  consola → `boot: … reset=sw` y hora_valida = false en los status tras el reinicio."""
import json
import re
import time

from benchlib import case
from test_bench_plus import _sntp_block
from test_mqtt import _a_operativo, _nodo, _suscribir, configurado

T11 = "con: CONECTANDO_BROKER -> OPERATIVO (mqtt_up)"
SINC = re.compile(r"sntp: sincronizado ts=\d+")
BOOT = re.compile(r"nodo: boot: \S+ fw=\S+ reset=(\w+)")


def _escribir(tb, slot, texto):
    r = tb.serial_write(slot, text=texto + "\n")
    assert r.get("ok", True), f"serial_write falló: {r}"


def test_tc_wdt_02_task_watchdog(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-WDT-02 · clima:NFR-WDT-01 — wdt-test → reset=task_wdt en ≤ 30 s (+ 2 s)."""
    tb = testbench
    with case(tb, "TC-WDT-02", "Watchdog de tareas (wdt-test)", run_dir, spec="TC-WDT villa-nodo-clima-piloto") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        _, reg = _a_operativo(tb, dut_slot, banco, c)
        c.step("Enviar «wdt-test» por la consola (USB-Serial/JTAG)")
        t0 = time.time()
        _escribir(tb, dut_slot, "wdt-test")
        blq = reg.esperar(re.escape("wdt: bloqueando ciclo"), 20, desde_ts=t0)
        assert blq, "sin «wdt: bloqueando ciclo»"
        boot = reg.esperar(BOOT.pattern, 60, desde_ts=blq["ts"])
        txt = [e.get("text", "") for e in reg.leer() if e.get("ts", 0) >= t0]
        c.save_log("serie", txt)
        assert boot, "el nodo quedó colgado: ninguna línea boot: en 60 s"
        dt = boot["ts"] - blq["ts"]
        reset = BOOT.search(boot["text"]).group(1)
        c.note(f"- de «bloqueando ciclo» a la línea boot: {dt:.1f} s; reset={reset}")
        assert reset == "task_wdt", f"reset={reset}"
        assert dt <= 33, f"reinicio a {dt:.1f} s (30 s + 2 s, más el arranque hasta boot:)"
        c.passed(f"reset=task_wdt a {dt:.1f} s de «bloqueando ciclo»")


def test_tc_time_08_reboot_without_sntp(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-TIME-08 · clima:FR-TIME-02 — tras reiniciar sin SNTP, hora_valida = false."""
    tb = testbench
    base = f"sitio1/zona1/{_nodo(dut_mac)}"
    with case(tb, "TC-TIME-08", "Reinicio sin SNTP: hora no válida", run_dir, spec="TC-TIME villa-nodo-clima-piloto") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        banco.broker_on()
        try:
            with _suscribir(tb, _nodo(dut_mac)) as sub:
                _, reg = _a_operativo(tb, dut_slot, banco, c, internet=True)
                assert reg.esperar(SINC.pattern, 90), "precondición: sin «sntp: sincronizado»"
                c.step("Bloquear SNTP y enviar «reiniciar» por la consola (USB-Serial/JTAG)")
                _sntp_block(True)
                t0 = time.time()
                _escribir(tb, dut_slot, "reiniciar")
                boot = reg.esperar(BOOT.pattern, 60, desde_ts=t0)
                assert boot, "sin línea boot: tras «reiniciar»"
                assert reg.esperar(re.escape(T11), 150, desde_ts=t0), "sin T11 tras el reinicio"
                time.sleep(130)                                  # dos status de ciclo
                st = [json.loads(m["payload"]) for m in sub.mensajes()
                      if m["topic"] == f"{base}/status" and not m["retain"] and m["t"] >= t0 and m["payload"]]
                st = [s for s in st if s.get("estado") == "online"]
                txt = [e.get("text", "") for e in reg.leer() if e.get("ts", 0) >= t0]
        finally:
            _sntp_block(False)
        c.save_log("serie", txt)
        reset = BOOT.search(boot["text"]).group(1)
        hv = [s.get("hora_valida") for s in st]
        i_boot = next((i for i, t in enumerate(txt) if BOOT.search(t)), 0)   # solo tras el reinicio
        c.note(f"- boot tras «reiniciar»: reset={reset}", f"- hora_valida en los status tras el reinicio: {hv}",
               f"- «sntp: sincronizado» tras el reinicio: {sum(bool(SINC.search(t)) for t in txt[i_boot:])}")
        assert reset == "sw", f"reset={reset}, esperado sw"
        assert len(hv) >= 2 and not any(hv), f"hora_valida tras el reinicio: {hv}"
        assert not any(SINC.search(t) for t in txt[i_boot:]), "se sincronizó con SNTP bloqueado"
        c.passed(f"reset=sw; hora_valida = false en {len(hv)} status sin SNTP")
