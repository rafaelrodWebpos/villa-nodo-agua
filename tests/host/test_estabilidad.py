"""Nivel host de tests/bench/estabilidad.py (TC-WDT-01): un reinicio del banco (reset=usb,
rst:0x15 USB_UART_CHIP_RESET; upstream #83, #86) no cuenta como fallo del firmware y uno del
watchdog sí, aunque la consola haya perdido la salida anterior (upstream #80). Corre con
`python3 tests/host/test_estabilidad.py`."""
import json
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "bench"))
import estabilidad as e  # noqa: E402

BASE = "sitio1/zona1/agua-7f9af8"


def msg(t, uptime, reset="usb", estado="online", retain=False, topic=BASE + "/status"):
    return {"t": t, "topic": topic, "retain": retain,
            "payload": json.dumps({"estado": estado, "uptime_s": uptime, "reset": reset})}


def serie(n, t0=1000.0, u0=100, reset="usb"):
    return [msg(t0 + 60 * i, u0 + 60 * i, reset) for i in range(n)]


class TestEstabilidad(unittest.TestCase):
    def test_sin_reinicios(self):
        st = e.status_vivos(serie(10), BASE)
        self.assertEqual(len(st), 10)
        self.assertEqual(e.reinicios_status(st), [])
        self.assertEqual(e.huecos(st, []), [])
        self.assertEqual(e.resets_ajenos(st, "usb"), [])

    def test_filtra_retain_offline_y_otro_topico(self):
        ms = serie(3) + [msg(5000, 1, retain=True), msg(5001, 2, estado="offline"),
                         msg(5002, 3, topic=BASE + "/nivel"), {"t": 5003, "topic": BASE + "/status",
                                                             "retain": False, "payload": "no json"}]
        self.assertEqual(len(e.status_vivos(ms, BASE)), 3)

    def test_reinicio_del_banco_no_es_del_nodo(self):
        ms = serie(5) + [msg(1000 + 60 * 5 + 20, 3, "usb"), msg(1000 + 60 * 6 + 20, 63, "usb")]
        st = e.status_vivos(ms, BASE)
        r = e.reinicios_status(st)
        self.assertEqual(len(r), 1)
        self.assertEqual(r[0]["origen"], "banco")
        self.assertEqual(e.huecos(st, []), [])

    def test_reinicio_jtag_tambien_es_del_banco(self):
        st = e.status_vivos(serie(3) + [msg(1200, 2, "jtag")], BASE)
        self.assertEqual(e.reinicios_status(st)[0]["origen"], "banco")

    def test_watchdog_es_del_nodo_aunque_la_consola_no_lo_vea(self):
        # #80: sin líneas en la consola, el status siguiente dice task_wdt
        ms = serie(5, reset="usb") + [msg(1000 + 60 * 5 + 35, 4, "task_wdt")]
        r = e.reinicios_status(e.status_vivos(ms, BASE))
        self.assertEqual([(x["reset"], x["origen"]) for x in r], [("task_wdt", "nodo")])

    def test_cualquier_otra_causa_es_del_nodo(self):
        for causa in ("panic", "int_wdt", "wdt", "brownout", "sw", "ext", "poweron", "otro", None):
            st = e.status_vivos(serie(2) + [msg(1150, 1, causa)], BASE)
            self.assertEqual(e.reinicios_status(st)[0]["origen"], "nodo", causa)

    def test_uptime_igual_cuenta_como_reinicio(self):
        st = e.status_vivos([msg(1000, 100), msg(1060, 100)], BASE)
        self.assertEqual(len(e.reinicios_status(st)), 1)

    def test_reset_que_cambia_sin_reinicio(self):
        st = e.status_vivos([msg(1000, 100, "usb"), msg(1060, 160, "task_wdt")], BASE)
        self.assertEqual(e.resets_ajenos(st, "usb"), ["task_wdt"])

    def test_huecos_fuera_y_dentro_de_un_corte(self):
        ms = serie(3) + [msg(1000 + 60 * 2 + 400, 100 + 60 * 2 + 400)]   # hueco de 400 s
        st = e.status_vivos(ms, BASE)
        self.assertEqual(e.huecos(st, []), [(1120.0, 400.0)])
        corte = {"t_corte": 1150, "t_vuelta": 1450}
        self.assertEqual(e.huecos(st, [corte]), [])

    def test_rst_de_la_consola(self):
        lineas = ["ESP-ROM:esp32s3-20210327", "rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)",
                  "rst:0x7 (TG0WDT_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)",
                  "rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)"]
        r = e.rst_consola(lineas)
        self.assertEqual([(x[1], x[3]) for x in r], [("0x15", "banco"), ("0x7", "nodo"), ("0xc", "nodo")])


