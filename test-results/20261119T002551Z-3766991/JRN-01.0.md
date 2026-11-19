# JRN-01.0 — Grabar la imagen de la CI

- commit bajo prueba: `376699112114bafa8ba67e3842bad6c8bcbf5c99`
- imagen: `0.0.0-3e3ffe7-bench` (variante bench, run CI None)
- slot: SLOT2

- Precondiciones: slot presente

## Flasheo

- `0x0` bootloader.bin
- `0x8000` partition-table.bin
- `0x9000` nvs_blank.bin
- `0xf000` ota_data_initial.bin
- `0x20000` nodo_agua.bin

- Grabando 5 imágenes en SLOT2 (NVS en blanco)
- respuesta: ok=True returncode=0
- Wrote 21120 bytes (13589 compressed) at 0x00000000 in 0.2 seconds (808.3 kbit/s).
- Wrote 3072 bytes (128 compressed) at 0x00008000 in 0.0 seconds (1075.1 kbit/s).
- Wrote 8192 bytes (31 compressed) at 0x0000f000 in 0.0 seconds (2011.4 kbit/s).
- Wrote 1035232 bytes (659692 compressed) at 0x00020000 in 4.5 seconds (1825.8 kbit/s).
- Wrote 24576 bytes (47 compressed) at 0x00009000 in 0.1 seconds (1809.0 kbit/s).

- Reset y captura del arranque: NODO-BENCH-IMAGE, Init complete

## Arranque

- log: `JRN-01.0-boot.log` (118 líneas)
- tras el último `rst:` (116 líneas):
- `NODO-BENCH-IMAGE`: sí
- `Init complete`: sí
- versión en el log: `I (496) nodo: nodo_agua 0.0.0-3e3ffe7-bench`

**Resultado: PASS** — 0.0.0-3e3ffe7-bench arranca: NODO-BENCH-IMAGE + Init complete
