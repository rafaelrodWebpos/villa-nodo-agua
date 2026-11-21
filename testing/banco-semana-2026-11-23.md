# Pruebas de banco de la semana del 2026-11-23 — rama `integracion-6.0.3-h95`

Lo que se valida es la rama de integración: ESP-IDF 6.0.3 más H-95 (FSD 0.7.2). Las imágenes
de la CI son `firmware-v0.0.0-ab3ddb2` y `firmware-v0.0.0-ab3ddb2-bench` (corridas
44897606678 y 44897608786), o las de un commit posterior de la rama. Siempre se usa el slot
del DUT (SLOT2) y nunca otro. Antes de empezar: el banco libre (`scripts/bench/tc_wdt_01_24h.sh
--check` da las condiciones), el candado `banco.lock` y las pruebas de host en verde.

Las duraciones son **aproximadas**: incluyen grabar y aprovisionar en cada prueba atómica y
salen de las corridas del 2026-11-15/19 en main. Hay que medirlas de nuevo.

## Orden

| # | Bloque | Pruebas | Duración aprox. | Por qué en este orden |
|---|---|---|---|---|
| 0 | Host (sin banco) | ctest 52/52, `test_grabado`, `test_estabilidad`, chequeos de la CI | 2 min | Si alguna sale en rojo, no se toca el banco |
| 1 | Journey: la puerta | JRN-01.0 → JRN-01.3 (grabar con la defensa, arranque con «ESP-IDF v6.0.3», portal, aprovisionamiento, broker y status) | 15 min | Mientras un paso del journey esté en rojo, lo demás no se puede leer |
| 2 | Watchdog y reinicio | TC-WDT-02, TC-TIME-08 | 10 min | El watchdog y la restauración tras un reinicio, con la imagen nueva |
| 3 | Conectividad heredada | TC-CON-03, TC-CON-06, TC-CON-07, TC-CON-08, TC-CON-09, TC-CON-10 | 35 min | Regresión de la tabla de §6 con el adaptador nuevo de FR-CON-A1/A2 (STA_CONNECTED, LOST_IP y la consulta cada segundo) |
| 4 | **Nueva: FR-CON-A2** | **TC-CON-A5** (broker parado 25 min con el AP arriba: reasociaciones a 10 y 20 min, sin reinicio) | 35 min | La única prueba de H-95 que el banco ya puede correr. **Falta escribir su ejecutable** (≈ 30 min, sin banco) |
| 5 | Portal y seguridad | TC-POR-02, 05, 07, 09, 11, 12; TC-CON-14 | 30 min | Regresión del portal con 6.0.3 |
| 6 | MQTT, log y HA | TC-MQTT-05, TC-LOG-04, TC-LOG-07, TC-LOG-08, TC-HA-A1 | 25 min | Regresión de MQTT, del log UDP y de Discovery |
| 7 | Hora | TC-TIME-06; bench⁺: TC-TIME-01, TC-TIME-04 | 20 min | SNTP con 6.0.3; las dos de bench⁺ necesitan Internet del AP |
| 8 | Estabilidad | **TC-WDT-01, 24 h**, con la imagen de integración | 24 h + 5 min | Lo último, en una ventana larga (fin de semana): la 24 h de esta tarde corre sobre main (3e3ffe7) y no cubre el firmware nuevo |

**Total sin la 24 h: ≈ 3 h 10 min**, que pueden repartirse en dos sesiones: bloques 0–4 y
bloques 5–7.

## Declaradas pero bloqueadas (no se corren esta semana)

| Prueba | Requisito | Le falta | Salvedad |
|---|---|---|---|
| TC-SON-A4 | FR-SON-04 | `bench-vref-lazos` (número sin asignar), `bench-salidas-dut` (CR-TB-22), `nodo-io-montaje` | La compra del banco llega a fin de mes |
| TC-SON-A5 | FR-SON-05, V24, V25 | `bench-lazos-fuga` (CR-TB-16), `bench-vref-lazos`, `bench-salidas-dut`, `bench-finales-valvula` (CR-TB-17), válvula real | Ídem |
| TC-IO-A6 | FR-IO-06 con las dos casas | `bench-ads-desconexion` (con el montaje puede hacerse con un aviso al operador), `bench-salidas-dut` | Ídem |
| TC-CON-A3 | FR-CON-A1 | `bench-dhcp-off` (CR-TB-24) | Próxima ronda del fork. Hasta entonces, **bloqueada, no fallida** |
| TC-CON-A4 | FR-CON-A2 (DROP) | `bench-broker-blackhole` (CR-TB-23) | Ídem. TC-CON-A5 cubre la cuenta de 10 min, pero no el DROP |

Mientras tanto, la lógica de FR-SON-04/05, FR-IO-06 y FR-CON-A1/A2 está cubierta en host:
TC-PROT-A1 (ciego, falla_vref, ads), TC-VAL-A1 (V24/V25) y TC-CON-A1.

## Lo que hay que decidir

- Un número CR-TB para `bench-vref-lazos` (llevar A3 fuera de 3,0–3,6 V), o ampliar CR-TB-16 para que lo cubra.