class Reloj:
    def __init__(self):
        self.t, self.esperas = 1000.0, []

    def __call__(self):
        return self.t

    def dormir(self, s):
        self.esperas.append(s)
        self.t += s


def llamada(reloj, fallos, error=ConnectionResetError("reset by peer"), dura=1.0):
    """fn que falla las primeras `fallos` veces (cada intento dura `dura` s) y luego responde."""
    n = {"i": 0}

    def fn():
        reloj.t += dura
        n["i"] += 1
        if n["i"] <= fallos:
            raise error
        return {"active": True}
    return fn


class TestVigia(unittest.TestCase):
    def setUp(self):
        self.r = Reloj()
        self.v = e.Vigia((OSError,), reloj=self.r, dormir=self.r.dormir)

    def test_sin_errores_no_registra(self):
        self.assertEqual(self.v.llamar("ap", llamada(self.r, 0)), (True, {"active": True}))
        self.assertEqual(self.v.eventos, [])
        self.assertFalse(self.v.caido())

    def test_transitorio_recuperado_queda_en_la_evidencia(self):
        ok, _ = self.v.llamar("wifi/ap_status", llamada(self.r, 2))
        self.assertTrue(ok)
        self.assertEqual(self.r.esperas, [5, 10])
        self.assertEqual([x.get("intento") for x in self.v.eventos], [1, 2, None])
        self.assertTrue(self.v.eventos[-1]["recuperado"])
        self.assertIn("ConnectionResetError", self.v.eventos[0]["error"])
        self.assertFalse(self.v.caido())

    def test_tres_reintentos_5_10_20_y_luego_falla(self):
        ok, val = self.v.llamar("ap", llamada(self.r, 99))
        self.assertEqual((ok, val), (False, None))
        self.assertEqual(self.r.esperas, [5, 10, 20])
        self.assertEqual(len(self.v.eventos), 4)
        self.assertFalse(self.v.caido())          # ~39 s sin responder: aún no

    def test_caido_solo_tras_mas_de_2_min_seguidos(self):
        fn = llamada(self.r, 99)
        self.v.llamar("ap", fn)
        self.r.t += 30
        self.v.llamar("ap", fn)                   # ~108 s desde el primer fallo
        self.assertFalse(self.v.caido())
        self.r.t += 30
        self.v.llamar("ap", fn)                   # > 120 s
        self.assertTrue(self.v.caido())

    def test_una_respuesta_reinicia_la_cuenta(self):
        self.v.llamar("ap", llamada(self.r, 99))
        self.r.t += 60
        self.assertTrue(self.v.llamar("ap", llamada(self.r, 0))[0])
        self.assertIsNone(self.v.sin_respuesta_desde)
        self.r.t += 200
        self.assertFalse(self.v.caido())

    def test_un_error_no_transitorio_no_se_traga(self):
        with self.assertRaises(KeyError):
            self.v.llamar("ap", llamada(self.r, 1, error=KeyError("x")))


if __name__ == "__main__":
    unittest.main(verbosity=1)
