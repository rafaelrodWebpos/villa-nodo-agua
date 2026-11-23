# Puerta: Load defined (cierra la Fase 0)

**Exclusión permanente:** solo artefactos commiteados. Nada de memoria, transcripciones ni relato del autor.

FSD: `docs/Functionality/FSD-nodo-agua.md`. IDs: `FR-<ÁREA>-nn`, `NFR-<ÁREA>-nn`, `C-<ÁREA>-nn`, `T-nn`.

## Requisitos

| # | Debe ser cierto | Dónde mirar | Evidencia |
|---|---|---|---|
| 1 | Todo Must/Should tiene contrato con columna o campo «No debe ocurrir» / `prohibited_outcomes` | FSD, capítulo de cada requisito | La fila de la tabla de contratos o un bloque `id:` YAML con su ID |
| 2 | Toda fila V01–V20 (§5.3) y B01–B09 (§6.2) tiene desde, evento, guarda, hacia, acción y límite, y la regla de completitud de §5.3 está escrita. Las filas T01–T25 se heredan de clima §6.3 (FSD del piloto 0.9.9) | FSD §5.3, §6.2 | Tablas completas |
| 3 | No queda ningún `(propuesto` sin decidir ni ningún `(assumed)` | FSD | grep |
| 4 | No hay `{{parámetros}}` sin enlazar (salvo las plantillas `{{ value_json.* }}` de HA) | FSD | grep |
| 5 | Cada fase de §3 se puede empezar con lo que entregan las anteriores | FSD §3 frente a los contratos | Los criterios de salida citan solo IDs cuyos contratos no usan capacidades de fases posteriores |
| 6 | Existen los tres planos y `docs/00-Overview.md` | `docs/` | ficheros |
| 7 | El perfil de amenazas (§21.1) precede a los requisitos de seguridad, y P14 dice «solo en producción, nunca en el banco» (C-SEC-A3, FR-SEC-A4) | FSD §21 | orden y texto |

## Comprobaciones mecánicas

```bash
F=docs/Functionality/FSD-nodo-agua.md
test -f docs/00-Overview.md && ls docs/Method/AI-Workflow.md docs/UserDocumentation/Manual-de-usuario.md
! grep -n '(propuesto\b' "$F" | grep -v 'propuesto, Dnn\|propuesto por el paquete'
! grep -n '(assumed)' "$F"
! grep -o '{{[^}]*}}' "$F" | grep -v 'value_json'
# Todo ID definido aparece en una fila de contrato o en un bloque id: YAML
python3 - "$F" <<'PY'
import re,sys
s=open(sys.argv[1],encoding='utf-8').read()
ids=re.findall(r'^- \*\*((?:FR|NFR|C)-[A-Z]+-[A-Z]?\d+)\*\* \[',s,re.M)+re.findall(r'^\| (C-[A-Z]+-[A-Z]?\d+) \[',s,re.M)
cov=set(re.findall(r'^id: (\S+)',s,re.M))
for row in re.findall(r'^\| ([A-Z]+-[A-Z]+-[A-Z]?\d+(?:[/, ]+(?:[A-Z]+-[A-Z]+-)?[A-Z]?\d+)*) \|',s,re.M):   # IDs con letra (FR-CON-A1), propietario 2026-11-23
    for x in re.split(r',\s*',row):
        if '/' in x: b,_,n=x.rpartition('-'); cov|={b+'-'+p for p in n.split('/')}
        else: cov.add(x)
miss=[i for i in ids if i not in cov]; print(len(ids),"IDs; sin contrato:",miss); sys.exit(1 if miss else 0)
PY
```

## Comprobaciones de juicio (cita lo que leíste)

- Elige 5 contratos al azar, incluidos uno de §5.5 (válvula) y uno de §6.4 (bomba). ¿Podría alguien competente montar un rig a partir de cada uno sin leer el código? Cita el estímulo y la observación.
- ¿Algún contrato que §3.1 asigna a la Fase 1 depende de MQTT, de SNTP, de la OTA o de Home Assistant? Si es así, es un defecto de secuencia.
- ¿Hay palabras vagas (adecuado, robusto, razonable, suficiente, funcionamiento normal…) en las líneas de requisito?

## Trampas de este proyecto

- Contratos de la Fase 1 que observan por MQTT o por órdenes (ya pasó en 0.1.0: §3.1 tuvo que separarlos).
- Contratos con IDs abreviados («C-HW-20…29», «FR-EJE-01, -02») que la comprobación mecánica no ve (pasó en 0.3.0).
- Un valor `Pnn` usado en un requisito sin fila en §4.4.
- Un ejemplo con datos reales de la vivienda (SSID, IP, zona) en un repositorio público.

## Veredicto
`OPEN` · `SHUT` + hallazgos.
