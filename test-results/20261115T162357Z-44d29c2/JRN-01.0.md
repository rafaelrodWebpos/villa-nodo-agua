# JRN-01.0 — Grabar la imagen de la CI

- commit bajo prueba: `44d29c2c4264aca0e0ba4309cfd844ebffa91271`
- imagen: `0.0.0-26c3661-bench` (variante bench, run CI None)
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

- Reset y captura del arranque: NODO-BENCH-IMAGE, Init complete

## Arranque

- log: `JRN-01.0-boot.log` (96 líneas)
- tras el último `rst:` (94 líneas):
- `NODO-BENCH-IMAGE`: sí
- `Init complete`: sí
- versión en el log: `I (480) nodo: nodo_agua 0.0.0-26c3661-bench`

**Resultado: PASS** — 0.0.0-26c3661-bench arranca: NODO-BENCH-IMAGE + Init complete
