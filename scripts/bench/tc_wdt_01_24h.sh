#!/usr/bin/env bash
# TC-WDT-01 (24 h) en el slot del DUT, con un solo comando. Comprueba las precondiciones,
# toma el candado compartido del banco y lanza la prueba; el candado se suelta solo al salir.
#   scripts/bench/tc_wdt_01_24h.sh            # comprueba y lanza
#   scripts/bench/tc_wdt_01_24h.sh --check    # solo comprueba, no toca el banco
# Lee TESTBENCH_URL y TESTBENCH_TOKEN de .env (nunca se imprimen). La salida de pytest va a
# $SALIDA; la evidencia, a test-results/<hora>-<commit>/ como siempre.
set -euo pipefail
cd "$(dirname "$0")/../.."
LOCK="$HOME/villa/banco.lock"
FW=".fw-run"
IMAGEN="nodo-clima-piloto-dev:latest"
SALIDA="${SALIDA:-${TMPDIR:-/tmp}/tc_wdt_01_24h.log}"
fallo() { echo "NO SE LANZA: $*" >&2; exit 1; }

set -a; . ./.env; set +a
[ -n "${TESTBENCH_URL:-}" ] && [ -n "${TESTBENCH_TOKEN:-}" ] || fallo ".env sin TESTBENCH_URL o TESTBENCH_TOKEN"

# 1 · el repo: limpio y subido (la evidencia nombra el commit)
[ -z "$(git status --porcelain --untracked-files=no)" ] || fallo "hay cambios sin commit"
[ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/main | cut -f1)" ] || fallo "HEAD no es origin/main"

# 2 · la imagen de banco ya bajada (CI), una sola
ls "$FW"/firmware_v*-bench.bin >/dev/null 2>&1 && [ -f "$FW/coldflash/flash_args" ] || fallo "falta la imagen de banco en $FW"
echo "imagen: $(basename "$FW"/firmware_v*-bench.bin)"

# 3 · nadie más en el banco: contenedores del piloto, candado, AP, broker, panel de pruebas
docker ps --format '{{.Image}} {{.Names}}' | grep -i piloto && fallo "corre un contenedor del piloto"
flock -n "$LOCK" true || fallo "el candado del banco está tomado"
api() { curl -sf -m 10 -H "X-Harness-Token: $TESTBENCH_TOKEN" "$TESTBENCH_URL/api/$1"; }
INFO=$(api info) || fallo "el banco no responde"
python3 - "$INFO" "$(api wifi/ap_status)" "$(api mqtt/status)" "$(api test/progress)" "$(api devices)" <<'EOF' || fallo "el banco no está libre"
import json, sys
info, ap, mq, prog, devs = (json.loads(x) for x in sys.argv[1:])
print(f"banco: build {info.get('build')}, slots corriendo {info.get('slots_running')}")
mal = []
if ap.get("active"): mal.append("el AP está encendido (no es nuestro)")
if mq.get("running"): mal.append("el broker está encendido (no es nuestro)")
if prog.get("active"): mal.append(f"hay una prueba en el panel: {prog.get('spec')}")
devs = devs if isinstance(devs, list) else devs.get("devices", devs.get("slots", []))
s2 = next((d for d in devs if d.get("label") == "SLOT2"), None)
if not s2 or not s2.get("present"): mal.append("SLOT2 no está presente")
elif s2.get("state") != "idle" or s2.get("debugging"): mal.append(f"SLOT2 ocupado ({s2.get('state')})")
for m in mal: print("  -", m)
sys.exit(1 if mal else 0)
EOF
echo "precondiciones: OK"
[ "${1:-}" = "--check" ] && exit 0

# 4 · lanzar (≈ 24 h + preparación); TZ=AST4 → horas locales en la evidencia
echo "lanzada $(date '+%F %T %z'); salida en $SALIDA"
nohup flock -n "$LOCK" timeout 93600 docker run --rm --network host -v "$PWD":/p -w /p \
    -u "$(id -u):$(id -g)" -e HOME=/tmp -e TZ=AST4 -e TESTBENCH_URL -e TESTBENCH_TOKEN "$IMAGEN" \
    /opt/bench-venv/bin/python -m pytest tests/bench/test_wdt.py --firmware-dir "$FW" \
    -p no:cacheprovider -q -rA --tb=short > "$SALIDA" 2>&1 &
echo "pid $!"
