"""TC-HA-A1 · FR-HA-A1 (bench) — con el Home Assistant de prueba (scripts/ha/ha_prueba.py,
heredado del piloto: Docker de la laptop, instancia nueva, broker del banco), el nodo aparece
como 1 dispositivo «Agua sitio1 zona1» con las 20 entidades de §14 (el % de batería queda
pendiente de definir en el FSD), sin duplicados tras 3 reinicios del DUT. Atómico: graba la
imagen de la CI y aprovisiona contra un AP desechable nuevo."""
import re
import time
from urllib.parse import urlparse

from benchlib import Registro, case, suscriptor_mqtt
from test_mqtt import _nodo, configurado

T11 = re.escape("con: CONECTANDO_BROKER -> OPERATIVO (mqtt_up)")
HA_ESPERA_S = 60
REINICIOS = 3
ENTIDADES = 20     # §14 menos el % de batería (pendiente del FSD)
DOMINIOS = {"valve": 1, "switch": 1, "binary_sensor": 10, "sensor": 5, "button": 2, "event": 1}


def _errores(devs):
    if len(devs) != 1:
        return [f"{len(devs)} dispositivos MQTT"]
    (dev,) = devs.values()
    err = [] if dev["nombre"] == "Agua sitio1 zona1" else [f"nombre {dev['nombre']!r}"]
    ents = dev["entidades"]
    dom = {}
    for e in ents:
        d = e["entity_id"].split(".")[0]
        dom[d] = dom.get(d, 0) + 1
    if len(ents) != ENTIDADES or dom != DOMINIOS:
        err.append(f"{len(ents)} entidades {dom}")
    # Los sensores de medida llevan expire_after (clima:FR-HA-03): HA los deja unavailable hasta
    # su primer valor, que llega con las E/S (FR-PUB-03). El resto depende solo de status online.
    no_disp = [e["entity_id"] for e in ents if e["state"] == "unavailable" and not e["entity_id"].startswith("sensor.")]
    if no_disp:
        err.append(f"no disponibles (status online no visto): {no_disp[:3]}")
    return err


def _esperar(ha, timeout):
    fin = time.time() + timeout
    while True:
        devs = ha.dispositivos_mqtt()
        err = _errores(devs)
        if not err or time.time() > fin:
            return devs, err
        time.sleep(3)


def test_tc_ha_a1_home_assistant_entities(testbench, dut_slot, dut_mac, firmware, run_dir, tmp_path):
    """TC-HA-A1 · FR-HA-A1 — 1 dispositivo con las 20 entidades; sin duplicados tras 3 reinicios."""
    from ha_cliente import HomeAssistant
    tb = testbench
    nodo = _nodo(dut_mac)
    with case(tb, "TC-HA-A1", "Home Assistant: entidades de §14", run_dir, spec="TC-HA (villa-nodo-agua)") as c, \
            configurado(tb, dut_slot, dut_mac, firmware, c, tmp_path) as banco:
        ha = HomeAssistant()
        c.note(f"- Home Assistant de prueba {ha.version} en la laptop (instancia nueva)", "")
        fases = []
        banco.broker_on()
        with suscriptor_mqtt(tb, f"homeassistant/+/{nodo}/#"):
            pass
        ha.anadir_mqtt(urlparse(tb.base_url).hostname)
        c.step("Reset y AP arriba; esperar T11 y el dispositivo en Home Assistant")
        t0 = time.time()
        reg = Registro(tb, dut_slot, t0)
        tb.serial_reset(dut_slot)
        banco.ap_on()
        assert reg.esperar(T11, 130), "sin T11 en 130 s"
        fases.append(("conexión", *_esperar(ha, HA_ESPERA_S)))
        for i in range(1, REINICIOS + 1):
            c.step(f"Reinicio {i} de {REINICIOS}; esperar T11")
            t_r = time.time()
            tb.serial_reset(dut_slot)
            assert reg.esperar(T11, 130, desde_ts=t_r + 5), f"sin T11 en 130 s tras el reinicio {i}"
            fases.append((f"reinicio {i}", *_esperar(ha, HA_ESPERA_S)))
        c.save_log("serie", [e["text"] for e in reg.leer()])
        c.note("", "## Resultado", "")
        for nombre, devs, err in fases:
            ents = [e for d in devs.values() for e in d["entidades"]]
            c.note(f"- {nombre}: {len(devs)} dispositivo(s), {len(ents)} entidades" + (f" — {err}" if err else ""))
        (dev,) = fases[-1][1].values()
        c.note("", "## Entidades (último reinicio)", "", *[f"- `{e['entity_id']}` = {e['state']}" for e in dev["entidades"]])
        for nombre, devs, err in fases:
            assert not err, f"{nombre}: {err} (FR-HA-A1)"
        c.passed(f"1 dispositivo «Agua sitio1 zona1» con {ENTIDADES} entidades tras conectar y tras {REINICIOS} reinicios")
