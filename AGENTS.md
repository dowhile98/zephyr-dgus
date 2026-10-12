# AGENTS.md — zephyr-dgus

## Idioma
Este proyecto usa español. pero Los mensajes de commit, comentarios y documentación deben escribirse en ingles salvo que se indique lo contrario.

## Estructura del módulo
Módulo Zephyr out-of-tree para pantallas DWIN DGUS II (protocolo UART).

```
src/              → Implementación del driver (dgus2_core.c, dgus2_hal.c, dgus2_protocol.c)
include/dgus2/    → Headers públicos
tests/            → Unit tests con Ztest (Twister)
samples/          → Aplicación de ejemplo
reference/        → Documentación del protocolo DGUS II (PDFs)
dts/              → Devicetree bindings
zephyr/module.yml → Registro como módulo Zephyr
```

El Kconfig principal es `CONFIG_DGUS2_DRIVER` (requiere `CONFIG_SERIAL` y selecciona `CONFIG_UART_INTERRUPT_DRIVEN`).

## Herramientas
- **West y herramientas Zephyr**: `/home/tecna/zephyrproject`
- **Zephyr SDK**: `/home/tecna/zephyr-sdk-1.0.1`
- **PDF tools (pdftotext, etc.)**: Activar con `source .venv/bin/activate`

## Comandos de build

```bash
# Activar entorno Zephyr
source /home/tecna/zephyrproject/zephyr/zephyr-env.sh

# Build para native_sim (host)
west build -b native_sim tests/

# Build para STM32 (ejemplo)
west build -b nucleo_f429zi samples/dwin-interface/

# Build limpio
west build -p auto
```

## Comandos de test

```bash
# Ejecutar unit tests con Twister
west twister -T tests/ -p native_sim

# Ejecutar tests específicos por tag
west twister -T tests/ -t dgus -p native_sim

# Ver output de tests fallidos
west twister -T tests/ -p native_sim --verbose
```

Los tests usan el harness `ztest` y corren sobre `native_sim` (no requieren hardware).

## Convenciones del proyecto

### Arquitectura
El driver sigue una separación por capas:
- **dgus2_protocol.c** → Parsing/construcción de frames (sin dependencias de hardware)
- **dgus2_core.c** → Lógica principal del driver, threading, suscriptores
- **dgus2_hal.c** → Abstracción de UART (único punto de acople con hardware)

### Testing
- Unit tests deben ejecutarse en `native_sim` sin hardware
- Usar Ztest (`zassert_*`) para assertions
- El tag principal es `dgus` (ver `tests/testcase.yaml`)
- Tests de protocolo no deben depender de UART real

### Sample
El sample en `samples/dwin-interface/` demuestra:
- Inicialización del driver
- Escritura/lectura de variables VP
- Cambio de páginas
- Suscripción a eventos touch

### Documentación de referencia
Los PDFs en `reference/` contienen la especificación del protocolo DGUS II. Cualquier implementación debe validar comandos, formatos de frame y comportamientos contra estos documentos.

## Prioridades de diseño
1. **Correctness** sobre performance
2. **Testabilidad** — código debe poder probarse sin hardware
3. **Portabilidad** — separar protocolo de implementación UART
4. **Simplicidad** — evitar abstracciones innecesarias
5. **Convenciones Zephyr** — seguir patrones estándar del RTOS

## Notas operativas
- El `promt.md` contiene el plan de diseño y requisitos detallados del módulo
- El README del sample (`samples/dwin-interface/README.rst`) está incompleto (placeholder)
- No usar `printk()` para debugging; usar el subsistema de logging de Zephyr (`LOG_INF`, `LOG_DBG`, etc.)
- Evitar memoria dinámica; usar buffers estáticos y ring buffers donde sea posible
- La concurrencia se maneja con threads, semáforos y mutexes de Zephyr (ver `struct dgus2_data` en `src/dgus2_internal.h`)
