# Nodo Agua — Especificación Funcional (FSD)

```yaml
document_status: vigente — Load defined (2026-11-14)
fsd_version: 0.8.3
repository: villa-nodo-agua (público)
baseline_commit: c3e5053 (Load defined, 2026-11-14); cada versión, el commit que la introduce
applicable_firmware_version: ninguna todavía
author: Rafael Rodriguez
reviewers: []
approval_status: D1–D18 y P1–P21 aceptadas por el propietario (2026-11-14; P14 con condición);
  H-93 (2026-11-20) y H-95 (2026-11-21, P22–P25) aceptados; H-96 (2026-11-21) aprobado,
  y P28–P34 aceptados (2026-11-21); P26/P27 aceptados para la próxima ronda (H-93)
created: 2026-11-14
last_updated: 2026-11-23
herencia: villa-nodo-clima-piloto, docs/Functionality/FSD-nodo-clima.md, versión 0.9.9 (commit ddf8016)
change_history:
  - 0.1.0 (2026-11-14): versión inicial a partir de /define (D1–D18 del propietario).
  - 0.2.0 (2026-11-14): P1–P20 aceptadas en bloque; P14 solo en la placa final de producción
    (S2/S4), nunca en placas del banco (C-SEC-A3, FR-SEC-A4); capacidades del banco
    CR-TB-16…22 (§3.1); P21 propuesta (lazo 4–20 mA a 24 V, C-HW-31).
  - 0.3.0 (2026-11-14): P21 aceptada (elevador MT3608); Fase 0 cerrada (Load defined).
  - 0.3.1 (2026-11-14): editorial — tabla de particiones copiada en §2.2 para la CI; contratos
    con los IDs completos (C-HW-20…29, FR-EJE-02, FR-CIC-01) y fila de C-BLD-01.
  - 0.4.0 (2026-11-15): decisiones del propietario. §14: se quita la entidad de % de batería
    (con LiFePO4 la tensión no da un % fiable); se anuncian la tensión y el estado
    `ok`/`baja`/`critica` de P5, con sus avisos (FR-ENE-02/03). §20: clima:E13 se hereda en
    la redacción de la 0.9.10 del piloto (E13-USJ), solo en la imagen de banco.
  - 0.4.1 (2026-11-15): aclaraciones del propietario. P4: cada banda incluye su borde
    superior (≤ 0,05 corto … > 0,90 abierta). FR-CIC-02: «una vez por ventana» = como mucho
    un aviso cada `ciclado_ventana_min`, contados desde el aviso.
  - 0.5.0 (2026-11-15): decisiones del propietario sobre las tablas. §5: los dos finales
    activos = FALLA (V03b, V15), ninguno = DESCONOCIDA; V20 pone `cerrada_por`=`fuga`;
    filas nuevas V21 (fuga durante CERRANDO), V22 y V23 (cierre y reapertura por batería,
    P16); una fuga confirmada siempre gana. §6: B05 sin límite de intentos para los cortes
    por nivel o flotador; B09 con el transductor en falla; filas nuevas B10 (`sin_agua`) y
    B11 (`nivel_bajo`); motivo `nivel_bajo` en FR-ORD-01.
  - 0.6.0 (2026-11-15): decisiones del propietario. §2.2: A3 del 0x4A mide la referencia
    de los lazos (fracción de P4 = V(lazo)/V(A3), con 3,3 V nominales si no es válida).
    §19: hora local = UTC−4 sin horario de verano. §6: FR-BOM-10 (flotador en falla tras
    10 min sin agua con nivel suficiente; evento `flotador_falla`), con excepciones en
    FR-BOM-04/08. Confirmados: GPIO5 sin señal = red ausente; los cortes por seco de antes
    de un reinicio cuentan como ocurridos al arrancar (FR-PER-02).
  - 0.7.0 (2026-11-20): H-93 del propietario, a partir del análisis de cortes parciales
    (`testing/analisis-cortes-parciales.md`). Cola de eventos críticos (FR-PUB-04, P23);
    evento `falla_vref` y LED (FR-SON-04); cierre preventivo con la casa cerrada y ≥ 2
    sondas en falla o falla de la referencia (FR-SON-05, V24, V25, parámetro `casa_cerrada`,
    P22): **cambia D6**. §25 nuevo: vigilante externo de 15 min por un canal que no depende
    de la Pi (FR-VIG-01), UPS de la cadena de red (C-INS-01) y broker por IPv4 con reservas
    DHCP (C-INS-02). FR-CON-A1/A2 (heredados del análisis del piloto, H1 y H2: IP perdida y
    reasociación sin broker), propuestos con P24/P25. Valores aceptados para la próxima
    ronda: k = 3 reinicios en 10 min (P26) y RSSI mínimo −75 dBm (P27).
  - 0.7.1 (2026-11-21): H-95 del propietario: P22, P23, P24 y P25 aceptados tal como
    estaban; FR-SON-05, FR-PUB-04/05 y FR-CON-A1/A2 pasan a normativos. Riesgo de
    `casa_cerrada`=false por (b′) aceptado con P22 (§21.1). Contrato de FR-IO-06 alineado con
    FR-SON-05: sin cierre solo con `casa_cerrada`=false.
  - 0.7.2 (2026-11-21): decisión del propietario. El riesgo de P22 se acepta con una
    condición: cada cambio de `casa_cerrada` genera el evento `casa_cerrada` (FR-CFG-A5) y un
    aviso de HA al propietario (FR-VIG-02), para que un cambio que no hizo él se note.
    Contratos de FR-CON-A1/A2 con las capacidades del banco ya numeradas por el propietario:
    `bench-dhcp-off` (CR-TB-24) y `bench-broker-blackhole` (CR-TB-23), registradas en el
    análisis de cortes parciales del piloto (H-94).
  - 0.8.0 (2026-11-21): H-96 del propietario: medidor testigo DAE AS200U-75P (pulsos por
    contacto seco) entre el medidor de la empresa de agua y la cisterna. **Propuesta**: §6.7 (FR-MED-01…08),
    entrada en GPIO18 (C-HW-32), tópicos `medidor` y `medidor/set`, eventos
    `consumo_casa_cerrada` y `medidor_falla`, parámetros nuevos en §19, línea A8 y P28–P33
    en §4.4. Nada de esto es normativo hasta que el propietario acepte P28–P33.
  - 0.8.1 (2026-11-21): decisión del propietario: P28–P33 aceptados, y P34 nuevo. Con la
    casa cerrada, además del umbral por hora hay un **umbral diario** (`consumo_diario_l`,
    50 L en 24 h fuera de riego y relleno): mismo evento `consumo_casa_cerrada`, con
    `motivo`=`diario` (FR-MED-09). Las fugas lentas solo se ven así, porque la boya rellena
    despacio y el medidor no cuenta bien por debajo de 0,25 GPM. §6.7 pasa a normativo.
    `bench-pulsos-medidor` es CR-TB-27.
  - 0.8.2 (2026-11-23): decisión del propietario tras TC-CON-06 en el banco (2026-11-22).
    P24 y FR-CON-A1 aclarados: `IP_LOST` solo cuenta en los estados que tienen IP
    (CONECTANDO_BROKER y OPERATIVO). ESP-IDF lo avisa 120 s después de cada desconexión
    aunque el nodo nunca haya tenido IP; en los demás estados se ignora.
  - 0.8.3 (2026-11-23): editorial, del cierre de la puerta Load defined (verificador
    independiente). Sin cambios de comportamiento: A8 en la Fase 1; asignación de fases de
    §6.6, §6.7, §10 y FR-CON-A1/A2; CR-TB-23/24 en §3; `consumo_casa_cerrada` en la lista de
    FR-PUB-04 (ya lo decía FR-MED-05); contratos de FR-MED-09 (tasa), FR-SON-04
    (`casa_cerrada`), FR-MED-03 (comando `reiniciar` de E13), FR-MED-06/09 y FR-EJE-04
    (casos que faltaban), C-HW-33 en campo; se quita «propuesta» de textos ya aceptados.
```

## Convenciones de este documento

Las convenciones son las mismas del FSD del piloto (`FSD-clima`, §«Convenciones»):
identificadores estables `FR-`, `NFR-`, `C-` y filas de transición; prioridades
`[Must]`/`[Should]`/`[May]`; procedencia `[user]`, `[derived]`, `[pack:esp32]`;
niveles de prueba `host`, `target`, `bench`, `bench⁺`, `field`, `other` y `manual`.
Además:

- **`clima:<ID>`** cita un requisito del FSD del piloto, versión 0.9.9. Ese requisito
  aplica a este nodo **con su contrato de verificación**, con las adaptaciones de
  §10.1. Un requisito heredado no se reescribe aquí. Si el piloto cambia, este FSD
  sigue atado a la 0.9.9 hasta que una enmienda diga lo contrario.
- **`Dnn`** son decisiones aceptadas por el propietario. **`Pnn`** son valores
  **propuestos**: el requisito que contiene un `Pnn` queda en estado `propuesto`
  hasta que el propietario acepte el valor (§4.4). Ningún `Pnn` pasa a normativo
  de forma implícita.
- Los nombres de zona y sitio de los ejemplos (`sitio1`, `zona1`) son ficticios. El
  repositorio es público y describe el **dispositivo**, no la vivienda (NFR-SEC-01).

---

## 1. Visión general del sistema

### 1.1 Propósito

Nodo ESP32-S3 que protege una vivienda contra **fugas de agua** y protege la
**bomba de la cisterna** (automatizaciones A-11 y A-12 del catálogo del
propietario). La vivienda pasa varios días seguidos cerrada, las fugas son el
riesgo n.º 1 y los cortes de la red eléctrica son frecuentes. Por eso, la protección
**funciona en local**, sin red, sin broker y sin Home Assistant (D18).

### 1.2 Usuarios y partes interesadas

| Parte | Interés |
|---|---|
| Propietario | Que una fuga con la casa cerrada se corte sola y le llegue un aviso; que la bomba no se queme en seco |
| Persona en el sitio (conserje, plomero) | Ver el estado en el nodo, silenciar el zumbador y abrir el agua en persona cuando ya no hay fuga |
| Home Assistant (hub) | Recibir estados y eventos, mandar órdenes de cierre o apertura, alertar por WhatsApp |
| Auditor de seguridad independiente | Puertas S1–S4 (nodo crítico, D14) |

### 1.3 Objetivos

