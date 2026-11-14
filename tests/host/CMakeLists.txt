# Nivel host: núcleos puros compilados con el compilador nativo, sin ESP-IDF
# (docs/Method/standards/testing.md). /build añade un test por requisito; la
# lógica de protección (FSD §5–§9) se prueba aquí como funciones puras.
cmake_minimum_required(VERSION 3.22)
project(nodo_agua_host_tests C)
enable_testing()
set(COMP ${CMAKE_CURRENT_SOURCE_DIR}/../../components)
add_compile_options(-Wall -Wextra -Werror)

# Comprueba que el nivel host funciona; no verifica ningún requisito del FSD.
add_executable(tier_smoke tier_smoke.c)
add_test(NAME tier_smoke COMMAND tier_smoke)
