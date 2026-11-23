# TC-POR-12 — Portal cerrado a los 10 min (ESPERA)

- imagen: `0.0.0-ee66859-bench` (variante bench, run CI 45085085658)

- Precondiciones: grabar la imagen de la CI en SLOT2 y P-NVS-ERASE
- Reset; el DUT abre el portal (T02)
- a ~30 s de T02, `agua-7f9af8` visible en el escaneo: sí
- Esperar el cierre del portal (T24, ≈ 10 min desde T02)

**Resultado: FAIL** — CommandTimeout: GET /api/serial/output?slot=SLOT2&lines=500&since=1795398352.048436: [Errno 104] Connection reset by peer
