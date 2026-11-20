# Análisis de cortes parciales frente al FSD (2026-11-20)

Análisis del FSD 0.6.0 frente a cortes parciales (no solo totales) de internet, de la red,
de la energía y combinados. Decisión del propietario: **H-93** (2026-11-20), aplicada en el
FSD 0.7.0. Los huecos se numeran P1–P11 **solo en este documento**; no son los valores Pnn
del FSD (§4.4).

La base: la protección es local y no depende de la red (D18, FR-HER-02). El nodo solo habla
con el broker, SNTP y los hosts OTA (FR-SEC-A2); nunca con WhatsApp ni la nube (§1.4).

## Casos

| Caso | Detecta el nodo | Sigue funcionando | Alerta (vía) | Recuperación | Hueco |
|---|---|---|---|---|---|
| LAN arriba sin internet; Deco sin internet; luz sin internet | Nada directo; sin SNTP, si reinicia queda sin hora válida | Todo: protección, MQTT local | HA sí; WhatsApp no | SNTP reintenta (clima:FR-TIME-06) | P2, P9 |
| DNS falla | Solo si `mqtt_host` es un nombre | Protección | Ninguna | Backoff (clima:FR-CON-02) | P7 |
| Pérdida de paquetes / internet intermitente | Cae MQTT (T15) | Protección | Los eventos del hueco se perdían | ≤ 75 s y retain (FR-PUB-02) | P1 |
| WhatsApp o nube caídos | No | Todo | HA recibe, no entrega | De HA | P2 |
| Satélite apagado | `wifi_down` | Protección | Ninguna hasta volver | ≤ 90 s tras volver el AP; no busca otro AP | P8 |
| Switch apagado con la Pi encendida; internet sin Pi | Broker caído | Protección | Ninguna fuera de la casa | Backoff | P2, P6 |
| DHCP caído | Se queda asociado sin IP, sin plazo | Protección | Ninguna | No definida | P7, H1 del piloto |
| Broker inalcanzable con el Wi-Fi arriba (satélite con el enlace roto) | Broker caído | Protección | Ninguna | Nunca reasocia | H2 del piloto |
| El nodo salta a otra red | Solo WPA2+PMF con su SSID (clima:FR-CON-07) | Protección | — | — | Bajo |
| Bajo voltaje o caídas breves | `reset=brownout` | UPS de 12 V (C-HW-20); bloqueos restaurados (FR-PER-02) | `status` al reconectar | Reinicio | P4 |
| Pérdida de una fase; UPS parcial | `red` mide la salida del inversor | Protección; cierre por batería (FR-ENE-04) | Solo si la cadena hasta la Pi tiene energía | — | P5, P6 |
| Sensores sin energía | `falla_sonda`, `falla_adc`, `falla`; referencia caída solo en el log | Las sondas en falla no detectan fugas | MQTT; LED prioridad 2 | Al volver la tensión | P3, P10 |
| La luz vuelve en otro orden | Espera con backoff | Protección | Al conectar | ≤ 60 s de backoff | Bajo |

## Huecos y decisión H-93

| N.º | Hueco | Riesgo | Decisión H-93 | En el FSD 0.7.0 |
|---|---|---|---|---|
| P1 | Eventos sin retain ni cola: con el broker caído se pierden | Alto | Aprobado | FR-PUB-04/05 (P23 propuesto) |
| P2 | Sin Pi, switch, internet o WhatsApp no sale ninguna alerta | Alto | Aprobado, T = 15 min, canal que no depende de la Pi | FR-VIG-01 (§25) |
| P3 | Referencia de los lazos caída: sondas ciegas, solo un log | Alto | Aprobado: `falla_vref` + LED; casa cerrada → cierre con N = 2 sondas en falla o falla de la referencia; casa ocupada → solo aviso (cambia D6) | FR-SON-04/05, V24/V25 (P22 propuesto) |
| P4 | Brownout al arrancar el motor y bucle de reinicios | Medio | Próxima ronda; k = 3 reinicios en 10 min | P26 |
| P5 | `red` no es la red pública | Medio | Próxima ronda | — |
| P6 | Sin UPS para la cadena de red | Alto | Aprobado (instalación) | C-INS-01 |
| P7 | Broker por nombre y DHCP sin reserva | Medio | Aprobado (instalación) | C-INS-02 |
| P8 | No elige el AP más fuerte ni pide RSSI mínimo | Medio | Próxima ronda; RSSI mínimo −75 dBm | P27 |
| P9 | Sin hora válida no hay ejercicio y no avisa | Medio | Próxima ronda | — |
| P10 | Seco con el sensor de presión en falla no definido | Medio | Próxima ronda | — |
| P11 | Bomba con baja tensión y corriente alta | Bajo | No se hace: lo cubre el térmico | — |
| H1 (piloto) | `IP_LOST` sin atender; asociado sin IP sin plazo | Alto | Heredado para considerar | FR-CON-A1 (P24 propuesto) |
| H2 (piloto) | Broker inalcanzable con el Wi-Fi arriba: nunca reasocia | Medio | Heredado para considerar | FR-CON-A2 (P25 propuesto) |

H1 y H2 vienen de `testing/analisis-cortes-parciales.md` del repositorio del piloto
(commit 77ae38b).
