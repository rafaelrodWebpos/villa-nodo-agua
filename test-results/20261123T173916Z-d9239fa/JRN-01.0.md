# JRN-01.0 — Grabar la imagen de la CI

- commit bajo prueba: `d9239faf43e0d6e615396062ab6b7ea453699f4e`
- imagen: `0.0.0-d9239fa-bench` (variante bench, run CI 45202669354)
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
- Wrote 21264 bytes (13675 compressed) at 0x00000000 in 0.2 seconds (790.3 kbit/s).
- Wrote 3072 bytes (128 compressed) at 0x00008000 in 0.0 seconds (1107.2 kbit/s).
- Wrote 8192 bytes (31 compressed) at 0x0000f000 in 0.0 seconds (2013.2 kbit/s).
- Wrote 1040896 bytes (663610 compressed) at 0x00020000 in 4.8 seconds (1751.0 kbit/s).
- Wrote 24576 bytes (47 compressed) at 0x00009000 in 0.1 seconds (2382.7 kbit/s).

- Reset y captura del arranque: NODO-BENCH-IMAGE, Init complete

## Arranque

- log: `JRN-01.0-boot.log` (119 líneas)
- tras el último `rst:` (117 líneas):
- `NODO-BENCH-IMAGE`: sí
- `Init complete`: sí
- versión en el log: `I (495) nodo: nodo_agua 0.0.0-d9239fa-bench`

**Resultado: PASS** — 0.0.0-d9239fa-bench arranca: NODO-BENCH-IMAGE + Init complete
