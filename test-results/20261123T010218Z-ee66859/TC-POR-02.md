# TC-POR-02 — GET / sirve el formulario

- Precondiciones: reset; el DUT debe arrancar en PORTAL
- Precondiciones: el DUT tiene configuración; P-NVS-ERASE y reset
- DUT en PORTAL: `agua-7f9af8`, fw `0.0.0-ee66859-bench`
- El banco se une al AP abierto agua-7f9af8 (intento 1)
- sta_join intento 1: ip del banco `192.168.4.2` (01:36:22.709 → 01:36:33.208 UTC)
- la API del banco sigue respondiendo tras sta_join (ruta de eth0 intacta)
- GET http://192.168.4.1/ por el relé HTTP del banco
- respuesta: HTTP 200, 3507 bytes, `TC-POR-02-get.html`
- campos (name): ['mqtt_host', 'mqtt_pass', 'mqtt_port', 'mqtt_user', 'sitio', 'wifi_pass', 'wifi_ssid', 'zona']

**Resultado: PASS** — GET / en agua-7f9af8: los 8 campos de §17, secretos vacíos
