"""TC-HW-* — restricciones de hardware (FSD §2.2), nivel target observado por el banco.
Atómicos: cada uno graba lo que necesita y deja el DUT con la imagen de la CI del
commit bajo prueba."""
import os
import pathlib
import re
import subprocess
import time

import pytest

from benchlib import _after_last_reset, _boot_capture, _lines, _parse_flash_args, case, esperar_lineas

REPO = pathlib.Path(__file__).resolve().parents[2]
USB_C_HW_01 = {"303a:1001", "1a86:55d3"}     # USB-Serial/JTAG nativo y CH343 (un solo USB-C con hub)


def _grabar(tb, slot, firmware):
    res = tb.flash(slot, _parse_flash_args(firmware["dir"] / "coldflash"), chip="esp32s3")
    assert res.get("ok") and res.get("returncode", 0) == 0, f"flash falló: {res.get('error') or res}"


def test_tc_hw_02_chip_reading(testbench, dut_slot, run_dir):
    """TC-HW-02 · C-HW-01, C-HW-02 (target) — esptool sobre la placa: ESP32-S3 con PSRAM de 8 MB,
    flash de 8 MB y las dos interfaces USB del único conector."""
    tb = testbench
    with case(tb, "TC-HW-02", "Lectura del chip y de la flash", run_dir, spec="TC-HW heredado (villa-nodo-clima-piloto)") as c:
        c.step(f"Precondiciones: {dut_slot} presente")
        slot = tb.get_slot(dut_slot)
        if not slot.get("present"):
            pytest.skip(f"not done: {dut_slot} no presente")
        usb = {d.get("vid_pid") for d in slot.get("usb_devices") or []}
        c.step("POST /api/chip/info (esptool en el Pi; la placa se reinicia sola)")
        r = tb._api_post("/api/chip/info", {"slot": dut_slot}, timeout=60)
        c.save_log("esptool", (r.get("output") or "").splitlines())
        c.note("", "## Resultado", "", f"- chip: `{r.get('chip')}` {r.get('revision')}", f"- features: `{r.get('features')}`",
               f"- flash: `{r.get('flash_size')}`", f"- USB del slot: {sorted(usb)}")
        assert r.get("ok") and r.get("returncode", 0) == 0, f"chip/info falló: {r.get('error') or r.get('returncode')}"
        assert str(r.get("chip", "")).startswith("ESP32-S3"), f"chip {r.get('chip')} (C-HW-01)"
        assert "Embedded PSRAM 8MB" in str(r.get("features", "")), "el módulo no es N8R8: sin PSRAM de 8 MB (C-HW-01)"
        assert r.get("flash_size") == "8MB", f"flash de {r.get('flash_size')}, se esperan 8 MB (C-HW-01/02)"
        assert USB_C_HW_01 <= usb, f"interfaces USB {sorted(usb)}: faltan {sorted(USB_C_HW_01 - usb)} (C-HW-01)"
        c.passed("ESP32-S3 con PSRAM de 8 MB, flash de 8 MB y las dos interfaces del único USB-C")


def test_tc_hw_03_boots_without_psram(testbench, dut_slot, firmware, run_dir):
    """TC-HW-03 · C-HW-03 (target) — la imagen de la CI, construida con la PSRAM deshabilitada
    (lo comprueba build_checks.py en la CI), llega a fin_arranque en ≤ 120 s sin abortar ni reiniciarse."""
    tb = testbench
    with case(tb, "TC-HW-03", "Arranque sin PSRAM", run_dir, spec="TC-HW heredado (villa-nodo-clima-piloto)") as c:
        c.note(f"- imagen: `{firmware['version']}` (variante {firmware['variant']}, run CI {firmware['ci_run']})", "")
        c.step(f"Precondiciones: grabar la imagen de la CI en {dut_slot}")
        _grabar(tb, dut_slot, firmware)
        c.step("Reset y 120 s de observación")
        t0 = time.time()
        reset = _lines(tb.serial_reset(dut_slot))
        time.sleep(max(0.0, t0 + 120 - time.time()))
        buf = [e.get("text", "") for e in esperar_lineas(tb, dut_slot, t0, [], timeout=0)]
        lineas = _after_last_reset(reset) + ["--- buffer del slot ---"] + buf
        c.save_log("serie", lineas)
        boot = _after_last_reset(reset) or buf
        fin = next((l for l in boot + buf if re.search(r"con: ARRANQUE -> (CONECTANDO_WIFI|PORTAL) \(fin_arranque\)", l)), None)
        e1 = sorted({l.strip() for l in boot + buf if "boot: " in l and " fw=" in l})
        malos = [l for l in boot + buf if "Guru Meditation" in l or "abort()" in l]
        c.note("", "## Resultado", "", f"- fin_arranque: `{(fin or 'no').strip()}`", f"- líneas E1: {len(e1)}",
               f"- Guru Meditation / abort(): {len(malos)}")
        assert any(firmware["version"] in l for l in e1), f"el arranque no es la imagen {firmware['version']}"
        assert fin, "sin con: ARRANQUE -> … (fin_arranque) en 120 s"
        assert not malos, f"el log muestra {malos[0][:80]}"
        assert len(e1) == 1, f"{len(e1)} arranques en la ventana: el nodo se reinició"
        c.passed("fin_arranque sin PSRAM, sin abortar y con un solo arranque en 120 s")