1. Cerrar la válvula general ante una fuga detectada por una sonda cableada, sin depender de la red.
2. Cortar el permiso de la bomba si trabaja en seco o si la cisterna está en nivel crítico, y rearmarlo de forma automática y limitada.
3. Mantener la protección durante los cortes de la red eléctrica (mini UPS propia, D2).
4. Informar a Home Assistant de cada estado, evento, falla de sensor y estado de la batería (regla de batería baja, idea #33).
5. Reutilizar sin rediseñar los módulos comunes ya auditados del piloto (§10).

### 1.4 No-objetivos

- El nodo **no** arranca ni para la bomba por presión: eso lo sigue haciendo el presostato (D3).
- El nodo **no** interpreta los modos de la casa (`Casa cerrada`, `Preparar llegada`). Esos modos son escenas de HA que mandan órdenes al nodo (D11). La única huella de esos modos en el nodo es el parámetro `casa_cerrada` (§19), que HA fija y que el nodo guarda en NVS para seguir actuando sin red (FR-SON-05, H-93).
- El nodo **no** recibe los sensores inalámbricos WH31LA. Le llegan como orden de cierre de HA (D4).
- El nodo **no** envía WhatsApp. Las alertas y los recordatorios son de HA.

### 1.5 Flujo de alto nivel

```text
 sondas cableadas (8) ─┐                       ┌─► puente H ─► válvula de bola 12 V (finales A/C)
 flotador mínimo ──────┤                       ├─► relé 12 V ─► contactor (contacto NC) ─► bomba
 nivel 4–20 mA ────────┤   ┌──────────────┐    ├─► zumbador · LED de estado
 presión salida ───────┼──►│  nodo-agua   │────┤
 pinza de corriente ───┤   │ protección   │    └─► MQTT ─► broker ─► Home Assistant ─► WhatsApp
 finales de carrera ───┤   │ local (L2)   │◄────── MQTT: órdenes de válvula/bomba/config/OTA
 red presente · Vbat ──┤   └──────────────┘
 botón local ──────────┘
```

Topología hidráulica (D7): calle → cisterna → bomba/hidroneumático → derivación a
piscina y riego → **válvula general** → casa. La ubicación física de la cisterna, la
bomba y el nodo es un asunto de instalación y no afecta al firmware.

---

## 2. Arquitectura del sistema

### 2.1 Arquitectura lógica

La protección (fugas, válvula, bomba, energía) es una **región independiente** de la
conectividad. Corre igual en cualquier estado del gestor de conectividad heredado
(clima §6), incluidos PORTAL, ESPERA y OTA_DESCARGA. La conectividad solo transporta
estados y órdenes.

### 2.2 Plataforma hardware y restricciones que lee el build

Heredadas sin cambios: **clima:C-HW-01** (Waveshare ESP32-S3-DEV-KIT-N8R8),
**clima:C-HW-02** (8 MB fijos), **clima:C-HW-03** (sin PSRAM), **clima:C-HW-07**
(pines prohibidos) y **clima:C-HW-08** (consola UART0 + USB-Serial/JTAG). La tabla de
particiones y **clima:C-BLD-01…04** también se heredan tal cual (dos slots OTA de 3 MB,
ESP-IDF 6.x, rollback activado).

**Tabla de particiones (normativa).** Es la de clima §2.2, copiada aquí porque la CI la
lee de este documento (`scripts/ci/build_checks.py`).

| Nombre | Tipo | Subtipo | Offset | Tamaño |
|---|---|---|---|---|
| nvs | data | nvs | 0x9000 | 0x6000 (24 KB) |
| otadata | data | ota | 0xF000 | 0x2000 (8 KB) |
| phy_init | data | phy | 0x11000 | 0x1000 (4 KB) |
| ota_0 | app | ota_0 | 0x20000 | 0x300000 (3 MB) |
| ota_1 | app | ota_1 | 0x320000 | 0x300000 (3 MB) |

- **C-BLD-01** [Must] heredado (clima:C-BLD-01): la tabla de particiones debe ser la anterior.

No se hereda clima:C-HW-04/05/06 (SHT41 y alimentación por USB). Las nuevas son:

| ID | Restricción | Procedencia |
|---|---|---|
| C-HW-20 [Must] | El nodo y la válvula deben alimentarse de una fuente de **12 V CC con batería LiFePO4 propia** (mini UPS de CC). La fuente debe conectarse a un circuito respaldado por el inversor de la vivienda. La placa toma 5 V de un regulador CC-CC desde los 12 V. | [user] D2 |
| C-HW-21 [Must] | La válvula debe ser **de bola motorizada de 12 V CC, de dos hilos con inversión de polaridad**, con finales de carrera de «abierta» y «cerrada» (contactos secos) y palanca manual. Debe mantener su posición sin alimentación. | [user] D1 |
| C-HW-22 [Must] | El motor de la válvula debe manejarse con un **puente H** (tipo DRV8871 o equivalente de ≥ 2 A) que no permita activar los dos sentidos a la vez. | [user] D13 |
| C-HW-23 [Must] | El permiso de la bomba debe actuar sobre una **bobina de relé de 12 V** que maneje un contactor dimensionado para la bomba. El circuito de la bomba debe pasar por el **contacto NC**: relé sin energía = bomba permitida. El tablero debe tener un selector manual «Auto / Puenteado». El ESP32 no conmuta 120/240 V. | [user] D3 |
| C-HW-24 [Must] | Las entradas analógicas de las sondas, del nivel, de la presión y de Vbat deben leerse con **3 ADS1115** en el bus I²C (SDA = GPIO8, SCL = GPIO9, 100 kHz) en las direcciones **0x48, 0x49 y 0x4A** (P1). | [user] D13 · direcciones P1 |
| C-HW-25 [Must] | La pinza de corriente debe ser **no invasiva, de salida en tensión** (tipo SCT-013-030, 30 A → 1 V) y leerse con el ADC1 interno (P10). | [user] D9 · modelo P10 |
| C-HW-26 [Must] | Cada sonda de fuga debe cablearse como **lazo supervisado** con resistencia de fin de línea de 10 kΩ ± 1 % y resistencia de polarización de 10 kΩ ± 1 % a la referencia del ADS1115 (P4). | [user] D6 · valores P4 |
| C-HW-27 [Must] | El transductor de nivel debe ser **sumergible, de presión hidrostática, de 4–20 mA**, leído sobre una resistencia de 150 Ω ± 0,1 % (P18). | [user] D8 · shunt P18 |
| C-HW-28 [Must] | El sensor de presión de salida debe ser **ratiométrico de 0,5–4,5 V y 0–10 bar**, alimentado a 5 V y leído con un divisor 2:3 (P9). | [derived] · P9 |
| C-HW-29 [Must] | El nodo debe ir en una **caja IP65**. | [user] D13 |
| C-HW-30 [Must] | La asignación de pines debe ser exactamente la de la tabla siguiente (P1). | P1 |
| C-HW-32 [Must] | La salida de pulsos del medidor testigo (C-HW-33) debe llegar a GPIO18 como contacto seco a GND, con pull-up externa de 10 kΩ a 3,3 V, resistencia serie de 1 kΩ y 100 nF a GND junto al conector del nodo (el cable mide hasta 10 m). La pull-up interna también se activa. | [user] H-96 · P28 |
| C-HW-33 [Must] | El medidor testigo debe ser el **DAE AS200U-75P** (¾", galones US, ±1,5 %, 0,25–10 GPM), instalado en horizontal, con la esfera hacia arriba, techado y seco, entre el medidor de la empresa de agua y la válvula de llenado de la cisterna. | [user] H-96 |
| C-HW-31 [Must] | El lazo 4–20 mA del transductor debe alimentarse a 24 V CC ± 5 % desde un elevador CC-CC alimentado por los 12 V (P21). | [derived] P21 |

**Asignación de pines (P1 y P28).** Ningún pin usado está en la lista
prohibida de clima:C-HW-07.

| GPIO | Función | Tipo |
|---|---|---|
| 8 / 9 | I²C SDA / SCL (3 × ADS1115) | bus |
| 4 | Pinza de corriente (ADC1_CH3) | analógica |
| 5 | Red presente (salida «AC OK» de la fuente u optoacoplador; nivel alto = red presente; con pull-down: sin señal = red ausente [user, 2026-11-15]) | entrada |
| 6 | Final de carrera «abierta» (contacto a GND, pull-up; cerrado = activo) | entrada |
| 7 | Final de carrera «cerrada» (contacto a GND, pull-up; cerrado = activo) | entrada |
| 10 | Flotador de nivel mínimo (contacto a GND con pull-up; cerrado = hay agua; cable cortado = sin agua [user, 2026-11-15]) | entrada |
| 11 | Botón local (a GND, pull-up) | entrada |
| 12 / 13 | Puente H IN1 (cerrar) / IN2 (abrir) | salida |
| 14 | Relé de permiso de la bomba (alto = relé energizado = **permiso cortado**) | salida |
| 15 | Zumbador | salida |
| 16 / 17 | LED de estado verde / rojo (externos) | salida |
| 18 | Pulsos del medidor testigo (contacto seco a GND; pull-up interna y externa, C-HW-32) — H-96 (P28) | entrada |

**Canales de los ADS1115 (P1).** 0x48: sondas 1–4; 0x49: sondas 5–8; 0x4A: A0 = nivel
4–20 mA, A1 = presión, A2 = Vbat (divisor 1:6, P5), A3 = referencia de polarización de
los lazos (la misma tensión que alimenta las resistencias de 10 kΩ de C-HW-26)
[user, 2026-11-15]. La fracción de P4 es V(lazo) / V(A3). Con A3 fuera de 3,0–3,6 V o
el 0x4A sin responder, se usa la referencia nominal de 3,3 V y se registra
`son: vref nominal`.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| C-BLD-01 | `gen_esp32part.py` sobre la tabla de la imagen (CI, `scripts/ci/build_checks.py`) | Coincide fila a fila con la tabla de §2.2 | Tabla por defecto del toolchain | other (CI) |
| C-HW-20, C-HW-21, C-HW-25, C-HW-26, C-HW-27, C-HW-28, C-HW-29, C-HW-32 | Revisión del montaje contra la lista de materiales | Cada componente coincide con su restricción | Una válvula sin finales; una bomba conectada al contacto NA; alimentación sin batería | other (revisión) |
| C-HW-33 | Medidor instalado | Modelo AS200U-75P, horizontal con la esfera arriba, techado y seco, entre el medidor de la empresa de agua y la válvula de llenado | Otro modelo; instalado vertical, enterrado o a la intemperie | field (instalación) |
| C-HW-22 | Firmware de prueba que pide abrir y cerrar a la vez | IN1 e IN2 nunca están en alto a la vez (osciloscopio o GPIO del banco) | Los dos en alto, aunque sea un instante | bench |
| C-HW-23 | Nodo sin alimentación | La bomba recibe tensión a través del contacto NC | Bomba sin tensión con el nodo apagado | field (puesta en marcha) |
| C-HW-24 | Arranque con los 3 ADS1115 conectados | Línea `i2c: scan … ack=0x48,0x49,0x4a` (§20) | Falta alguna dirección | target |
| C-HW-31 | Medir la tensión del lazo con el transductor conectado, a 4 y a 20 mA | 22,8–25,2 V a la salida del elevador; ≥ 12 V en bornes del transductor | < 12 V en bornes | bench + field |
| C-HW-30 | Revisión del código + grep en la CI | Solo los pines de la tabla | Un pin fuera de la tabla o de la lista prohibida | other (CI) |

### 2.3 Arquitectura software

- Tareas FreeRTOS: **protección** (prioridad más alta de la aplicación, periodo de
  muestreo de P2), **medición analógica** (nivel, presión, corriente, Vbat),
  **interfaz local** y las de conectividad y OTA heredadas.
- **Persistencia** (D15): configuración (§19) y **bloqueos** (§9) en NVS. Los bloqueos
  se escriben solo cuando cambian.
- La protección no espera nunca por la red: ninguna llamada de red se hace desde la
  tarea de protección.

### 2.4 Capas de componentes

```text
┌───────────────────────────────────────────────────────────────────────────┐
│ L2 Lógica de aplicación                                                   │
│  [Protección de fugas + válvula §5] [Bomba y cisterna §6] [Energía §7]    │
│  [Interfaz local §8] [Persistencia de bloqueos §9]                         │
│  [Gestor de conectividad — heredado §10] [Política OTA — heredada §11]    │
├───────────────────────────────────────────────────────────────────────────┤
│ L1 Interfaces                                                             │
│  [Drivers de E/S §12] [Publicador MQTT y órdenes §13] [Discovery HA §14]   │
│  [Portal cautivo — heredado §15] [Configuración por MQTT §16]              │
├───────────────────────────────────────────────────────────────────────────┤
│ L0 Fundación (heredada §17)                                               │
│  [Wi-Fi STA/AP + DNS] [esp-mqtt] [esp_https_ota] [NVS] [SNTP] [I²C] [ADC]  │
│  [FreeRTOS + TWDT]                                                        │
└───────────────────────────────────────────────────────────────────────────┘
```

La correspondencia con el código fuente es HOW y está en `docs/Method/project/arquitectura.md`.

---

## 3. Fases de implementación

Cada fase se entra solo con lo que entregaron las anteriores.

### 3.1 Fase 1 — Protección local sin red

**Entrega:** drivers de E/S (§12), protección de fugas y válvula (§5), bomba y
cisterna (§6; del medidor testigo, solo el conteo y la NVS de §6.7), energía (§7, sin publicar), interfaz local (§8), persistencia (§9),
identidad (§18), líneas de log A1–A7 (§20), y portal + Wi-Fi heredados (§15, §10) para
dejar el nodo aprovisionado.

**Salida:** en verde, en host, target y bench, **con el broker apagado**, los contratos
de §5–§9, §12, §18 y FR-OBS-A1 cuyo estímulo es local (sondas, finales, flotador,
botón, red, tensiones, reloj, pulsos del medidor) y que se observan por GPIO o por las
líneas A1–A8. Los contratos cuyo estímulo, cuya precondición o cuya observación es un
mensaje MQTT (órdenes, `…/state`, eventos, parámetros fijados por `config/set`, como
`casa_cerrada`=false o `i_seco_a` calibrado) se cierran en la Fase 2, igual que §5.6 (el ejercicio necesita la hora SNTP). Los contratos de §15 y clima:FR-CON-*
heredados, en verde.

**Necesita del banco** (capacidades nuevas, numeradas por el propietario; se
implementan en el fork del banco con la puerta S6). Se prueba con las mismas piezas
que se instalarán cuando es posible (válvula, finales, flotador, ADS1115, resistencias
de fin de línea, shunt, transductores y pinza reales):

| CR-TB | Capacidad | Qué hace el banco |
|---|---|---|
| CR-TB-16 | `bench-lazos-fuga` | Pone cada uno de los 8 lazos en `seca`, `mojada`, `abierta` o `corto` |
| CR-TB-17 | `bench-finales-valvula` | Con la válvula real: fuerza cada final como activo o lo corta (simula un final que no llega) |
| CR-TB-18 | `bench-entradas-digitales` | Flotador (agua / sin agua), red presente, botón local (pulsación de duración dada) |
| CR-TB-19 | `bench-fuente-4-20ma` | Inyecta una corriente programable de 0–25 mA en la entrada del nivel |
| CR-TB-20 | `bench-tensiones` | Tensión programable de 0–5 V en la entrada de presión y tensión equivalente de Vbat |
| CR-TB-21 | `bench-senal-pinza` | Senoide de 60 Hz de amplitud programable (0–1 V RMS) en la entrada de la pinza |
| CR-TB-22 | `bench-salidas-dut` | Lee IN1, IN2, el relé de la bomba, el zumbador y los dos LED con marca de tiempo |
| CR-TB-23 | `bench-broker-blackhole` | Descarta (DROP, sin RST) el tráfico del DUT al puerto 1883 (FR-CON-A2; H-94 del piloto) |
| CR-TB-24 | `bench-dhcp-off` | Apaga solo el DHCP del AP del banco con el AP arriba (FR-CON-A1; H-94 del piloto) |
| CR-TB-27 | `bench-pulsos-medidor` | Tren de cierres de contacto seco en GPIO18 con duración, periodo y rebote programables (H-96) |

### 3.2 Fase 2 — Telemetría, órdenes y configuración

**Entrega:** publicador MQTT y órdenes (§13), Discovery (§14), configuración por MQTT
(§16), publicación de energía y batería (§7), ciclado (§6.6), publicación y alertas
del medidor testigo (§6.7) y observabilidad (§20).

**Salida:** contratos de §13, §14, §16, §7 (publicación), §6.6, §6.7 (FR-MED-04…09), §10
(FR-HER-01, FR-CON-A1/A2; los de bench⁺ cuando existan CR-TB-23/24), §20 y FR-SEC-A1 en verde en
bench; contratos heredados clima:FR-MQTT-*, clima:FR-HA-* y clima:FR-TIME-* en verde.
Los casos marcados «Fase 3» (OTA en FR-FUG-02, FR-HER-02 y FR-PER-02) y FR-SEC-A2 se
cierran en la Fase 3, porque necesitan la OTA.

### 3.3 Puerta S1 (entre la Fase 2 y la Fase 3)

Auditoría independiente de todo el código de las Fases 1 y 2 (`auditoria-seguridad.md`
del propietario, S1). Con especial atención al punto 6 (ningún mensaje se salta las
protecciones; §21). Un hallazgo crítico o alto bloquea la Fase 3.

### 3.4 Fase 3 — Actualización OTA

**Entrega:** OTA heredada (§11) con sus adaptaciones, y la variante de build de
producción firmada (P14, §22). Esa variante **no se graba** en placas del banco (C-SEC-A3);
la activación de Secure Boot y del cifrado ocurre solo en la placa final, al instalar (§3.5).

**Salida:** contratos de §11 en verde; puerta **S3**.

### 3.5 Antes de instalar — S2 y S4

Auditoría completa del nodo y de su configuración de producción (S2), más la revisión
reforzada de nodo crítico (S4, D14). Después, la puesta en marcha en el sitio
(calibración de §6, requisitos de instalación de §25 y contratos `field`).

---

## 4. Riesgos, supuestos y dependencias

### 4.1 Riesgos

| Riesgo | Mitigación |
|---|---|
| Una fuga en un punto sin sonda cableada no se detecta en local | WH31LA vía HA (D4) + detección de ciclado (§6.6) |
| Un WH31LA detecta una fuga mientras HA o la red están caídos: no se cierra | Riesgo aceptado (D4); el ciclado sigue avisando cuando vuelva la red |
| La válvula se pega por el sarro | Ejercicio periódico (§5.6, D12) |
| Un cable de sonda cortado deja un punto sin protección | Lazo supervisado y aviso `abierta` (D6); con la casa cerrada, cierre preventivo con ≥ 2 sondas en falla (FR-SON-05) |
| Se cae la referencia de los lazos y todas las sondas quedan ciegas | Evento `falla_vref` y LED (FR-SON-04); con la casa cerrada, cierre preventivo (FR-SON-05) |
| Una fuga ocurre con el broker inalcanzable y su evento se pierde | Cola de eventos críticos que se envía al volver (FR-PUB-04) |
| La Pi, el switch o internet caen y ninguna alerta sale | Vigilante externo por un canal que no depende de la Pi (FR-VIG-01) y UPS de la cadena de red (C-INS-01) |
| Una falsa alarma deja la casa sin agua con nadie dentro | Reapertura remota con todas las sondas secas (D5) |
| Un apagón largo agota la batería con la válvula abierta | Cierre preventivo con batería crítica y sin red (§7) |
| Un atacante en la LAN manda abrir la válvula durante una fuga | Guardas en el firmware (§5) que ningún mensaje salta; nodo crítico S4 (D14) |
| El nodo falla y la bomba queda cortada | Contacto NC: nodo sin energía = bomba permitida (D3) |
| El cable del medidor testigo se corta y se lee «consumo cero» (un contacto seco no se supervisa) | `medidor_falla` cuando el nivel de la cisterna sube sin pulsos (FR-MED-08); comparación mensual con la factura de la empresa de agua |

### 4.2 Supuestos

- La piscina y el riego se derivan **antes** de la válvula general (D7). Es un requisito para la instalación hidráulica.
- La fuente de 12 V ofrece una salida «AC OK» o se añade un optoacoplador en la entrada de red (P1).
- Las protecciones propias de la bomba (térmico, presostato) siguen instaladas (regla de seguridad del propietario).

### 4.3 Explícitamente fuera de alcance

- Control de arranque y parada de la bomba por presión (D3, opción 3 rechazada).
- Una segunda válvula en la acometida de la calle (D7, opción 3 rechazada).
- Interpretar los modos de la casa en el firmware (D11, opción 3 rechazada).
- Recepción directa de los WH31LA.
- Medidor de energía por Modbus (D9, opción 3 rechazada).

### 4.4 Decisiones de valores

| ID | Decisión | Estado | Afecta a |
|---|---|---|---|
| D1 | Válvula de bola motorizada 12 V CC con finales y palanca | aceptada [user] | C-HW-21, §5 |
| D2 | Fuente 12 V con LiFePO4 propia, conectada a un circuito respaldado por el inversor | aceptada [user] | C-HW-20, §7 |
| D3 | Permiso de la bomba con contactor por contacto NC + selector «Auto / Puenteado» | aceptada [user] | C-HW-23, §6 |
| D4 | 8 sondas cableadas con cierre local; WH31LA solo vía HA (riesgo aceptado) | aceptada [user] | §5, §13 |
| D5 | Reapertura remota solo con todas las sondas cableadas secas; en persona con botón o palanca; recierre si vuelve la fuga | aceptada [user] | FR-VAL-07…09 |
| D6 | Lazos supervisados (seca, mojada, abierta, corto); una falla de lazo avisa, no cierra. **Cambiada por H-93 (2026-11-20):** con `casa_cerrada`=true, ≥ 2 sondas activas en falla o la falla de la referencia cierran la válvula en preventivo; con la casa ocupada, solo aviso | aceptada [user]; H-93 | FR-SON-*, FR-SON-05 |
| D7 | Válvula a la salida de la bomba, con piscina y riego antes; con la válvula cerrada se mantiene el permiso de la bomba | aceptada [user] | FR-BOM-09 |
| D8 | Nivel por transductor 4–20 mA + flotador de nivel mínimo independiente | aceptada [user] | §6 |
| D9 | Trabajo en seco = corriente baja **y** presión baja durante T; umbrales en NVS, calibrados al instalar | aceptada [user] | FR-BOM-03 |
| D10 | Rearme automático si la causa desaparece; límite de intentos y bloqueo manual | aceptada [user]; números en P6 | FR-BOM-05…07 |
| D11 | El nodo detecta el ciclado y lo publica; HA decide; el nodo no cierra por ciclado | aceptada [user] | §6.6 |
| D12 | Ejercicio de la válvula cada 30 días a una hora configurable | aceptada [user] | §5.6 |
| D13 | Placa del piloto + ADS1115 + puente H + relé 12 V en caja IP65 | aceptada [user] | §2.2 |
| D14 | Nodo crítico: puertas S1–S4 | aceptada [user] | §3, §21 |
| D15 | Los bloqueos se guardan en NVS y un reinicio nunca rearma | aceptada [user] | §9 |
| D16 | LED + zumbador; pulsación corta silencia, pulsación larga de 3 s rearma/abre; zumbador 5 min | aceptada [user] | §8 |
| D17 | Parámetros por MQTT con rangos fijos en el firmware; ninguno apaga una protección | aceptada [user] | §16 |
| D18 | La protección es local y no depende de la red | aceptada [user] | §2.1, §5, §6 |
| P1 | Pines y direcciones de §2.2 | aceptado [user] | C-HW-24, C-HW-30 |
| P2 | Muestreo de sondas cada 250 ms; mojada **confirmada** tras 4 muestras seguidas (1 s); orden de cierre ≤ 500 ms después de confirmar (≤ 1,5 s desde el agua) | aceptado [user] | FR-FUG-01/02 |
| P3 | Tiempo máximo de carrera = `t_carrera_max_s`, 30 s de fábrica (rango 5–120 s) | aceptado [user] | FR-VAL-04 |
| P4 | Bandas del lazo (fracción de la referencia): corto ≤ 0,05; mojada (0,05–0,40]; seca (0,40–0,60]; indeterminada (0,60–0,90]; abierta > 0,90. Cada banda incluye su borde superior, para que en la duda gane la protección [user, 2026-11-15] | aceptado [user] | FR-SON-01 |
| P5 | Batería LiFePO4 4S: «baja» < 12,4 V, «crítica» < 12,0 V, histéresis 0,2 V; divisor 1:6 | aceptado [user] | FR-ENE-* |
| P6 | Bomba: `i_marcha_a` 0,5 A; `i_seco_a` sin valor de fábrica (detección por corriente inactiva hasta calibrar); `p_min_bar` 0,5; `t_seco_s` 10 s; rearme con nivel ≥ 30 % y flotador con agua durante 10 min; 3 intentos en 24 h; nivel crítico < 10 % | aceptado [user] | §6 |
| P7 | Ciclado: arranque = corriente ≥ `i_marcha_a` sostenida 2 s tras ≥ 2 s por debajo; aviso con ≥ 6 arranques en 60 min | aceptado [user] | §6.6 |
| P8 | Medidas cada 60 s; estados y eventos ≤ 1 s tras el cambio (en OPERATIVO) | aceptado [user] | §13 |
| P9 | Sensor de presión 0,5–4,5 V, 0–10 bar, divisor 2:3 | aceptado [user] | C-HW-28 |
| P10 | Pinza SCT-013-030; RMS sobre 10 ciclos de 60 Hz con ≥ 2000 muestras/s | aceptado [user] | C-HW-25, FR-IO-04 |
| P11 | La OTA se rechaza (`ocupado`) con la válvula en movimiento o con una sonda mojada | aceptado [user] | FR-OTA-A1 |
| P12 | Movimiento manual (palanca): el nodo no vuelve a mover el motor por su cuenta; publica `movimiento_manual` | aceptado [user] | FR-VAL-10 |
| P13 | Una sonda en falla (abierta, corto, indeterminada) cuenta como «no mojada» para la reapertura | aceptado [user] | FR-VAL-07 |
| P14 | Firma de imagen + Secure Boot v2 + cifrado de flash **solo en la placa final de producción, al instalar** (puertas S2/S4); **nunca** en las placas de desarrollo del banco, porque los eFuses son irreversibles | aceptado con condición [user] | §21, Fase 3 |
| P15 | Órdenes y configuración: payload ≤ 256 bytes; una orden de válvula durante un movimiento se rechaza con `ocupado` | aceptado [user] | §13 |
| P16 | Cierre preventivo por batería crítica **y** red ausente; reapertura automática con red presente 5 min, Vbat ≥ 12,8 V y sin bloqueo | aceptado [user] | FR-ENE-04/05 |
| P17 | Patrones del LED y del zumbador de §8.2 | aceptado [user] | §8 |
| P18 | 4–20 mA sobre 150 Ω; falla < 3,8 mA o > 20,5 mA | aceptado [user] | FR-NIV-02 |
| P19 | Con el transductor en falla, el rearme automático queda inhibido (solo manual) | aceptado [user] | FR-BOM-06 |
| P21 | El lazo 4–20 mA del transductor se alimenta a **24 V CC** con un elevador CC-CC desde los 12 V: un transmisor de 12–36 V no funciona con 12 V menos los 3 V del shunt (elevador MT3608) | aceptado [user] | C-HW-31 |
| P20 | Bloqueo `bomba_bloqueada` también al rearmar en seco dentro de los 10 min siguientes a un rearme | aceptado [user] | FR-BOM-07 |
| H-93 | Cola de eventos críticos; vigilante externo con T = 15 min y canal que no depende de la Pi; `falla_vref` + LED; cierre preventivo con la casa cerrada y N = 2 sondas en falla o falla de la referencia (con la casa ocupada, solo aviso); UPS de la cadena de red y broker por IPv4 con reservas DHCP como requisitos de instalación | aceptado [user] 2026-11-20 | FR-PUB-04, FR-SON-04/05, §25 |
| P22 | `casa_cerrada` es un parámetro de §19 que fijan las escenas de HA (`Casa cerrada` → true; `Preparar llegada` y llegada → false); se guarda en NVS; **valor de fábrica `true`** (sin configurar, gana la protección) | aceptado [user] H-95; riesgo de (b′) aceptado con la condición de FR-CFG-A5 y FR-VIG-02 (2026-11-21) | FR-SON-05 |
| P23 | Cola de eventos críticos en RAM con **32** entradas; llena, se descarta el evento más antiguo que no sea `fuga` y se cuenta en `eventos_perdidos` | aceptado [user] H-95 | FR-PUB-04 |
| P24 | Asociado al AP sin dirección IP durante **60 s** (o `IP_LOST` en CONECTANDO_BROKER u OPERATIVO, los estados con IP; en los demás se ignora [user, 2026-11-23]) → se trata como `wifi_down` | aceptado [user] H-95 (H1 del análisis del piloto) | FR-CON-A1 |
| P25 | Con el Wi-Fi asociado y el broker inalcanzable durante **10 min**, una reasociación del Wi-Fi, sin reiniciar; como mucho una vez por cada periodo de 10 min | aceptado [user] H-95 (H2 del análisis del piloto) | FR-CON-A2 |
| P26 | Bucle de reinicios: k = **3** reinicios en **10 min** | aceptado [user] H-93; el requisito va en la próxima ronda | próxima ronda |
| H-96 | Medidor testigo DAE AS200U-75P entre el medidor de la empresa de agua y la cisterna, leído por una entrada del nodo, sin la nube de DAE | aprobado [user] 2026-11-21 | §6.7, C-HW-32/33 |
| P28 | Entrada del medidor en **GPIO18**: libre, fuera de la lista de clima:C-HW-07, sin función de arranque ni de JTAG. Contacto seco a GND con pull-up externa de 10 kΩ, 1 kΩ en serie y 100 nF. Alternativa: GPIO21 | aceptado [user] H-96 | C-HW-30, C-HW-32 |
| P29 | Pulso válido = contacto cerrado ≥ 50 ms tras ≥ 50 ms abierto (el mismo antirrebote de FR-IO-05); conteo exacto hasta 5 pulsos/s. `galones_por_pulso` de fábrica **1,0, a confirmar con el manual del AS200U-75P** y calibrar en la obra con un balde de 5 galones; rango 0,1–100 | aceptado [user] H-96 | FR-MED-01/02 |
| P30 | Total y pulsos en NVS **como mucho una vez cada 60 s** y solo si cambiaron, y además antes de un reinicio ordenado. Ante un reinicio inesperado se pierden como mucho los pulsos de los últimos 60 s. Desgaste: ≤ 1440 escrituras al día con flujo continuo, repartidas por el nivelado de la NVS | aceptado [user] H-96 | FR-MED-03 |
| P31 | Caudal = litros de los últimos **5 min** × 12, en L/h, publicado con las medidas cada 60 s (P8) | aceptado [user] H-96 | FR-MED-04 |
| P32 | Alerta con `casa_cerrada`=true: más de **20 L** en **60 min** fuera de un periodo permitido. **HA abre el periodo** con `medidor/set` al empezar el riego o el relleno de la piscina (1–720 min), y el periodo se alarga **120 min** después, porque la cisterna se rellena cuando la boya abre, que puede ser después del riego. Alternativa: franjas horarias fijas en §19 (necesitan hora válida) | aceptado [user] H-96 | FR-MED-05/06 |
| P33 | `medidor_falla`: el nivel sube ≥ 10 cm (nivel válido) sin ningún pulso durante esa subida | aceptado [user] H-96 | FR-MED-08 |
| P34 | Con `casa_cerrada`=true, umbral **diario**: más de **50 L** en las últimas 24 h fuera de los periodos permitidos → `consumo_casa_cerrada` con `motivo`=`diario`; configurable (`consumo_diario_l`). Las fugas lentas solo se ven así: la boya rellena despacio y el medidor no cuenta bien por debajo de 0,25 GPM | aceptado [user] H-96 | FR-MED-09 |
| P27 | RSSI mínimo de instalación **−75 dBm** | aceptado [user] H-93; el requisito va en la próxima ronda | próxima ronda (hueco P8) |

**Próxima ronda del FSD (H-93):** huecos P4 (brownout al arrancar el motor y bucle de
reinicios, con P26), P5 (`red` mide la salida del inversor, no la red pública), P8 (elegir
el AP por señal, con P27), P9 (ejercicio vencido sin hora válida) y P10 (seco con el sensor
de presión en falla) de `testing/analisis-cortes-parciales.md`. P11 no se hace: lo cubre
el térmico de la bomba [user, H-93].

---

# Parte A — Lógica de aplicación (L2)

## 5. Protección de fugas y válvula general (A-11)

### 5.1 Propósito

Cerrar la válvula general ante una fuga confirmada por una sonda cableada activa,
mantenerla cerrada hasta una reapertura permitida y confirmar cada movimiento con
los finales de carrera.

### 5.2 Estados de la válvula

| Estado | Significado | Persistente |
|---|---|---|
| DESCONOCIDA | Ningún final activo (sin movimiento en curso) | No |
| ABIERTA | Final «abierta» activo, motor parado | Derivado de los finales |
| CERRADA | Final «cerrada» activo, motor parado | Derivado de los finales |
| CERRANDO / ABRIENDO | Motor en marcha hacia ese sentido | No |
| FALLA | Movimiento sin confirmar a tiempo, o los dos finales activos a la vez [user, 2026-11-15] | No (se reevalúa al arrancar) |

**Marcas persistentes** (NVS, §9): `bloqueo_fuga` (cierre por sonda cableada) y
`cerrada_por` ∈ {`ninguno`, `fuga`, `orden`, `bateria`, `ejercicio`, `sondas`}.

**Guarda `g_seco`:** ninguna sonda **activa** está en estado `mojada` (P13: una sonda
en falla cuenta como no mojada).

**Guarda `g_ciego`** (H-93): `casa_cerrada`=true **y** (≥ 2 sondas activas en falla según
FR-SON-02 **o** la referencia en falla según FR-SON-04).

### 5.3 Tabla de transiciones (normativa)

Todas las filas son [Must]. Procedencia: [user] D1, D5, D12, D18; [derived] el resto.

| Fila | Desde | Evento | Guarda | Hacia | Acción | Límite |
|---|---|---|---|---|---|---|
| V01 | ARRANQUE | fin de lectura de finales | solo «abierta» | ABIERTA | ninguna (no mueve el motor) | — |
| V02 | ARRANQUE | fin de lectura de finales | solo «cerrada» | CERRADA | ninguna | — |
| V03 | ARRANQUE | fin de lectura de finales | ninguno | DESCONOCIDA | publicar | — |
| V03b | ARRANQUE | fin de lectura de finales | los dos | FALLA | publicar; evento `falla_valvula` | — |
| V04 | ABIERTA, ABRIENDO, DESCONOCIDA | fuga confirmada | — | CERRANDO | `bloqueo_fuga`=1, `cerrada_por`=`fuga` | orden ≤ P2 |
| V05 | ABIERTA, DESCONOCIDA | orden `cerrar` válida | — | CERRANDO | `cerrada_por`=`orden` | ≤ 500 ms |
| V06 | CERRANDO | final «cerrada» | — | CERRADA | parar el motor | ≤ P3 |
| V07 | CERRANDO / ABRIENDO | tiempo de carrera > P3 | — | FALLA | parar el motor; evento `falla_valvula` | P3 |
| V08 | CERRADA | orden `abrir` válida | `g_seco` | ABRIENDO | `bloqueo_fuga`=0, `cerrada_por`=`ninguno` | ≤ 500 ms |
| V09 | CERRADA | orden `abrir` | no `g_seco` | CERRADA | rechazar `fuga_activa` | — |
| V10 | CERRADA | pulsación larga | `g_seco` | ABRIENDO | igual que V08 | ≤ 500 ms |
| V11 | CERRADA | pulsación larga | no `g_seco` | CERRADA | patrón de rechazo (§8) | — |
| V12 | ABRIENDO | final «abierta» | — | ABIERTA | parar el motor | ≤ P3 |
| V13 | ABIERTA | ejercicio vencido | `g_seco`, `bloqueo_fuga`=0 | CERRANDO | `cerrada_por`=`ejercicio` | — |
| V14 | CERRADA | final «cerrada» confirmado | `cerrada_por`=`ejercicio` y `g_seco` | ABRIENDO | registrar tiempo de carrera | — |
| V15 | ABIERTA / CERRADA | cambia un final sin orden | — | solo «abierta» → ABIERTA; solo «cerrada» → CERRADA; ninguno → DESCONOCIDA; los dos → FALLA | evento `movimiento_manual` (P12); con los dos, `falla_valvula` | — |
| V16 | FALLA | orden `cerrar` u `abrir` válida, o pulsación larga | según V05/V08/V10 | CERRANDO / ABRIENDO | un reintento por orden | P3 |
| V17 | CERRADA | fuga confirmada | — | CERRADA | `bloqueo_fuga`=1, `cerrada_por`=`fuga` | — |
| V18 | ABRIENDO | fuga confirmada | — | CERRANDO | como V04 | ≤ P2 |
| V19 | CERRANDO, ABRIENDO | orden de válvula | — | sin cambio | rechazar `ocupado` (P15) | — |

**Completitud.** Cualquier par (estado × evento) no listado se **ignora** y se
registra en el log con `val: ignorado <estado> <evento>`. Una fuga confirmada en
FALLA dispara el reintento de cierre de V16 (fila V20).

| Fila | Desde | Evento | Guarda | Hacia | Acción | Límite |
|---|---|---|---|---|---|---|
| V20 | FALLA | fuga confirmada | — | CERRANDO | `bloqueo_fuga`=1, `cerrada_por`=`fuga`; un reintento | P3 |
| V21 | CERRANDO | fuga confirmada | — | CERRANDO (sigue cerrando) | `bloqueo_fuga`=1, `cerrada_por`=`fuga` | — |
| V22 | ABIERTA, DESCONOCIDA | batería `critica` y red `ausente` (FR-ENE-04) | — | CERRANDO | `cerrada_por`=`bateria` | ≤ 500 ms |
| V23 | CERRADA | reapertura por batería cumplida (FR-ENE-05) | `cerrada_por`=`bateria`, `bloqueo_fuga`=0, `g_seco` y no `g_ciego` (H-93) | ABRIENDO | `cerrada_por`=`ninguno` | ≤ 500 ms |
| V24 | ABIERTA, DESCONOCIDA | `g_ciego` (FR-SON-05) | — | CERRANDO | `cerrada_por`=`sondas`; `bloqueo_fuga` sin cambio | ≤ 500 ms |
| V25 | CERRADA | orden `abrir` válida o pulsación larga | `g_seco` y `g_ciego` | CERRADA | rechazar `sondas_en_falla`; con la pulsación, patrón de rechazo (§8) | — |

Filas V20–V23 [user, 2026-11-15]. Filas V24 y V25 [user] H-93; V24 es de nivel, como V22.
V25 tiene prioridad sobre V08 y V10: mientras se cumple `g_ciego` no se reabre, porque V24
volvería a cerrar. No hay reapertura automática cuando `g_ciego` deja de cumplirse: la
reapertura es la de D5 (remota con todo seco, o en persona); con la llegada, HA pone
`casa_cerrada`=false y la reapertura vuelve a ser posible [derived]. **Una fuga confirmada siempre gana:** en ABIERTA,
DESCONOCIDA, ABRIENDO, CERRANDO, FALLA y CERRADA (V04, V18, V21, V20, V17) la válvula
termina CERRADA con `bloqueo_fuga`=1 y `cerrada_por`=`fuga`. El par (ARRANQUE × fuga
confirmada) es imposible por construcción: ARRANQUE dura la primera lectura de los
finales (antirrebote de 50 ms, FR-IO-05) y una confirmación necesita 4 muestras cada
250 ms (FR-FUG-01). Los eventos de V22 y V23 son de nivel: la tarea de protección los
envía en cada ciclo mientras se cumple su condición. V14 deja `cerrada_por`=`ninguno`.

### 5.4 Requisitos

- **FR-FUG-01** [Must] [user] D18 (P2): una sonda activa que pase a `mojada` deberá
  quedar **confirmada** tras 4 muestras seguidas en `mojada` con muestreo cada 250 ms.
- **FR-FUG-02** [Must] [user] D4 (P2): con una fuga confirmada y la válvula en ABIERTA,
  el nodo deberá activar IN1 (cerrar) en ≤ 500 ms, sin ninguna condición de red.
- **FR-FUG-03** [Must] [derived] D7: una fuga confirmada no deberá cambiar el permiso
  de la bomba.
- **FR-VAL-04** [Must] [derived] (P3): si el final esperado no se activa dentro de
  `t_carrera_max_s`, el nodo deberá cortar el motor y pasar a FALLA (V07).
- **FR-VAL-05** [Must] [derived]: el motor deberá quedar sin alimentación (IN1 = IN2 = 0)
  en ≤ 100 ms desde que se activa el final esperado.
- **FR-VAL-06** [Must] [user] D18: el nodo no deberá mover el motor en el arranque (V01–V03).
- **FR-VAL-07** [Must] [user] D5: una orden `abrir` con alguna sonda activa en `mojada`
  deberá rechazarse con el motivo `fuga_activa` y la válvula no deberá moverse (V09).
- **FR-VAL-08** [Must] [user] D5: tras una reapertura (V08/V10), una nueva fuga
  confirmada deberá cerrar la válvula otra vez (V04).
- **FR-VAL-09** [Must] [user] D5: la pulsación larga deberá abrir con las mismas guardas
  que la orden remota (V10/V11).
- **FR-VAL-10** [Must] [derived] (P12): un movimiento hecho con la palanca manual no
  deberá provocar ningún movimiento del motor por parte del nodo (V15).
- **FR-VAL-11** [Must] [derived]: el nodo no deberá activar IN1 e IN2 a la vez.

### 5.5 Contratos de verificación

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-FUG-01 | Lógica pura: secuencias de muestras | Confirma con 4 `mojada` seguidas; no confirma con 3 + 1 `seca` | Confirmar con < 4; no confirmar con 4 | host |
| FR-FUG-02 | Válvula ABIERTA; **broker apagado y Wi-Fi caído**; el banco pasa la sonda 3 a `mojada` | IN1 en alto ≤ 1,5 s desde el estímulo; línea `val: ABIERTA -> CERRANDO (fuga s3)` | Esperar a la red; IN2 en alto; ninguna acción | bench |
| FR-FUG-02 | Igual, con el nodo en PORTAL, en ESPERA y durante OTA_DESCARGA (Fase 3) | Igual | Igual | bench |
| FR-FUG-03 | Fuga confirmada con el permiso en PERMITIDA | GPIO14 sigue en bajo | Permiso cortado | bench |
| FR-VAL-04 | CERRANDO; el banco no activa el final «cerrada» | A P3 ± 1 s: IN1 = IN2 = 0, estado FALLA, evento `falla_valvula` | Motor en marcha > P3 + 1 s | bench |
| FR-VAL-05 | CERRANDO; el banco activa el final «cerrada» | IN1 = 0 en ≤ 100 ms | Motor en marcha tras 100 ms | bench |
| FR-VAL-06 | 10 arranques con cada combinación de finales (A, C, ninguno, ambos) | IN1 = IN2 = 0 durante todo el arranque | Un pulso del motor | bench |
| FR-VAL-07 | CERRADA, sonda 2 `mojada`; orden `abrir` | Respuesta `rechazado: fuga_activa`; IN2 = 0 | Movimiento | bench |
| FR-VAL-08 | Reapertura válida; luego sonda 5 `mojada` | Cierre según FR-FUG-02 | Quedarse abierta | bench |
| FR-VAL-09 | CERRADA con fuga activa: pulsación de 3 s; luego seca: pulsación de 3 s | 1.º: patrón de rechazo, sin movimiento. 2.º: ABRIENDO | Abrir con fuga | bench |
| FR-VAL-10 | CERRADA con `bloqueo_fuga`; el banco simula la palanca (final «abierta» activo, «cerrada» inactivo) | Evento `movimiento_manual`; IN1 = IN2 = 0 durante 60 s | Intento de cierre por el motor | bench |
| FR-VAL-11 | Lógica pura: todas las filas de §5.3 y combinaciones de eventos simultáneos | Nunca IN1 = IN2 = 1 | Cualquier caso con los dos en alto | host + bench |
| V01–V25 | Tabla de transiciones como prueba parametrizada | Cada fila produce su estado y su acción | Una transición no listada | host |

### 5.6 Ejercicio periódico (D12)

- **FR-EJE-01** [Must] [user] D12: si han pasado `ejercicio_dias` desde el último
  movimiento completo y la hora local es `ejercicio_hora`, el nodo deberá ejecutar
  V13 → V06 → V14 → V12, siempre que se cumplan las guardas de V13 y la hora sea
  válida (clima:FR-TIME-02).
- **FR-EJE-02** [Must] [user] D12: el nodo deberá medir y publicar el tiempo de carrera
  de cierre y de apertura del ejercicio en `valvula/state` (`t_cierre_ms`, `t_apertura_ms`).
- **FR-EJE-03** [Must] [user] D12: si una fuga se confirma durante el ejercicio, la
  válvula deberá terminar CERRADA con `bloqueo_fuga`=1 (V17/V18).
- **FR-EJE-04** [Should] [user] D12: una orden `ejercicio` deberá iniciar un ejercicio
  con las mismas guardas, y la orden `posponer_ejercicio` deberá reiniciar la cuenta.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-EJE-01 | Lógica pura con un reloj simulado | Dispara solo en la hora configurada y con los días cumplidos | Disparo sin hora válida, con bloqueo o con sonda mojada | host |
| FR-EJE-01, FR-EJE-02 | `ejercicio_dias`=1, hora = ahora + 2 min; el banco responde a los finales con 3 s de retardo | Cierre y apertura; `t_cierre_ms` ≈ 3000 ± 300 | Quedarse cerrada sin fuga | bench |
| FR-EJE-03 | Durante CERRANDO del ejercicio, sonda 1 `mojada` | Termina CERRADA con `bloqueo_fuga`=1 | Reapertura | bench |
| FR-EJE-04 | Orden `ejercicio` con todo seco | Ciclo completo | Rechazo sin motivo | bench |
| FR-EJE-04 | Lógica pura: `posponer_ejercicio` a los 29 días | El siguiente ejercicio, `ejercicio_dias` después de la orden | Ejercicio al día 30 | host |

## 6. Bomba y cisterna (A-12)

### 6.1 Estados del permiso de la bomba

| Estado | Relé (GPIO14) | Persistente |
|---|---|---|
| PERMITIDA | sin energía (contacto NC cerrado) | Sí |
| CORTADA(`seco` \| `nivel` \| `flotador` \| `orden`) | energizado | Sí, con el motivo |
| BLOQUEADA | energizado | Sí, con los intentos |

### 6.2 Tabla de transiciones (normativa)

| Fila | Desde | Evento | Guarda | Hacia | Acción |
|---|---|---|---|---|---|
| B01 | PERMITIDA | trabajo en seco (FR-BOM-03) | — | CORTADA(seco) | intentos_24h += 1 |
| B02 | PERMITIDA | flotador sin agua | — | CORTADA(flotador) | — |
| B03 | PERMITIDA | nivel < `nivel_critico_pct` con transductor válido | — | CORTADA(nivel) | — |
| B04 | PERMITIDA | orden `cortar` | — | CORTADA(orden) | — |
| B05 | CORTADA(seco\|nivel\|flotador) | condición de rearme cumplida (FR-BOM-05) | motivo `nivel` o `flotador`; o motivo `seco` con intentos_24h < `intentos_max` | PERMITIDA | — |
| B06 | CORTADA(seco) | condición de rearme cumplida | intentos_24h ≥ `intentos_max` | BLOQUEADA | evento `bomba_bloqueada` |
| B07 | CORTADA(any), BLOQUEADA | orden `rearmar` o pulsación larga | flotador con agua | PERMITIDA | intentos_24h = 0 |
| B08 | CORTADA(any), BLOQUEADA | orden `rearmar` o pulsación larga | flotador sin agua | sin cambio | rechazar `sin_agua` |
| B09 | CORTADA(orden) | orden `permitir` | flotador con agua y (nivel ≥ crítico o transductor en falla) | PERMITIDA | — |
| B10 | CORTADA(orden) | orden `permitir` | flotador sin agua | sin cambio | rechazar `sin_agua` |
| B11 | CORTADA(orden) | orden `permitir` | flotador con agua, transductor válido y nivel < crítico | sin cambio | rechazar `nivel_bajo` |

Filas B05, B09–B11 [user, 2026-11-15]: solo los cortes por seco cuentan como intentos y
para el bloqueo de 3 en 24 h; los cortes por nivel o flotador se rearman solos al
recuperarse. Con el transductor en falla, la bomba sigue protegida por el flotador y
la detección de seco, y el nivel se publica como `falla` (FR-NIV-02).

### 6.3 Requisitos

- **FR-BOM-01** [Must] [user] D3: con el permiso CORTADA o BLOQUEADA, GPIO14 deberá
  estar en alto; en PERMITIDA, en bajo.
- **FR-BOM-02** [Must] [user] D3: desde el reset hasta restaurar el estado de NVS,
  GPIO14 deberá estar en bajo, y el estado de NVS deberá aplicarse en ≤ 2 s desde el reset.
- **FR-BOM-03** [Must] [user] D9 (P6): el nodo deberá declarar **trabajo en seco**
  cuando, durante más de `t_seco_s` seguidos, la corriente sea ≥ `i_marcha_a` **y**
  < `i_seco_a` **y** la presión sea < `p_min_bar`. Mientras `i_seco_a` no esté
  configurado, el criterio de corriente no se evalúa y el nodo deberá publicar
  `bomba/state.calibrada`=false.
- **FR-BOM-04** [Must] [user] D8: el flotador sin agua deberá cortar el permiso (B02)
  aunque el transductor esté en falla o indique otro nivel, salvo con el flotador en
  falla (FR-BOM-10).
- **FR-BOM-05** [Must] [user] D10 (P6): la condición de rearme automático es
  nivel ≥ `nivel_rearme_pct` **y** flotador con agua, sin interrupción, durante
  `t_rearme_min`.
- **FR-BOM-06** [Must] [derived] (P19): con el transductor en falla, B05 no deberá
  ejecutarse.
- **FR-BOM-07** [Must] [user] D10 (P6, P20): al alcanzar `intentos_max` cortes por
  seco en 24 h móviles, el permiso deberá pasar a BLOQUEADA y solo deberá salir por
  B07.
- **FR-BOM-08** [Must] [user] D3: ninguna orden ni valor de configuración deberá
  llevar el permiso a PERMITIDA con el flotador sin agua, salvo con el flotador en
  falla (FR-BOM-10).
- **FR-BOM-09** [Must] [user] D7: el estado de la válvula no deberá cambiar el permiso
  de la bomba.
- **FR-BOM-10** [Must] [user, 2026-11-15]: con el transductor válido indicando nivel
  ≥ `nivel_rearme_pct` y el flotador sin agua durante 10 min seguidos, el nodo deberá
  declarar el **flotador en falla**: publicar una vez el evento `flotador_falla` y, mientras
  dure, tratar el flotador como «con agua» en B02, B05, B07 y B09, de modo que la bomba
  quede protegida por el transductor (B03) y la detección de seco (B01), el mismo
  criterio que B09 con el transductor en falla. La falla termina cuando el flotador
  vuelve a «con agua» o cuando el transductor deja de ser válido; en ese caso se aplica
  B02 otra vez. El flotador sigue con pull-up: un cable cortado se lee «sin agua».

### 6.4 Contratos de verificación

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-BOM-01 | Cada estado de §6.1 | Nivel de GPIO14 según la tabla | Otro nivel | bench |
| FR-BOM-02 | NVS con CORTADA(seco); reset | GPIO14 en bajo durante el arranque y en alto ≤ 2 s tras el reset | Alto antes del reset lógico; bajo después de 2 s | bench |
| FR-BOM-03 | Lógica pura: series de corriente y presión | Declara seco solo con las tres condiciones durante > `t_seco_s` | Seco con presión alta; seco con 1 s de menos | host |
| FR-BOM-03 | Banco: senoide equivalente a 0,8 A (calibrado `i_seco_a`=1,0), presión 0,2 bar, 11 s | CORTADA(seco) a los 10 ± 1 s | No cortar; cortar con presión 2 bar | bench |
| FR-BOM-03 | `i_seco_a` sin configurar | `calibrada`=false; no hay corte por corriente | Corte por corriente | bench |
| FR-BOM-04 | Transductor indicando 80 %; flotador sin agua | CORTADA(flotador) ≤ 1 s | Seguir permitida | bench |
| FR-BOM-05 | CORTADA(seco); nivel 35 %, flotador con agua durante 10 min | PERMITIDA a los 10 min ± 10 s | Rearme con una interrupción del flotador a los 5 min | host + bench (tiempos reducidos por configuración de prueba) |
| FR-BOM-06 | CORTADA(seco), transductor a 2 mA, flotador con agua | Sin rearme en 2 × `t_rearme_min` | Rearme | bench |
| FR-BOM-07 | 3 cortes por seco en 24 h simuladas | BLOQUEADA; `bomba_bloqueada`; solo `rearmar` la saca | Rearme automático | host + bench |
| FR-BOM-08 | Flotador sin agua; órdenes `permitir`, `rearmar`; configuración con cualquier valor dentro de rango | Rechazo `sin_agua`; GPIO14 en alto | GPIO14 en bajo | bench |
| FR-BOM-09 | Válvula CERRADA y ABIERTA | Permiso sin cambio | Corte por la válvula | bench |
| FR-BOM-10 | Lógica pura con planta simulada: nivel 50 % y flotador sin agua 10 min; después transductor en falla | B02 al empezar; `flotador_falla` a los 10 min y PERMITIDA; con el transductor en falla, CORTADA(flotador) otra vez | `flotador_falla` antes de 10 min o con nivel < `nivel_rearme_pct`; seguir permitida sin transductor | host |
| FR-BOM-10 | Banco: flotador abierto, transductor a 12 mA durante 10 min | `flotador_falla` y relé en bajo | Relé alto tras el evento | bench |
| B01–B11 | Tabla como prueba parametrizada | Cada fila produce su estado | Transición no listada | host |

### 6.5 Nivel y presión

- **FR-NIV-01** [Must] [user] D8: el nivel deberá calcularse como
  `(I − 4 mA) / 16 mA × rango_transductor_cm` y publicarse en cm y en % de
  `altura_util_cm`.
- **FR-NIV-02** [Must] [user] D8 (P18): una corriente < 3,8 mA o > 20,5 mA deberá
  marcar el transductor en `falla` y publicar el nivel como no válido.
- **FR-PRE-01** [Must] [derived] (P9): la presión deberá calcularse como
  `(V − 0,5) / 4,0 × 10 bar`; una tensión < 0,3 V o > 4,7 V deberá marcar el sensor
  en `falla`.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-NIV-01 | Función pura con 4, 12 y 20 mA, rango 500 cm, altura útil 400 cm | 0 cm; 250 cm / 62,5 %; 500 cm / 100 % (saturado) | Otro valor; % > 100 | host |
| FR-NIV-01 | El banco inyecta 12,00 mA | Nivel publicado a ± 2 % del rango | Error mayor | bench |
| FR-NIV-02 | 3,5 mA y 21 mA | `falla` en ≤ 2 s | Nivel numérico válido | bench |
| FR-PRE-01 | Función pura con 0,5 / 2,5 / 4,5 V; banco con 2,5 V | 0 / 5 / 10 bar; ± 0,2 bar en banco | Otro valor | host + bench |

### 6.6 Ciclado anormal (D11)

- **FR-CIC-01** [Must] [user] D11 (P7): el nodo deberá contar los arranques de la bomba
  en una ventana móvil de `ciclado_ventana_min`.
- **FR-CIC-02** [Must] [user] D11 (P7): al alcanzar `ciclado_max` arranques en la ventana,
  el nodo deberá publicar el evento `ciclado_anormal` con el número de arranques y el
  estado de la válvula (`abierta` o `cerrada`), una vez por ventana: como mucho un aviso
  cada `ciclado_ventana_min`, contados desde el aviso [user, 2026-11-15].
- **FR-CIC-03** [Must] [user] D11: el ciclado no deberá mover la válvula ni cambiar el
  permiso de la bomba.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-CIC-01, FR-CIC-02 | Lógica pura: 6 arranques en 50 min; 5 en 60 min | Un evento en el primer caso; ninguno en el segundo | Evento con 5; dos eventos en la misma ventana | host |
| FR-CIC-02 | Banco: 6 arranques simulados con la válvula CERRADA | Evento con `valvula`=`cerrada` | Otro valor | bench |
| FR-CIC-03 | Igual | Válvula y permiso sin cambios | Cambio | bench |

### 6.7 Medidor testigo de agua (H-96)

El DAE AS200U-75P (C-HW-33) mide todo lo que entra a la cisterna: lo mismo que factura
la empresa de agua. Su salida de pulsos es un contacto seco (reed) en GPIO18 (C-HW-32). Sirve de testigo
de la factura, deja un histórico en HA y avisa de consumo con la casa cerrada. Las fugas
después de la bomba ya las ven la corriente y la presión (§6). El medidor agrega la boya
que no cierra (la cisterna rebosa) y las fugas de la acometida. Una fuga lenta de la casa
baja la cisterna poco a poco y la boya la rellena despacio, a un caudal que el medidor
puede no contar bien (< 0,25 GPM): por eso hay un umbral por hora y otro diario (P34).

- **FR-MED-01** [Must] [user] H-96 (P29): el nodo deberá contar un pulso por cada cierre del
  contacto que dure ≥ 50 ms después de ≥ 50 ms abierto. Los rebotes más cortos no deberán
  sumar pulsos. El conteo deberá seguir en cualquier estado de conectividad, de la válvula
  y de la bomba.
- **FR-MED-02** [Must] [user] H-96 (P29): cada pulso deberá sumar `galones_por_pulso` ×
  3,78541 L al total acumulado, con el valor del parámetro en ese momento. Un cambio de
  `galones_por_pulso` no deberá recalcular lo ya acumulado. El total no deberá bajar nunca.
- **FR-MED-03** [Must] [user] H-96 (P30): el total y los pulsos deberán guardarse en NVS
  como mucho una vez cada 60 s y solo si cambiaron. También deberán guardarse antes de un
  reinicio ordenado (OTA, orden). Tras cualquier reinicio deberán restaurarse, con una
  pérdida de como mucho los pulsos de los últimos 60 s.
- **FR-MED-04** [Must] [user] H-96 (P31): el nodo deberá publicar `medidor` con el total en
  litros, el caudal en L/h y los pulsos, con las medidas (FR-PUB-03).
- **FR-MED-05** [Must] [user] H-96 (P32): con `casa_cerrada`=true, si los litros contados
  fuera de un periodo permitido en los últimos `consumo_ventana_min` superan
  `consumo_alerta_l`, el nodo deberá publicar el evento `consumo_casa_cerrada` con
  `motivo`=`hora`, los litros y la ventana.
  Deberá publicarlo una vez por episodio: el siguiente solo después de una ventana completa
  sin pulsos. El evento es crítico: pasa por la cola de FR-PUB-04 con las mismas reglas.
- **FR-MED-06** [Must] [user] H-96 (P32): la orden `medidor/set`
  `{"accion":"permitir_consumo","min":<1–720>}` deberá abrir un periodo permitido hasta
  ahora + `min` + `consumo_margen_min`. Una orden nueva solo alarga el periodo, nunca lo
  acorta. Un payload no válido se rechaza como en FR-ORD-02. El periodo vive en RAM: un
  reinicio lo borra.
- **FR-MED-07** [Must] [derived] H-96: el medidor no deberá mover la válvula ni cambiar el
  permiso de la bomba (como FR-CIC-03).
- **FR-MED-09** [Must] [user] H-96 (P34): con `casa_cerrada`=true, si los litros contados
  fuera de un periodo permitido en las últimas 24 h superan `consumo_diario_l`, el nodo
  deberá publicar el evento `consumo_casa_cerrada` con `motivo`=`diario` y los litros, como
  mucho una vez cada 24 h contadas desde el aviso. Es independiente del umbral por hora:
  pueden salir los dos. Es crítico como en FR-MED-05. La suma de 24 h vive en RAM: un
  reinicio la pone a cero.
- **FR-MED-08** [Should] [derived] H-96 (P33): si el nivel válido de la cisterna sube
  ≥ 10 cm sin ningún pulso durante esa subida, el nodo deberá publicar `medidor_falla` una
  vez, hasta el siguiente pulso.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-MED-01 | Lógica pura con marcas de tiempo: pulsos de 60/60 ms, de 40 ms, cierres con 5 rebotes de 3 ms, 5 pulsos/s | Un pulso por cierre ≥ 50 ms; ninguno por el de 40 ms; uno por el cierre con rebotes | Pulsos de más o de menos | host |
| FR-MED-01 | El banco da 1000 cierres de 100 ms cada 200 ms, con 3 rebotes de 2 ms en cada flanco; el Wi-Fi cae a la mitad | `pulsos` = 1000 exactos | Otro número; conteo parado sin Wi-Fi | bench |
| FR-MED-02 | Lógica pura: 10 pulsos a 1,0 gal; cambio a 0,5; 10 pulsos más | 37,85 L y luego 56,78 L (± 0,01 L) | Recalcular los 10 primeros; total que baja | host |
| FR-MED-03 | Lógica pura con reloj simulado: pulsos seguidos 5 min; 10 min sin pulsos | ≤ 1 escritura cada 60 s; 0 escrituras sin cambios | Escritura por pulso; escrituras sin cambios | host |
| FR-MED-03 | El banco da 300 pulsos (uno cada 200 ms); reinicio con el comando serie `reiniciar` (E13, imagen de banco). Luego 300 más, 90 s de espera y corte de alimentación | Tras el reinicio: 300 exactos. Tras el corte: 600 exactos; `nvs=1` en la línea A8 | Total menor que el guardado; total a cero | bench |
| FR-MED-04 | Lógica pura: 50 L en los últimos 5 min | `caudal_lh` = 600 | Otro valor | host |
| FR-MED-04 | Suscriptor a `medidor`; 5 min con un pulso cada 6 s a 1,0 gal | 5 ± 1 publicaciones; `caudal_lh` ≈ 2271 ± 5 % al final; `total_l` creciente | Total que baja; sin publicaciones | bench |
| FR-MED-05, FR-MED-06 | Lógica pura: `casa_cerrada` × periodo (sin, dentro, en el margen, vencido) × litros (19, 21) | Evento `motivo`=`hora` solo con true, fuera de periodo y 21 L; uno por episodio | Evento con false, dentro del periodo o en el margen; dos en un episodio | host |
| FR-MED-09 | Lógica pura con reloj simulado: 2,5 L/h durante 30 h (por debajo del umbral por hora); igual con `casa_cerrada`=false; igual dentro de periodos permitidos | Con true: un evento `diario` al pasar de 50 L (hora 20) y ninguno más antes de 24 h desde el aviso; con false o dentro de periodos, ninguno | Evento con false; dos en 24 h; `motivo`=`hora` | host |
| FR-MED-09 | Lógica pura: 40 L acumulados en 24 h y reinicio | La suma de 24 h vuelve a 0 tras el reinicio | Suma conservada | host |
| FR-MED-09 | `casa_cerrada`=true, `consumo_alerta_l`=1000, `consumo_diario_l`=10; el banco da 3 pulsos (11,4 L a 1,0 gal) | Un `consumo_casa_cerrada` con `motivo`=`diario` | `motivo`=`hora`; ningún evento | bench |
| FR-MED-05 | `casa_cerrada`=true; broker apagado; 25 L en 10 min; broker encendido a los 3 min | `consumo_casa_cerrada` desde la cola, con `edad_s` | Evento perdido | bench |
| FR-MED-06 | Lógica pura: periodo hasta t+90; orden de 10 min en t+20; reinicio dentro del periodo | El periodo sigue hasta t+90 (no se acorta); tras el reinicio, sin periodo | Periodo acortado; periodo que sobrevive al reinicio | host |
| FR-MED-06 | `permitir_consumo` 30 min; 25 L a los 10 min; `min`=0, `min`=721, `{"accion":5}` | Sin evento; las tres órdenes inválidas se rechazan con `orden_rechazada` | Evento dentro del periodo; orden inválida aceptada | bench |
| FR-MED-07 | Válvula ABIERTA, permiso PERMITIDA; provocar `consumo_casa_cerrada` | Válvula y permiso sin cambios | Movimiento o corte | bench |
| FR-MED-08 | Lógica pura: nivel de 100 → 112 cm sin pulsos; con 1 pulso; nivel en falla | Un `medidor_falla` en el primer caso; ninguno en los otros | Evento con pulsos o con el nivel en falla | host |

## 7. Energía y batería (D2, idea #33)

- **FR-ENE-01** [Must] [user] D2: el nodo deberá publicar `red` = `presente`/`ausente`
  según GPIO5, con retain, ≤ 1 s tras el cambio (en OPERATIVO).
- **FR-ENE-02** [Must] [user] idea #33 (P5): el nodo deberá publicar `bateria` con la
  tensión y el estado `ok`/`baja`/`critica` según los umbrales y la histéresis de P5,
  con retain, en cada cambio de estado y en cada ciclo de medidas.
- **FR-ENE-03** [Must] [user] idea #33: el nodo deberá publicar los eventos `bateria_baja`
  y `bateria_critica` una vez por cambio de estado. El recordatorio periódico hasta el
  cambio es de HA.
- **FR-ENE-04** [Must] [user] D2 (P16): con la batería `critica` y la red `ausente`,
  la válvula deberá cerrarse con `cerrada_por`=`bateria`.
- **FR-ENE-05** [Must] [derived] (P16): con `cerrada_por`=`bateria`, la red `presente`
  durante 5 min, Vbat ≥ 12,8 V y `bloqueo_fuga`=0, la válvula deberá reabrirse.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-ENE-01 | El banco cambia GPIO5 | `red` cambia ≤ 1 s | Sin publicación | bench |
| FR-ENE-02 | Función pura de umbrales con rampas de subida y bajada | Cambios en 12,4/12,0 al bajar y en 12,6/12,2 al subir | Oscilación con ruido de ± 0,1 V | host |
| FR-ENE-02 | El banco ajusta la tensión del divisor | `bateria.v` a ± 0,1 V | Error mayor | bench |
| FR-ENE-03 | Bajar de 12,6 V a 12,3 V y a 11,9 V | Un `bateria_baja` y un `bateria_critica` | Repetidos sin cambio de estado | bench |
| FR-ENE-04 | ABIERTA; red ausente; Vbat 11,9 V | CERRANDO, `cerrada_por`=`bateria` | Seguir abierta; cerrar con red presente | bench |
| FR-ENE-05 | Tras FR-ENE-04: red presente 5 min, 12,9 V, todo seco | ABRIENDO a 5 min ± 10 s | Reabrir con `bloqueo_fuga`=1 | bench |

## 8. Interfaz local (D16)

### 8.1 Requisitos

- **FR-LOC-01** [Must] [user] D16: con una fuga confirmada, el zumbador deberá sonar
  durante `zumbador_min` minutos o hasta una pulsación corta.
- **FR-LOC-02** [Must] [user] D16: la pulsación corta (< 1 s) deberá silenciar el zumbador
  y no deberá cambiar el estado de la válvula ni de la bomba.
- **FR-LOC-03** [Must] [user] D16: la pulsación larga (≥ 3 s) deberá ejecutar V10/V11 y
  B07/B08.
- **FR-LOC-04** [Must] [user] D16 (P17): el LED deberá mostrar el patrón de §8.2 del
  estado de mayor prioridad.
- **FR-LOC-05** [Should] [user] D16: con `zumbador_activo`=false, el zumbador no deberá sonar.

### 8.2 Patrones (P17)

| Prioridad | Condición | LED | Zumbador |
|---|---|---|---|
| 1 | Fuga confirmada | rojo fijo | intermitente 1 Hz (FR-LOC-01) |
| 2 | Válvula FALLA, bomba BLOQUEADA, sonda en falla, referencia de los lazos en falla (FR-SON-04, H-93) o batería crítica | rojo 1 Hz | 3 pitidos cada 10 min |
| 3 | Válvula CERRADA o bomba CORTADA | rojo 0,25 Hz | — |
| 4 | Sin red o sin broker | verde 1 Hz | — |
| 5 | Todo normal | verde fijo | — |
| — | Pulsación larga rechazada | rojo 5 Hz durante 2 s | 1 pitido largo |

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-LOC-01 | Fuga; esperar | Zumbador activo durante `zumbador_min` ± 5 s | Silencio antes; seguir después | bench |
| FR-LOC-02 | Fuga; pulsación de 0,5 s | Zumbador apagado; válvula y bomba sin cambios | Movimiento o rearme | bench |
| FR-LOC-03 | Pulsación de 2,5 s y de 3,5 s | Solo la de 3,5 s actúa | Acción con 2,5 s | bench |
| FR-LOC-04 | Lógica pura con combinaciones de condiciones | Patrón de la mayor prioridad | Otro patrón | host |
| FR-LOC-05 | `zumbador_activo`=false; fuga | Zumbador apagado; la válvula cierra | Sonido | bench |

## 9. Persistencia de bloqueos y arranque (D15)

- **FR-PER-01** [Must] [user] D15: `bloqueo_fuga`, `cerrada_por`, el estado del permiso
  de la bomba, su motivo y los instantes de los cortes por seco de las últimas 24 h
  deberán guardarse en NVS en cada cambio y solo en los cambios.
- **FR-PER-02** [Must] [user] D15: tras cualquier reinicio (corte de alimentación,
  watchdog, pánico, OTA), los valores de FR-PER-01 deberán restaurarse; ningún reinicio
  deberá poner `bloqueo_fuga` en 0 ni el permiso en PERMITIDA. Los cortes por seco
  restaurados cuentan como ocurridos al arrancar (el reloj monótono empieza de cero)
  [user, 2026-11-15].
- **FR-PER-03** [Must] [user] D15: si al arrancar hay una sonda activa en `mojada`, la
  fuga deberá confirmarse y tratarse como en marcha (FR-FUG-01/02).

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-PER-01 | 1 h en ABIERTA y PERMITIDA, sin cambios | 0 escrituras en NVS de esas claves (contador de escrituras en el log) | Escrituras periódicas | bench |
| FR-PER-02 | `bloqueo_fuga`=1 y BLOQUEADA; reinicio por corte, por `esp_restart`, por pánico forzado (imagen de banco) y tras OTA (Fase 3) | Tras cada uno: `bloqueo_fuga`=1, BLOQUEADA, válvula sin movimiento | Cualquier rearme | bench |
| FR-PER-03 | Sonda 4 `mojada` antes del arranque; válvula ABIERTA | CERRANDO ≤ 1,5 s después de terminar el arranque | Seguir abierta | bench |

## 10. Gestor de conectividad — heredado

Se heredan **clima §6 completo**: estados, tabla T01–T25, completitud y
clima:FR-CON-01…07, con sus contratos.

### 10.1 Adaptaciones

- **FR-HER-01** [Must] [derived]: los requisitos heredados aplican cambiando el
  identificador `clima-…` por el de §18 y «el ciclo de medición sigue» por «la
  protección (§5–§9) sigue».
- **FR-HER-02** [Must] [user] D18: en todos los estados de conectividad (incluidos
  PORTAL, VALIDANDO, ESPERA y OTA_DESCARGA) deberán cumplirse los plazos de FR-FUG-02,
  FR-BOM-03 y FR-BOM-04.
- **FR-CON-A1** [Must] [derived] H-93, H1 del análisis del piloto (P24): el evento
  `IP_LOST` del Wi-Fi en CONECTANDO_BROKER u OPERATIVO, o seguir asociado al AP sin
  dirección IP durante 60 s, deberá tratarse como `wifi_down` (T08 de clima §6), con su
  backoff y sin reiniciar. En los demás estados (sin IP), `IP_LOST` no deberá soltar la
  asociación ni escribir ninguna línea `con:` [user, 2026-11-23].
- **FR-CON-A2** [Must] [derived] H-93, H2 del análisis del piloto (P25): con el Wi-Fi
  asociado y con IP, y el broker inalcanzable durante 10 min seguidos, el nodo deberá
  reasociar el Wi-Fi una vez, sin reiniciar, y no más de una vez por cada 10 min. Aclara
  clima:FR-CON-04: el plazo de ≤ 75 s cuenta desde que el broker vuelve, también después
  de una reasociación.

Si el piloto adopta H1 y H2 con otros valores, se alinean aquí por enmienda (§«Convenciones»).

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-CON-A1 | Lógica pura del gestor: `IP_LOST` en cada estado; asociado sin IP 59 s y 61 s | `wifi_down` con `IP_LOST` solo en CONECTANDO_BROKER y OPERATIVO; en los demás, sin efecto y sin tocar la cuenta de 60 s; `wifi_down` a los 61 s, nada a los 59 s | Soltar la asociación con `IP_LOST` sin IP; quedarse asociado sin IP > 60 s + 1 s; reinicio | host |
| FR-CON-A1 | Banco: nodo configurado, AP del banco apagado 10 min (TC-CON-06) | Ninguna línea `con: ip perdida`; solo el ciclo de backoff | `IP_LOST` que suelta la asociación sin IP | bench |
| FR-CON-A1 | Banco con el DHCP apagado y el AP arriba (`bench-dhcp-off`, CR-TB-24) | `con: … (wifi_down)` a 60 s ± 2 s; OPERATIVO ≤ 90 s tras volver el DHCP (clima:FR-CON-03) | Nodo mudo sin causa | bench⁺ |
| FR-CON-A2 | Broker bloqueado (DROP, `bench-broker-blackhole`, CR-TB-23) 25 min con el AP arriba | Una reasociación a los 10 min ± 30 s y otra a los 20 min ± 30 s; ningún reinicio; OPERATIVO ≤ 75 s tras desbloquear | Reasociar antes de 10 min; reinicio | bench⁺ |
| FR-CON-A2 | Lógica pura con relojes simulados | Reasociación a 600 s y como mucho una por 600 s | Más de una por 600 s | host |
| FR-HER-01 | Contratos heredados ejecutados con este firmware | Todos en verde con `agua-…` | Un contrato heredado sin ejecutar | bench |
| FR-HER-02 | Forzar cada estado de conectividad (sin config, Wi-Fi caído, broker caído, portal abierto, ESPERA, OTA en curso) y provocar fuga, seco y flotador | Plazos cumplidos en cada uno | Un plazo incumplido en algún estado | bench |

## 11. Política OTA — heredada

Se heredan **clima §7 y §12**: clima:FR-OTA-01…14 y NFR-OTA-01, con hosts permitidos,
anti-downgrade, validación de imagen y rollback. `ota_repo` pasa a ser el de este
repositorio.

- **FR-OTA-A1** [Must] [derived] (P11): una orden OTA con la válvula en CERRANDO o
  ABRIENDO, o con una sonda en `mojada`, deberá rechazarse con `rechazado:ocupado`.
- **FR-OTA-A2** [Must] [derived] D15: la imagen nueva no deberá marcarse como válida
  si no ha restaurado los bloqueos de FR-PER-02.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-OTA-A1 | Válvula en movimiento; sonda mojada; orden OTA | `ota`=`rechazado:ocupado` | Descarga iniciada | bench |
| FR-OTA-A2 | Imagen de prueba que no restaura los bloqueos | Rollback a la imagen anterior | Imagen marcada válida | bench⁺ |

---

# Parte B — Interfaces (L1)

## 12. Drivers de E/S

- **FR-SON-01** [Must] [user] D6 (P4): cada lazo deberá clasificarse en `seca`, `mojada`,
  `abierta`, `corto` o `indeterminada` según las bandas de P4.
- **FR-SON-02** [Must] [user] D6: un lazo activo en `abierta`, `corto` o `indeterminada`
  durante 5 s seguidos deberá publicar el evento `falla_sonda` y no deberá mover la válvula,
  salvo el cierre preventivo de FR-SON-05.
- **FR-SON-04** [Must] [user] H-93: con A3 del 0x4A fuera de 3,0–3,6 V o el 0x4A sin
  responder durante 5 s seguidos, el nodo deberá publicar el evento `falla_vref` (una vez
  por cambio a falla), escribir `son: vref nominal` y mostrar en el LED la prioridad 2
  (§8.2). La clasificación sigue con la referencia nominal de §2.2.
- **FR-SON-05** [Must] [user] H-93 (P22): con `g_ciego` (§5.2) el nodo deberá cerrar la
  válvula con `cerrada_por`=`sondas` (V24) en ≤ 500 ms, sin ninguna condición de red, y
  no deberá poner `bloqueo_fuga` en 1. Con `casa_cerrada`=false, las mismas fallas solo
  avisan (FR-SON-02, FR-SON-04).
- **FR-SON-03** [Must] [derived]: una sonda con `sonda_activa`=false no deberá
  confirmar fugas ni fallas y deberá publicarse como `inactiva`.
- **FR-IO-04** [Must] [user] D9 (P10): la corriente RMS de la bomba deberá calcularse
  sobre 10 ciclos de 60 Hz con ≥ 2000 muestras/s, con error ≤ 5 % del fondo de escala.
- **FR-IO-05** [Must] [derived]: los finales, el flotador y el botón deberán filtrarse
  con un antirrebote de 50 ms.
- **FR-IO-06** [Must] [derived]: un ADS1115 que no responda deberá marcar sus canales en
  `falla` (sondas → `falla_sonda`, nivel → `falla`) en ≤ 2 s y publicar `falla_adc`.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-SON-01 | Función pura con fracciones en cada banda y en los bordes | Clasificación según P4 | Otra clase | host |
| FR-SON-01 | El banco presenta 10 kΩ, 3 kΩ, abierto y corto en cada canal | `seca`, `mojada`, `abierta`, `corto` | Otra clase | bench |
| FR-SON-02 | Lazo 6 abierto 6 s | Un `falla_sonda` (sonda 6, `abierta`); válvula sin movimiento | Cierre | bench |
| FR-SON-03 | Sonda 8 inactiva y en corto | Estado `inactiva`; sin eventos | Evento o cierre | bench |
| FR-SON-04 | `casa_cerrada`=false; el banco baja A3 a 2,5 V durante 6 s; luego la devuelve a 3,3 V (con `casa_cerrada`=true, lo mismo y además el cierre de FR-SON-05) | Un `falla_vref` a 5 s ± 1 s; `son: vref nominal`; LED rojo 1 Hz; sin `falla_vref` repetidos; al volver, LED según el resto de condiciones | Ningún evento; eventos repetidos sin cambio; reinicio | bench |
| FR-SON-05 | Lógica pura: `casa_cerrada` × número de sondas activas en falla (0–8) × referencia (ok/falla) × estado de la válvula | Cierre solo con `casa_cerrada`=true y (≥ 2 en falla o referencia en falla); `bloqueo_fuga` sin cambio; V25 rechaza la reapertura | Cierre con 1 sonda en falla; cierre con `casa_cerrada`=false; reapertura con `g_ciego` | host |
| FR-SON-05 | ABIERTA, **broker apagado y Wi-Fi caído**; `casa_cerrada`=true; el banco abre los lazos 2 y 7 durante 6 s. Repetir con la referencia en falla. Repetir con `casa_cerrada`=false | Con true: IN1 en alto a 5 s + 500 ms del segundo lazo (o de la referencia); `cerrada_por`=`sondas`. Con false: solo `falla_sonda`/`falla_vref`, sin movimiento | Cierre con false; esperar a la red; `bloqueo_fuga`=1 | bench |
| FR-IO-04 | Banco: senoide de 60 Hz de 0,5 V RMS en GPIO4 | 15 A ± 1,5 A | Error mayor | bench |
| FR-IO-05 | Rebotes de 30 ms | Sin cambio de estado | Cambio | bench |
| FR-IO-06 | `casa_cerrada`=false; desconectar el ADS1115 0x49 en marcha. Repetir con `casa_cerrada`=true | Con false: `falla_adc`; sondas 5–8 en falla ≤ 2 s; sin pánico. Con true: lo mismo y, además, cierre por V24 (FR-SON-05) | Reinicio; con false, cierre | bench |

## 13. Publicador MQTT y órdenes

Se heredan clima:FR-MQTT-01…09 (cliente, credenciales, LWT, reconexión, `status` con
diagnóstico). Prefijo `<sitio>/<zona>/<nodo>/` como en el piloto.

### 13.1 Tópicos

| Tópico | Dir. | Payload (ejemplo) | QoS | Retain |
|---|---|---|---|---|
| `valvula/state` | → | `{"estado":"cerrada","bloqueo_fuga":true,"cerrada_por":"fuga"\|"orden"\|"bateria"\|"ejercicio"\|"sondas"\|"ninguno","t_cierre_ms":8200,"t_apertura_ms":8100,"ts":…}` | 1 | sí |
| `valvula/set` | ← | `{"accion":"cerrar"\|"abrir"\|"ejercicio"\|"posponer_ejercicio","motivo":"fuga_remota"\|"casa_cerrada"\|"llegada"\|"manual"}` | 1 | no |
| `bomba/state` | → | `{"permiso":"cortada","motivo":"seco","intentos_24h":1,"calibrada":true,"ts":…}` | 1 | sí |
| `bomba/set` | ← | `{"accion":"cortar"\|"permitir"\|"rearmar"}` | 1 | no |
| `fuga/<n>` (n = 1…8) | → | `{"estado":"seca"\|"mojada"\|"abierta"\|"corto"\|"indeterminada"\|"inactiva","ts":…}` | 1 | sí |
| `nivel` | → | `{"v":250,"u":"cm","pct":62.5,"valido":true,"ts":…}` | 0 | sí |
| `presion` | → | `{"v":3.2,"u":"bar","valido":true,"ts":…}` | 0 | sí |
| `corriente_bomba` | → | `{"v":4.1,"u":"A","ts":…}` | 0 | sí |
| `medidor` | → | `{"total_l":12345.67,"caudal_lh":180,"pulsos":3261,"galones_por_pulso":1.0,"ts":…}` (§6.7) | 0 | sí |
| `medidor/set` | ← | `{"accion":"permitir_consumo","min":90}` (FR-MED-06) | 1 | no |
| `flotador` | → | `{"estado":"agua"\|"sin_agua","ts":…}` | 1 | sí |
| `red` | → | `{"estado":"presente"\|"ausente","ts":…}` | 1 | sí |
| `bateria` | → | `{"v":12.9,"u":"V","estado":"ok"\|"baja"\|"critica","ts":…}` | 1 | sí |
| `evento` | → | `{"tipo":"fuga"\|"falla_valvula"\|"movimiento_manual"\|"falla_sonda"\|"falla_adc"\|"bomba_seco"\|"bomba_bloqueada"\|"ciclado_anormal"\|"bateria_baja"\|"bateria_critica"\|"orden_rechazada"\|"flotador_falla"\|"falla_vref"\|"casa_cerrada"\|"consumo_casa_cerrada"\|"medidor_falla",…,"ts":…,"edad_s":…}` (`edad_s` solo en los eventos que salen de la cola, FR-PUB-04; `consumo_casa_cerrada` lleva además `"motivo":"hora"\|"diario"`, `"l"` y `"ventana_min"`, §6.7) | 1 | no |
| `config/set`, `config/state` | ←, → | §16 | 1 | no, sí |
| `status`, `cmd/ota` | →, ← | heredados (clima §9.3, §12) | 1 | sí, no |

`ts` como en el piloto (época Unix, o ausente si la hora no es válida).

### 13.2 Requisitos

- **FR-PUB-01** [Must] [user] (P8): cada cambio de estado de válvula, bomba, sondas,
  flotador, red y batería deberá publicarse en ≤ 1 s mientras el nodo esté en OPERATIVO.
- **FR-PUB-02** [Must] [derived]: al entrar en OPERATIVO, el nodo deberá publicar todos
  los tópicos con retain de §13.1 con el estado actual.
- **FR-PUB-03** [Must] [user] (P8): las medidas deberán publicarse cada 60 s.
- **FR-PUB-04** [Must] [user] H-93 (P23): un evento crítico (`fuga`, `falla_valvula`,
  `movimiento_manual`, `falla_sonda`, `falla_adc`, `falla_vref`, `bomba_seco`,
  `bomba_bloqueada`, `ciclado_anormal`, `bateria_baja`, `bateria_critica`,
  `flotador_falla`, `consumo_casa_cerrada`) generado fuera de OPERATIVO deberá guardarse en una cola en RAM y
  publicarse al entrar en OPERATIVO, después de los tópicos de FR-PUB-02, en el orden en
  que ocurrió, con `edad_s` (segundos del reloj monótono desde que ocurrió) y con `ts` solo
  si la hora era válida al ocurrir. La cola no sobrevive a un reinicio: tras un reinicio,
  lo que se pierde lo reconstruyen los estados con retain y FR-PER-03.
- **FR-PUB-05** [Must] [derived] H-93 (P23): con la cola llena, el nodo deberá descartar el
  evento más antiguo que no sea `fuga` (si todos son `fuga`, el `fuga` más antiguo) y
  publicar el número de
  descartados en `status` como `eventos_perdidos`.
- **FR-ORD-01** [Must] [user] regla de confirmación: cada orden de válvula o bomba deberá
  responderse con su `…/state` final o con un evento `orden_rechazada` que incluya el
  motivo (`fuga_activa`, `sin_agua`, `nivel_bajo`, `ocupado`, `payload`, `accion`,
  `sondas_en_falla` (V25, H-93)).
- **FR-ORD-02** [Must] [derived] (P15): un payload > 256 bytes, JSON no válido, una
  acción desconocida o un campo de tipo incorrecto deberán rechazarse con
  `orden_rechazada` (`payload` o `accion`), sin efecto y sin reinicio.
- **FR-ORD-03** [Must] [derived]: los mensajes con retain recibidos en `valvula/set` y
  `bomba/set` deberán ignorarse.
- **FR-ORD-04** [Must] [user] D4: la orden `cerrar` con `motivo`=`fuga_remota` deberá
  ejecutar V05 con `cerrada_por`=`orden` y no deberá poner `bloqueo_fuga` en 1.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-PUB-01 | Suscriptor a `#`; cambiar cada entrada en el banco | Cada `…/state` ≤ 1 s | Retraso o ausencia | bench |
| FR-PUB-02 | Reiniciar el broker | Todos los tópicos retain reaparecen tras reconectar | Faltan tópicos | bench |
| FR-PUB-03 | 5 min en OPERATIVO | 5 ± 1 publicaciones de cada medida | Otro número | bench |
| FR-PUB-04 | Suscriptor a `#`; broker apagado; el banco moja la sonda 3 y abre el lazo 6 (6 s); a los 3 min, broker encendido | `evento` `fuga` y luego `falla_sonda`, en ese orden, después de los tópicos con retain y en ≤ 75 s desde que vuelve el broker (clima:FR-CON-04); `edad_s` ≈ el tiempo transcurrido ± 5 s | Un evento ausente, duplicado o fuera de orden | bench |
| FR-PUB-04 | Lógica pura de la cola: llenado, orden, `edad_s` | Orden de llegada; `edad_s` correcta | Pérdida con la cola sin llenar | host |
| FR-PUB-05 | Lógica pura: 40 eventos con 2 `fuga` entre ellos y la cola de 32 | Los 2 `fuga` se conservan; `eventos_perdidos` = 8 | Un `fuga` descartado; contador distinto | host |
| FR-ORD-01 | Cada orden en cada estado relevante | Estado final o rechazo con motivo | Silencio | bench |
| FR-ORD-02 | 300 bytes, `{`, `{"accion":"x"}`, `{"accion":5}` | `orden_rechazada`; sin movimiento; `uptime` sigue creciendo | Reinicio; movimiento | bench |
| FR-ORD-03 | Publicar `abrir` con retain y reconectar | Sin efecto | Apertura | bench |
| FR-ORD-04 | Orden `fuga_remota` | CERRADA, `bloqueo_fuga`=false | `bloqueo_fuga`=true | bench |

## 14. Discovery de Home Assistant

Se heredan clima:FR-HA-01…03 (esquema y tópicos de Discovery). Las entidades de este nodo son:
`valve` (válvula: estados `open`/`closed`/`opening`/`closing`), `switch` (permiso de
la bomba), `binary_sensor` × 8 (fugas, `device_class: moisture`), `binary_sensor`
(flotador, red), `sensor` (nivel cm y %, presión, corriente, Vbat, y el estado de la
batería `ok`/`baja`/`critica` de P5 con `device_class: enum`, ambos del tópico `bateria`),
`sensor` del medidor testigo (§6.7: total en L con `device_class: water` y
`state_class: total_increasing`, para el panel de energía de HA; caudal en L/h con
`device_class: volume_flow_rate`), `button` (rearmar bomba, ejercicio) y `event` (§13.1; los avisos `bateria_baja` y
`bateria_critica` de FR-ENE-03). No se anuncia un % de batería: con LiFePO4 la tensión
no da un % fiable [user, 2026-11-15].

- **FR-HA-A1** [Must] [derived]: cada entidad de la lista deberá anunciarse con un
  `unique_id` que incluya el identificador de §18.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-HA-A1 | HA del banco; arranque del nodo | Todas las entidades listadas aparecen, sin duplicados tras 3 reinicios | Entidad ausente o duplicada | bench |

## 15. Portal cautivo — heredado

Se hereda **clima §11** completo: clima:FR-POR-01…10, incluido FR-POR-08 (10 min y ESPERA,
H-82). Los campos son los mismos (§19). Los parámetros de §16 no aparecen en el portal.

## 16. Configuración por MQTT (D17)

- **FR-CFG-A1** [Must] [user] D17: un mensaje en `config/set` con
  `{"param":"<nombre>","v":<valor>}` deberá aceptarse solo si el nombre está en §19 con
  interfaz `mqtt` y el valor está dentro de su rango; si se acepta, se guarda en NVS y se
  publica `config/state` completo en ≤ 1 s.
- **FR-CFG-A2** [Must] [user] D17: un valor fuera de rango o un parámetro desconocido
  deberá rechazarse con `orden_rechazada` sin modificar la NVS.
- **FR-CFG-A3** [Must] [user] D17: `sonda_activa[n]`=false deberá rechazarse con
  `fuga_activa` mientras la sonda n esté en `mojada`.
- **FR-CFG-A4** [Must] [derived] D17: ningún parámetro del portal (§19, interfaz
  `portal`) deberá poder cambiarse por `config/set`.
- **FR-CFG-A5** [Must] [user] (condición del riesgo de P22, 2026-11-21): cada `config/set`
  aceptado que **cambie** el valor de `casa_cerrada` deberá publicar en ≤ 1 s el evento
  `{"tipo":"casa_cerrada","v":<nuevo>,"antes":<anterior>,…}`, además de `config/state`. Un
  `config/set` con el mismo valor no es un cambio y no publica el evento. Las escenas de HA
  cambian el parámetro por `config/set`, así que el evento sale igual venga de donde venga.
  Fuera de OPERATIVO no hay `config/set`, así que el evento no pasa por la cola de FR-PUB-04.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-CFG-A1 | `t_seco_s`=15 | `config/state` con 15; persiste tras reinicio | Sin persistir | bench |
| FR-CFG-A2 | `t_seco_s`=0, =9999; `xyz`=1 | Rechazo; NVS sin cambios | Valor aplicado | host + bench |
| FR-CFG-A3 | Sonda 3 mojada; desactivarla | Rechazo `fuga_activa` | Desactivada | bench |
| FR-CFG-A4 | `mqtt_host`, `wifi_ssid` por `config/set` | Rechazo | Cambio | bench |
| FR-CFG-A5 | Suscriptor a `#`; `casa_cerrada`: true → false, false → false, false → true por `config/set` | Un evento `casa_cerrada` en ≤ 1 s en el 1.º y el 3.º (`v` y `antes` correctos), ninguno en el 2.º; `config/state` en los tres | Cambio sin evento; evento sin cambio | bench |

---

# Parte C — Fundación (L0)

## 17. Wi-Fi, SNTP, MQTT, HTTPS OTA, NVS, I²C, ADC y watchdog — heredados

Se heredan clima §13–§15: clima:FR-TIME-01…06, NFR-WDT-01 y la configuración de Wi-Fi
WPA2 con PMF (clima:FR-CON-07).

- **NFR-WDT-A1** [Must] [derived]: la tarea de protección deberá estar suscrita al TWDT;
  un bloqueo de esa tarea deberá provocar un reinicio, del que se vuelve según FR-PER-02.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| NFR-WDT-A1 | Imagen de banco con comando que bloquea la tarea de protección | Reinicio con `reset=task_wdt`; bloqueos restaurados | Nodo colgado sin reinicio | bench |

---

# Parte D — Aspectos transversales

## 18. Identidad del nodo

- **FR-ID-01** [Must] [derived]: el identificador deberá ser **`agua-`** seguido de los
  3 últimos bytes de la MAC STA en hexadecimal en minúsculas, estable entre reinicios y OTA.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-ID-01 | Función con `24:0A:C4:A1:B2:C3` | `agua-a1b2c3` | Mayúsculas u otro prefijo | host |
| FR-ID-01 | 3 reinicios | Mismo id en `boot:` | Cambio | bench |

## 19. Catálogo de configuración

Heredados del portal (clima §17, sin cambios): `wifi_ssid`, `wifi_pass`, `mqtt_host`,
`mqtt_port`, `mqtt_user`, `mqtt_pass`, `zona`, `sitio`, más las constantes de build.
Se heredan clima:FR-CFG-01 y clima:C-CFG-01.

Parámetros de campo (interfaz `mqtt`, NVS, efecto inmediato; valores de §4.4):

| Nombre | Tipo | Por defecto | Rango | Ref. |
|---|---|---|---|---|
| `sonda_activa[1..8]` | bool | true | — (FR-CFG-A3) | FR-SON-03 |
| `t_carrera_max_s` | entero | 30 | 5–120 | P3 |
| `ejercicio_dias` | entero | 30 | 7–90 | D12 |
| `ejercicio_hora` | entero (hora local: UTC−4 sin horario de verano, TZ POSIX `AST4`) [user, 2026-11-15] | 3 | 0–23 | D12 |
| `casa_cerrada` | bool | true (P22) | — | FR-SON-05, H-93 |
| `zumbador_activo` | bool | true | — | D16 |
| `zumbador_min` | entero | 5 | 1–30 | D16 |
| `rango_transductor_cm` | entero | 500 | 50–1000 | FR-NIV-01 |
| `altura_util_cm` | entero | 200 | 50–1000 | FR-NIV-01 |
| `nivel_critico_pct` | entero | 10 | 5–30 | P6 |
| `nivel_rearme_pct` | entero | 30 | `nivel_critico_pct`+10 – 90 | P6 |
| `t_rearme_min` | entero | 10 | 2–120 | P6 |
| `intentos_max` | entero | 3 | 1–5 | P6 |
| `i_marcha_a` | decimal | 0,5 | 0,2–10 | P6 |
| `i_seco_a` | decimal | sin configurar | 0,2–20 y > `i_marcha_a` | P6 |
| `p_min_bar` | decimal | 0,5 | 0,2–5 | P6 |
| `t_seco_s` | entero | 10 | 3–60 | P6 |
| `ciclado_max` | entero | 6 | 3–30 | P7 |
| `ciclado_ventana_min` | entero | 60 | 10–240 | P7 |
| `galones_por_pulso` | decimal | 1,0 (a confirmar con el manual) | 0,1–100 | P29 |
| `consumo_alerta_l` | entero | 20 | 5–1000 | P32 |
| `consumo_ventana_min` | entero | 60 | 10–1440 | P32 |
| `consumo_margen_min` | entero | 120 | 0–720 | P32 |
| `consumo_diario_l` | entero | 50 | 10–2000 | P34 |

Ningún rango permite apagar el cierre por fuga ni el corte por seco o por flotador (D17).
`casa_cerrada`=false apaga solo el cierre preventivo de FR-SON-05, nunca el cierre por
fuga; es la decisión del propietario para la casa ocupada (H-93).
`i_seco_a` sin configurar no apaga el corte por flotador ni por nivel crítico.

## 20. Observabilidad

Se hereda clima §18 (líneas E0–E3, log UDP solo en la imagen de banco, C-LOG-01/02,
FR-LOG-01…03). Excepción a la regla de la 0.9.9: clima:E13 (comandos serie de la imagen
de banco) se hereda en la redacción de la **0.9.10** del piloto [user, 2026-11-15]: el nodo
lee los comandos por la consola primaria (UART0) y también por el USB-Serial/JTAG, porque
el banco solo escribe por este último. La imagen de producción no cambia (C-LOG-02). Líneas nuevas y normativas [derived]:

| Ref. | Línea | Cuándo |
|---|---|---|
| A1 | `val: <ESTADO> -> <ESTADO> (<evento>)` | Cada fila de §5.3 |
| A2 | `bom: <ESTADO> -> <ESTADO> (<evento>)` | Cada fila de §6.2 |
| A3 | `son: s<n> <clase>` | Cada cambio de clase de un lazo |
| A4 | `ana: niv=<cm\|falla> p=<bar\|falla> i=<A> vbat=<V> red=<0\|1>` | Cada ciclo de medidas |
| A5 | `nvs: bloqueos escritos (<n>)` | Cada escritura de FR-PER-01 |
| A6 | `ord: <tópico> <accion> -> <ok\|rechazado:<motivo>>` | Cada orden recibida |
| A7 | `i2c: scan sda=8 scl=9 hz=100000 ack=<lista>` | Al arrancar |
| A8 | `med: pulsos=<n> total_l=<L> caudal_lh=<L/h> nvs=<0\|1>` | Cada ciclo de medidas (H-96) |

- **FR-OBS-A1** [Must] [derived]: el nodo deberá escribir las líneas A1–A7 en los
  momentos indicados, y A8 (H-96).

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-OBS-A1 | Ejecutar los contratos de §5, §6, §9 y §13 | Cada línea con el formato exacto | Formato distinto o ausente | bench |

## 21. Seguridad

### 21.1 Perfil de amenazas

Se hereda el perfil «LAN doméstica, repositorio público» de clima §19.1, con estos cambios:

| Elemento | Supuesto |
|---|---|
| Nodo crítico | Sí (D14): S1–S4 |
| Amenaza adicional (b′) | Alguien en la LAN con las credenciales del broker manda **abrir la válvula durante una fuga**, **permitir la bomba sin agua**, o configura umbrales para anular las protecciones |
| Integridad del firmware | Heredada en desarrollo. En la placa de producción: firma + Secure Boot v2 + cifrado de flash, activados al instalar (P14, FR-SEC-A4) |

**Riesgos aceptados.** (b′) puede **cerrar** la válvula o **cortar** la bomba (deja la
casa sin agua, un fallo seguro) y puede leer los estados. Con `config/set` también puede
poner `casa_cerrada`=false y apagar el cierre preventivo de FR-SON-05 (no el cierre por
fuga). Riesgo aceptado con P22 (H-95) **con una condición** del propietario
(2026-11-21): cada cambio de `casa_cerrada`, venga de una escena de HA o de cualquier otro
cliente MQTT, genera el evento `casa_cerrada` (FR-CFG-A5) y un aviso de HA al propietario
(FR-VIG-02), para que un cambio que no hizo él se note. No puede abrir con una sonda
mojada, permitir la bomba sin agua ni apagar una protección (FR-SEC-A1).

### 21.2 Requisitos

Se heredan NFR-SEC-01, NFR-SEC-02 y FR-SEC-03 de clima §19.2, con este repositorio.

- **FR-SEC-A1** [Must] [user] D14, D17: ninguna secuencia de mensajes MQTT, de cualquier
  longitud y orden, deberá provocar: una apertura con una sonda activa mojada; el permiso
  PERMITIDA con el flotador sin agua; un movimiento con IN1 = IN2 = 1; o un valor de
  configuración fuera de §19.
- **FR-SEC-A2** [Must] [derived] (auditoría, punto 1): el nodo solo deberá abrir
  conexiones hacia el broker, el servidor SNTP y los hosts OTA permitidos.
- **C-SEC-A3** [Must] [user] P14: ninguna imagen de banco ni de desarrollo debe activar
  Secure Boot ni el cifrado de flash, y ningún procedimiento del banco debe quemar eFuses
  de seguridad. Los eFuses son irreversibles y las placas del banco deben seguir siendo
  regrabables.
- **FR-SEC-A4** [Must] [user] P14, D14: la placa final de producción deberá quedar, al
  instalarse (puertas S2/S4), con Secure Boot v2, imágenes firmadas con la clave del
  propietario (fuera del repositorio) y cifrado de flash en modo release.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| FR-SEC-A1 | Fuzzing de `valvula/set`, `bomba/set` y `config/set` (≥ 10 000 mensajes aleatorios y mutados) con sonda 1 mojada y flotador sin agua | Válvula CERRADA, permiso cortado y config dentro de rango durante toda la prueba; sin reinicios | Cualquier violación o reinicio | bench + host (lógica de guardas) |
| C-SEC-A3 | CI: `sdkconfig` de cada imagen de banco y de desarrollo; banco: `espefuse summary` de cada placa del banco antes y después de cada sesión | `CONFIG_SECURE_BOOT` y `CONFIG_FLASH_ENCRYPTION_ENABLED` sin activar; eFuses de Secure Boot y de cifrado en 0 | Cualquiera activado; un eFuse de seguridad quemado en una placa del banco | other (CI) + bench |
| FR-SEC-A4 | Puerta S2: `espefuse summary` de la placa de producción tras el procedimiento de instalación; intentar grabar por USB una imagen sin firmar | Secure Boot v2 y cifrado de flash (modo release) activos; la imagen sin firmar no arranca | Placa de producción sin protección; arranque de una imagen sin firmar | field (S2) |
| FR-SEC-A2 | Captura de tráfico del nodo durante 1 h con OTA (capacidad de captura del banco) | Solo los destinos listados | Cualquier otro destino | bench⁺ |

## 22. Build

Heredado de clima §20 (ESP-IDF 6.x, CI en GitHub Actions, imagen de banco con
`CONFIG_NODO_BENCH_BUILD`). Hay dos variantes de build (P14): **desarrollo/banco** (sin
Secure Boot ni cifrado, C-SEC-A3) y **producción** (firmada, para FR-SEC-A4). La cadena de
firma, la custodia de la clave y el procedimiento de quemado se definen en una enmienda
antes de la Fase 3 (puerta S3).

---

# Parte E — Operación y verificación

## 23. Procedimientos operativos

Se documentan en `docs/UserDocumentation/Manual-de-usuario.md`: aprovisionamiento
(heredado), calibración de la bomba en la puesta en marcha (`i_marcha_a`, `i_seco_a`,
`p_min_bar`), qué hacer ante una fuga, reapertura en persona, rearme de la bomba y
cambio de la batería.

## 24. Verificación y validación

### 24.0 Arquitectura de pruebas

| Capa | Nivel principal | Notas |
|---|---|---|
| L2 (§5–§9) | host (tablas de transición, umbrales, guardas, ventanas de tiempo) + bench | La lógica de protección debe ser pura y testeable en host |
| L1 (§12–§16) | bench | Necesita las capacidades de simulación de §3.1 |
| L0 (§17) | target + bench (heredado) | — |
| Instalación | field | Calibración, NC de la bomba, finales reales, §25; nunca se inyectan fugas reales sin supervisión |

La matriz y el plan de pruebas los genera `/build` desde los contratos.

### 24.1 Pruebas de aceptación (journeys)

1. **Fuga con la casa cerrada y sin red:** Wi-Fi apagado → sonda mojada → la válvula cierra ≤ 1,5 s + carrera; al volver la red, HA recibe `fuga` y `valvula/state`.
2. **Falsa alarma resuelta a distancia:** sonda seca de nuevo → `abrir` remoto → abre.
3. **Cisterna vacía por falta de agua en la calle:** seco → corte → la cisterna se llena → rearme automático; al tercer seco en 24 h → bloqueo.
4. **Apagón largo:** red ausente → batería baja → crítica → cierre preventivo → vuelve la red → reapertura.
5. **Ataque en la LAN:** con una fuga activa, órdenes de abrir y configuraciones hostiles → sin efecto (FR-SEC-A1).

En el journey 1, el `fuga` que recibe HA al volver la red sale de la cola de FR-PUB-04 (H-93).

## 25. Instalación y vigilancia externa (H-93)

Requisitos que no cumple el firmware del nodo sino la instalación y el sistema de la
casa. Se verifican en la puesta en marcha (§3.5) y entran en la auditoría S2.

- **C-INS-01** [Must] [user] H-93: el módem, el router principal del mesh, el satélite que
  da cobertura al nodo, el switch y la Pi del broker deben alimentarse desde un circuito
  respaldado (inversor o UPS), igual que el nodo (C-HW-20).
- **C-INS-02** [Must] [user] H-93: en producción, `mqtt_host` debe ser una dirección IPv4
  (no un nombre) y el router debe tener reservas DHCP para el nodo y para la Pi del broker.
- **FR-VIG-01** [Must] [user] H-93 (A-16 del propietario): si durante **15 min** no hay
  constancia, fuera de la casa, de que el nodo publica su `status` en vivo, el propietario
  deberá recibir un aviso en el teléfono por un canal que no dependa de la Pi ni de la
  conexión a internet de la casa (por ejemplo, un servicio externo que espera un latido y
  avisa cuando falta). La caída de la Pi, del switch, del router o de internet deberá
  producir el mismo aviso.
- **FR-VIG-02** [Must] [user] (condición del riesgo de P22, 2026-11-21): cada evento
  `casa_cerrada` del nodo (FR-CFG-A5) deberá producir un aviso de HA al propietario con el
  valor nuevo y la hora, también cuando el cambio lo hizo una escena.

| ID | Precondición · estímulo | Observación esperada | No debe ocurrir | Nivel |
|---|---|---|---|---|
| C-INS-01 | Puesta en marcha: quitar la luz general | `valvula/state` y `status` siguen llegando a HA durante 30 min de apagón | HA sin datos del nodo durante el apagón | field |
| C-INS-02 | Revisión de la configuración de producción (S2) y del router; luego apagar el DNS del router 10 min | `mqtt_host` es IPv4; reservas presentes; el nodo sigue en OPERATIVO | Un nombre en `mqtt_host`; salida de OPERATIVO | field |
| FR-VIG-02 | HA de la casa; cambiar `casa_cerrada` con la escena «Casa cerrada», con «Preparar llegada» y con un `config/set` manual desde otro cliente | Un aviso en el teléfono por cada cambio, con el valor nuevo | Un cambio sin aviso | field |
| FR-VIG-01 | Cuatro cortes, uno cada vez: switch de la Pi, la Pi, el internet de la casa (desconectar el módem) y el nodo | En cada uno, aviso en el teléfono en ≤ 15 min más el periodo de comprobación del canal (que queda anotado en la instalación) | Ningún aviso; aviso que llega por la Pi | field |

---

## Apéndice A — Constantes

Ver §4.4 (D1–D18, P1–P34, H-93, H-95, H-96) y §19. Los valores numéricos normativos salen de
esas dos tablas, de las restricciones de §2.2 (por ejemplo la referencia de los lazos,
3,0–3,6 V y 5 s) o del propio requisito cuando el propietario lo fijó en él (por ejemplo
FR-BOM-10).

## Relacionado

- FSD del piloto, versión 0.9.9 (`villa-nodo-clima-piloto`, `docs/Functionality/FSD-nodo-clima.md`)
- `docs/Method/` (CÓMO) · `docs/UserDocumentation/Manual-de-usuario.md` (OPERAR)
