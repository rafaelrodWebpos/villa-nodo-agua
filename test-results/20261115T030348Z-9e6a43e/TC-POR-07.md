# TC-POR-07 — Redirecciones del portal cautivo

- Precondiciones: reset; el DUT debe arrancar en PORTAL
- DUT en PORTAL: `agua-7f9af8`, fw `0.0.0-46989e0-bench`
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (03:08:08.163 → 03:08:20.115 UTC)
- GET http://192.168.4.1/generate_204
- GET /generate_204: 03:08:20.123 → 03:08:20.229 UTC
- GET http://192.168.4.1/hotspot-detect.html
- GET /hotspot-detect.html: 03:08:20.239 → 03:08:20.334 UTC
- log: `TC-POR-07-serie.log` (141 líneas)
- arranques del DUT (hora del banco, UTC):
  - 03:08:03.812 `rst:0x15 (USB_UART_CHIP_RESET),boot:0x9 (SPI_FAST_FLASH_BOOT)`
  - 03:08:03.812 `I (480) nodo: boot: agua-7f9af8 fw=0.0.0-46989e0-bench reset=usb part=ota_0 img=valida`
- /generate_204: respuesta final HTTP 200, 3507 bytes, formulario=True
- /hotspot-detect.html: respuesta final HTTP 200, 3507 bytes, formulario=True

**Resultado: PASS** — /generate_204 y /hotspot-detect.html terminan en el formulario del portal
