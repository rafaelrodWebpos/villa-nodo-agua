# Puerta: DUT ready (cierra la Fase 2)

**Exclusión permanente:** solo artefactos commiteados y respuestas en vivo del banco. Nada de memoria, transcripciones ni relato del autor.

## Requisitos

| # | Debe ser cierto | Dónde mirar | Evidencia |
|---|---|---|---|
| 1 | Las capacidades `bench-*` están copiadas de lo que el banco reporta ahora mismo | `testing/test-plan.yaml`; `curl $TESTBENCH_URL<endpoint de reported_by>` (`$TESTBENCH_URL` en `.env`) | El endpoint de `reported_by` de cada `bench-*` en `yes` responde `ok` en vivo. Ninguna capacidad lleva el motivo «banco pendiente». Ninguna lleva ya `exercised: no`: `/commission` las ejerce o las vuelve a declarar |
| 2 | `bench-ap-internet` está resuelta (`yes` o `no` con su consecuencia), y el nivel de cada test OTA es coherente con ella (bench+ o field, FSD §22.0) | plan | Campo `tier` de los TC-OTA y JRN de OTA |
| 3 | Registro del DUT: la placa del **SLOT2** es una Waveshare ESP32-S3-DEV-KIT-N8R8 (ESP32-S3, 8 MB de flash; clima:C-HW-01/02), sus eFuses de Secure Boot y de cifrado están en 0 (`espefuse summary`, C-SEC-A3) y el marcador de arranque aparece en la consola del slot. La sesión respetó `bench_coordination` | plan `dut.identity`, evidencia de /commission | Lectura de chip-info y espefuse con commit |
| 4 | Camino de ida probado: ejecución de CI → artefacto → flasheo con los offsets de `flash_args` → marcador `Init complete` observado en serie | plan, resultado del test de camino de ida | status `successful` con commit y run id |
| 5 | La fila 1 de la agenda (placa del SLOT2) está cerrada. Las filas 2–5 dependen de la compra y pueden seguir abiertas; la 6 se cierra con la ola de MQTT | `testing/debugging-agenda.md` | Fila 1 con estado cerrado y `dut-board` en `yes` |
| 6 | No queda ninguna capacidad del proyecto en `unproven` que necesite un test de la ola 1 **sin xfail**. Los pasos en xfail por la compra (JRN-01.4…01.8) se exceptúan | plan | Comprobación mecánica |
| 7 | El runner release-verify está registrado (si el propietario lo autorizó) | GitHub → Settings → Runners; `gh api repos/rafaelrodWebpos/villa-nodo-agua/actions/runners` | Runner con las etiquetas `self-hosted, testbench`; si no está, la capacidad sigue en `no` con su motivo |

## Comprobaciones mecánicas

```bash
curl -sf "$TESTBENCH_URL/api/info"
curl -s -H "X-Harness-Token: $TESTBENCH_TOKEN" "$TESTBENCH_URL/api/devices" | grep -q '"SLOT2"'
python3 -c "import yaml; p=yaml.safe_load(open('testing/test-plan.yaml')); \
  print([k for k,c in p['capabilities'].items() if c.get('reason','').startswith('banco pendiente')])"   # debe ser []
grep '^| 1 |' testing/debugging-agenda.md | grep -c 'pendiente'   # debe ser 0 (fila 1 cerrada)
python3 -c "import yaml; p=yaml.safe_load(open('testing/test-plan.yaml')); c=p['capabilities']; \
  ex={'home-assistant-test','github-test-releases'}; \
  print([(t['id'],n) for t in p['tests'] if t.get('wave')==1 and not t.get('xfail') for n in t.get('needs',[]) if c.get(n,{}).get('available')=='unproven' and n not in ex])"   # debe ser []
python3 -c "import yaml; p=yaml.safe_load(open('testing/test-plan.yaml')); c=p['capabilities']; \
  print([t['id'] for t in p['tests'] if t.get('status')=='successful' and any(c.get(n,{}).get('available')!=True for n in t.get('needs',[]) if n in ('home-assistant-test','github-test-releases'))])"   # debe ser [] (condición de la fila 6)
```

## Trampas de este proyecto

- Un USB nativo abierto con DTR/RTS activos detiene el S3 y parece un cuelgue (testing.md).
- Leer el tamaño de flash del log del bootloader: repite la configuración, no mide el chip.
- Dar por buena la salida a Internet del AP porque «el banco suele tenerla».

## Veredicto
`OPEN` · `SHUT` + hallazgos.
