# Nivel host: núcleos puros compilados con el compilador nativo, sin ESP-IDF
# (docs/Method/standards/testing.md). /build añade un test por requisito.
cmake_minimum_required(VERSION 3.22)
project(nodo_agua_host_tests C)
enable_testing()
set(COMP ${CMAKE_CURRENT_SOURCE_DIR}/../../components)
add_compile_options(-Wall -Wextra -Werror)

# Comprueba que el nivel host funciona; no verifica ningún requisito del FSD.
add_executable(tier_smoke tier_smoke.c)
add_test(NAME tier_smoke COMMAND tier_smoke)

# TC-ID-01 · FR-ID-01
add_executable(test_identidad test_identidad.c ${COMP}/identidad/identidad.c)
target_include_directories(test_identidad PRIVATE ${COMP}/identidad/include)
add_test(NAME TC-ID-01 COMMAND test_identidad)

# TC-CON-01 · T01, T02 (filas de la tabla §6.3 implementadas)
add_executable(test_conectividad test_conectividad.c ${COMP}/conectividad/conectividad.c)
target_include_directories(test_conectividad PRIVATE ${COMP}/conectividad/include)
add_test(NAME TC-CON-01 COMMAND test_conectividad)

# TC-CON-13 · FR-CON-07 (seguridad mínima del STA, SEC-07)
add_executable(test_conectividad_sta test_conectividad_sta.c ${COMP}/conectividad/conectividad_sta.c)
target_include_directories(test_conectividad_sta PRIVATE ${COMP}/conectividad/include)
add_test(NAME TC-CON-13 COMMAND test_conectividad_sta)

# TC-POR-01 · FR-POR-03 (generador del formulario)
add_executable(test_portal_form test_portal_form.c ${COMP}/portal/portal_form.c)
target_include_directories(test_portal_form PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-01 COMMAND test_portal_form)

# TC-POR-13 · FR-POR-09 (POST incompleto → 408, SEC-05)
add_executable(test_portal_lectura test_portal_lectura.c ${COMP}/portal/portal_lectura.c)
target_include_directories(test_portal_lectura PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-13 COMMAND test_portal_lectura)

# TC-POR-14 · FR-POR-10 (portal solo por la IP del AP, SEC-06)
add_executable(test_portal_origen test_portal_origen.c ${COMP}/portal/portal_origen.c)
target_include_directories(test_portal_origen PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-14 COMMAND test_portal_origen)

# TC-POR-03 · FR-POR-04 (validador de §17)
add_executable(test_portal_validar test_portal_validar.c ${COMP}/portal/portal_validar.c ${COMP}/portal/portal_form.c)
target_include_directories(test_portal_validar PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-03 COMMAND test_portal_validar)

# TC-POR-04 · FR-POR-04 (respuesta a un envío inválido)
add_executable(test_portal_envio test_portal_envio.c ${COMP}/portal/portal_urlenc.c ${COMP}/portal/portal_validar.c ${COMP}/portal/portal_form.c)
target_include_directories(test_portal_envio PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-04 COMMAND test_portal_envio)

# TC-POR-06 · FR-POR-02 (redirección y DNS del portal cautivo)
add_executable(test_portal_cautivo test_portal_cautivo.c ${COMP}/portal/portal_cautivo.c)
target_include_directories(test_portal_cautivo PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-06 COMMAND test_portal_cautivo)

# TC-POR-10 · FR-POR-06 (los datos del formulario se guardan tal cual)
add_executable(test_portal_config test_portal_config.c ${COMP}/portal/portal_config.c ${COMP}/portal/portal_validar.c ${COMP}/portal/portal_form.c)
target_include_directories(test_portal_config PRIVATE ${COMP}/portal/include)
add_test(NAME TC-POR-10 COMMAND test_portal_config)

# TC-SEC-02 · FR-SEC-03 (el log de páginas no escribe la consulta de la URL)
add_executable(test_log_ruta test_log_ruta.c ${COMP}/portal/portal_cautivo.c)
target_include_directories(test_log_ruta PRIVATE ${COMP}/portal/include)
add_test(NAME TC-SEC-02 COMMAND test_log_ruta)

