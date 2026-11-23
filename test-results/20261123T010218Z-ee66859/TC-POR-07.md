# TC-POR-07 — Redirecciones del portal cautivo

- Precondiciones: reset; el DUT debe arrancar en PORTAL
- DUT en PORTAL: `agua-7f9af8`, fw `0.0.0-ee66859-bench`
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (01:37:28.180 → 01:37:39.187 UTC)
- GET http://192.168.4.1/generate_204
- GET /generate_204: 01:37:39.196 → 01:37:39.312 UTC
- GET http://192.168.4.1/hotspot-detect.html
- GET /hotspot-detect.html: 01:37:39.324 → 01:37:39.428 UTC
- log: `TC-POR-07-serie.log` (128 líneas)
- arranques del DUT (hora del banco, UTC):
  - 01:37:23.712 `rst:0x15 (USB_UART_CHIP_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)`
  - 01:37:23.712 `I (495) nodo: boot: agua-7f9af8 fw=0.0.0-ee66859-bench reset=usb part=ota_0 img=valida`
- /generate_204: respuesta final HTTP 200, 3507 bytes, formulario=True
- /hotspot-detect.html: respuesta final HTTP 200, 3507 bytes, formulario=True

**Resultado: PASS** — /generate_204 y /hotspot-detect.html terminan en el formulario del portal
