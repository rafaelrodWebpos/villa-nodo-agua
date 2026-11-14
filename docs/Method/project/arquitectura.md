# Nodo Agua — Arquitectura del código (CÓMO)

**Estado:** pendiente; se escribe en `/harness`.

Reglas ya fijadas por el FSD (§2.3, §24.0): la lógica de protección (§5–§9) se
escribe como funciones puras sin dependencias de ESP-IDF, para probarla en host;
la tarea de protección no hace llamadas de red. Los módulos comunes se toman del
piloto `villa-nodo-clima-piloto` en la versión que cumple su FSD 0.9.9.