# TC-LOG-02 · FR-LOG-02 (ensamblado de líneas del log UDP)
add_executable(test_udplog_lineas test_udplog_lineas.c ${COMP}/udplog/udplog_lineas.c)
target_include_directories(test_udplog_lineas PRIVATE ${COMP}/udplog/include)
add_test(NAME TC-LOG-02 COMMAND test_udplog_lineas)


# TC-CON-02 · FR-CON-02 (función de backoff)
add_executable(test_backoff test_backoff.c ${COMP}/conectividad/conectividad_backoff.c)
target_include_directories(test_backoff PRIVATE ${COMP}/conectividad/include)
add_test(NAME TC-CON-02 COMMAND test_backoff)

# TC-DIAG-01 · FR-MQTT-08 (valores de `reset` de §9.3, también en E1)
add_executable(test_diag_reset test_diag_reset.c ${COMP}/diagnostico/diag_reset.c)
target_include_directories(test_diag_reset PRIVATE ${COMP}/diagnostico/include)
add_test(NAME TC-DIAG-01 COMMAND test_diag_reset)

# TC-MQTT-01 · FR-MQTT-02 y TC-MQTT-02 · FR-MQTT-08 (serializadores del publicador)
add_executable(test_publicador test_publicador.c ${COMP}/publicador/publicador.c)
target_include_directories(test_publicador PRIVATE ${COMP}/publicador/include)
add_test(NAME TC-MQTT-01 COMMAND test_publicador mqtt01)
add_test(NAME TC-MQTT-02 COMMAND test_publicador mqtt02)

# TC-HA-01 · clima:FR-HA-02, clima:FR-HA-03, FR-HA-A1 (Discovery de §14)
add_executable(test_ha_discovery test_ha_discovery.c ${COMP}/ha_discovery/ha_discovery.c)
target_include_directories(test_ha_discovery PRIVATE ${COMP}/ha_discovery/include)
add_test(NAME TC-HA-01 COMMAND test_ha_discovery)

# TC-LOG-05 · FR-LOG-02, FR-LOG-03 (dos escritores a la vez; acumulador por escritor, H-55)
find_package(Threads REQUIRED)
add_executable(test_udplog_escritores test_udplog_escritores.c ${COMP}/udplog/udplog_escritores.c ${COMP}/udplog/udplog_lineas.c)
target_include_directories(test_udplog_escritores PRIVATE ${COMP}/udplog/include)
target_link_libraries(test_udplog_escritores Threads::Threads)
add_test(NAME TC-LOG-05 COMMAND test_udplog_escritores)

# TC-LOG-06 · FR-LOG-02, FR-LOG-03 (reintento del envío UDP, H-57)
add_executable(test_udplog_envio test_udplog_envio.c ${COMP}/udplog/udplog_envio.c)
target_include_directories(test_udplog_envio PRIVATE ${COMP}/udplog/include)
add_test(NAME TC-LOG-06 COMMAND test_udplog_envio)

# TC-TIME-02 · FR-TIME-02 (validez de la hora) y TC-TIME-07 · FR-TIME-04, FR-TIME-06 (planificador SNTP)
add_executable(test_hora test_hora.c ${COMP}/hora/hora.c)
target_include_directories(test_hora PRIVATE ${COMP}/hora/include)
add_test(NAME TC-TIME-02 COMMAND test_hora time02)
add_test(NAME TC-TIME-07 COMMAND test_hora time07)

# Núcleos puros de §5–§7: TC-FUG-A1 · FR-FUG-01, TC-ENE-A1 · FR-ENE-02, TC-NIV-A1 · FR-NIV-01,
# TC-PRE-A1 · FR-PRE-01, TC-CIC-A1 · FR-CIC-01/02
add_executable(test_nucleo test_nucleo.c ${COMP}/fugas/fugas.c ${COMP}/energia/energia.c ${COMP}/bomba/bomba.c)
target_include_directories(test_nucleo PRIVATE ${COMP}/fugas/include ${COMP}/energia/include ${COMP}/bomba/include)
target_link_libraries(test_nucleo m)
add_test(NAME TC-FUG-A1 COMMAND test_nucleo fug)
add_test(NAME TC-ENE-A1 COMMAND test_nucleo ene)
add_test(NAME TC-NIV-A1 COMMAND test_nucleo niv)
add_test(NAME TC-PRE-A1 COMMAND test_nucleo pre)
add_test(NAME TC-CIC-A1 COMMAND test_nucleo cic)
