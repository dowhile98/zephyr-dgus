# Revisión, mejora y evolución de módulo DWIN DGUS para Zephyr RTOS

Quiero que actúes como **ingeniero senior de software embebido especializado en Zephyr RTOS, C, drivers, protocolos UART, arquitectura de software, Clean Architecture, SOLID y testing/TDD para sistemas embebidos**.

Estoy desarrollando un **driver/módulo para Zephyr RTOS destinado a controlar pantallas DWIN DGUS mediante interfaz UART**.

Ya existe una implementación funcional, pero considero que requiere una **revisión técnica profunda, correcciones, mejoras arquitectónicas, ampliación de funcionalidades y una estrategia formal de testing**.

El objetivo NO es simplemente "hacer que funcione", sino convertirlo en un módulo:

- mantenible
- escalable
- testeable
- portable entre diferentes placas STM32 y otros SoC soportados por Zephyr
- correctamente integrado con Zephyr RTOS
- con una API clara y estable
- con bajo acoplamiento
- siguiendo principios de Clean Architecture y SOLID cuando sean aplicables
- desarrollado y evolucionado mediante TDD
- acompañado de ejemplos y tests que permitan validar su funcionamiento

---

## 1. Primero: analizar antes de modificar

**No comiences modificando el código inmediatamente.**

Primero realiza una auditoría completa del módulo existente.

Revisa:

- estructura de directorios
- archivos `.c` y `.h`
- API pública
- API interna
- estructuras de datos
- manejo de memoria
- buffers
- parsing de paquetes
- generación de paquetes
- comunicación UART
- callbacks
- concurrencia
- uso de threads/workqueues/timers, si existen
- manejo de errores
- estados internos
- timeouts
- sincronización
- logging
- configuración mediante Kconfig
- DeviceTree, si corresponde
- integración con APIs de Zephyr
- dependencias entre componentes
- portabilidad
- testabilidad
- documentación
- nomenclatura
- cumplimiento de convenciones de Zephyr

Identifica explícitamente:

1. problemas actuales
2. posibles bugs
3. condiciones de carrera
4. problemas de diseño
5. acoplamientos innecesarios
6. funcionalidades incompletas
7. funcionalidades que deberían existir pero todavía no están implementadas
8. código difícil de testear
9. posibles problemas de escalabilidad
10. posibles problemas de mantenimiento futuro

**No asumas que la arquitectura actual es correcta.**

Pero tampoco reescribas todo innecesariamente.

Si propones una modificación importante, explica:

- problema actual
- por qué es un problema
- alternativa propuesta
- ventajas
- desventajas
- impacto sobre la API existente
- si requiere cambios incompatibles

---

# 2. Revisar la documentación de referencia

Existe una carpeta:

```text
reference/
```

Esta carpeta contiene documentación relacionada con las pantallas DWIN DGUS y/o el protocolo utilizado.

Debes revisar cuidadosamente su contenido antes de diseñar nuevas funcionalidades.

Utiliza la documentación como fuente para determinar:

- formato de paquetes
- comandos
- registros
- VP
- SP
- variables
- tipos de datos
- escritura
- lectura
- respuestas
- errores
- checksum/CRC, si corresponde
- direccionamiento
- tamaños
- restricciones
- comportamiento de la interfaz UART
- tiempos y timeouts
- funcionalidades disponibles en DGUS

Cuando encuentres una funcionalidad documentada que todavía no esté implementada, evalúa si debería formar parte del módulo.

**No inventes funcionalidades del protocolo.**

Si existe una ambigüedad entre la implementación y la documentación, indícala explícitamente.

---

# 3. Evaluar la arquitectura

Analiza si la arquitectura actual permite separar correctamente responsabilidades.

Como mínimo evalúa conceptualmente la separación entre:

```text
Application / User
        │
        ▼
DWIN DGUS API
        │
        ▼
Protocol / Frame Layer
        │
        ▼
Transport Layer
        │
        ▼
UART / Zephyr
```

La implementación debe evitar que la lógica del protocolo quede excesivamente acoplada a la implementación concreta de UART.

Evalúa si conviene separar:

### API pública

Responsable de proporcionar una interfaz sencilla al usuario del módulo.

Ejemplos conceptuales:

```c
dwin_init()
dwin_write_vp()
dwin_read_vp()
dwin_set_page()
dwin_get_page()
```

### Protocol layer

Responsable de:

- construir frames
- interpretar frames
- validar frames
- checksum/CRC
- interpretar comandos
- convertir datos
- detectar errores

### Transport layer

Responsable exclusivamente de transportar bytes.

Idealmente debería ser posible probar el protocolo sin utilizar hardware UART real.

---

# 4. Clean Architecture y SOLID

Aplica Clean Architecture y SOLID de forma **pragmática**, apropiada para software embebido en C.

No quiero una arquitectura innecesariamente compleja.

Evalúa especialmente:

- Single Responsibility Principle
- Dependency Inversion Principle
- separación entre protocolo y hardware
- interfaces mediante function pointers cuando sea útil
- inversión de dependencias
- bajo acoplamiento
- alta cohesión
- facilidad de testing

Evita introducir abstracciones únicamente por seguir un patrón.

Cada abstracción debe tener una justificación técnica.

---

# 5. Revisar y ampliar funcionalidades

Después de entender completamente la implementación existente y la documentación, identifica funcionalidades faltantes.

Por ejemplo, dependiendo de lo soportado realmente por DGUS:

- lectura de VP
- escritura de VP
- diferentes tamaños de datos
- enteros
- valores signed/unsigned
- endianess
- strings
- arrays
- páginas
- comandos del sistema
- respuesta/ACK
- callbacks/eventos
- recepción asíncrona
- timeout
- retry
- detección de frames inválidos
- checksum/CRC
- manejo de errores
- múltiples dispositivos
- configuración mediante DeviceTree/Kconfig
- logging
- estadísticas/diagnóstico

**No implementes automáticamente todos estos puntos.**

Primero determina cuáles son realmente necesarios según:

1. documentación de `reference/`
2. arquitectura existente
3. necesidades del módulo
4. convenciones de Zephyr
5. mantenibilidad

Presenta primero la propuesta de funcionalidades y luego implementa las prioritarias.

---

# 6. Testing — requisito fundamental

El testing debe ser tratado como una parte principal del desarrollo.

Quiero utilizar la **suite de testing oficial de Zephyr**.

Investiga primero cómo Zephyr estructura actualmente sus tests y sigue sus convenciones.

El desarrollo debe seguir una metodología **TDD (Test Driven Development)** siempre que sea razonablemente aplicable.

La idea es:

```text
Test
  ↓
Implementación mínima
  ↓
Test
  ↓
Refactor
  ↓
Test
```

Los tests deben permitir detectar regresiones cuando se modifique el driver.

---

# 7. Separar tests unitarios de tests hardware

Quiero distinguir claramente entre:

### Unit tests

Deben poder ejecutarse sin hardware físico.

Deben probar principalmente:

- construcción de frames
- parsing
- validación
- checksum/CRC
- conversión de datos
- manejo de errores
- estados
- timeouts simulables
- callbacks
- edge cases
- frames corruptos
- frames incompletos
- frames demasiado largos
- datos inválidos

Siempre que sea posible, estos tests deben ejecutarse en:

```text
native_sim
```

o el mecanismo equivalente recomendado actualmente por Zephyr.

### Hardware tests / integration tests

Deben utilizar una placa real y validar:

```text
Zephyr
   ↓
DWIN driver
   ↓
UART
   ↓
DWIN display
```

Estos tests pueden ejecutarse inicialmente en placas STM32 disponibles para el proyecto.

Actualmente disponemos, entre otras, de:

- STM32F407
- STM32F429
- NUCLEO-H753

No asumas que una determinada placa tiene exactamente los mismos periféricos o configuración UART.

Utiliza DeviceTree/Kconfig de Zephyr correctamente para hacer el ejemplo portable.

---

# 8. Crear ejemplo en samples/

Debe existir un ejemplo:

```text
samples/dwin-interface/
```

Este ejemplo debe ser realmente útil para un desarrollador.

No quiero un simple `hello world`.

Debe demostrar las principales capacidades del módulo.

Por ejemplo:

```text
Inicialización
      ↓
Configuración UART
      ↓
Inicialización DWIN
      ↓
Escritura de VP
      ↓
Lectura de VP
      ↓
Cambio de página
      ↓
Recepción de eventos
      ↓
Logs
```

El ejemplo debe servir como:

1. demostración
2. herramienta de validación
3. referencia para nuevos desarrolladores
4. prueba de integración básica

Debe incluir la configuración necesaria de:

```text
prj.conf
CMakeLists.txt
Kconfig
DeviceTree overlays
```

cuando sean necesarios.

Documenta claramente cómo ejecutarlo en una placa STM32.

---

# 9. API pública

Revisa la API pública y determina si es:

- intuitiva
- consistente
- suficientemente genérica
- fácil de utilizar
- estable
- segura
- coherente con Zephyr

Evita exponer estructuras internas innecesariamente.

Utiliza tipos y convenciones apropiadas para C y Zephyr.

Evalúa también:

- `enum`
- `struct`
- `typedef`
- códigos de error
- `const`
- tamaños explícitos
- `size_t`
- `uint8_t`, `uint16_t`, etc.
- documentación Doxygen

---

# 10. Manejo de errores

El driver debe tener una estrategia clara de errores.

Evalúa y mejora:

- errores UART
- timeout
- frame inválido
- checksum incorrecto
- comando no soportado
- buffer overflow
- parámetros inválidos
- estado incorrecto
- dispositivo no disponible

Utiliza los mecanismos y convenciones apropiados de Zephyr cuando corresponda.

No ocultes errores silenciosamente.

Los errores deben ser fáciles de diagnosticar.

---

# 11. Concurrencia y RTOS

Como se trata de Zephyr, revisa cuidadosamente cualquier acceso concurrente.

Determina si realmente necesitamos:

- ISR
- callback UART
- thread
- work queue
- k_work
- k_msgq
- k_fifo
- k_mutex
- k_sem
- timers

No agregues mecanismos de sincronización innecesarios.

Analiza explícitamente:

- contexto ISR
- contexto thread
- callbacks
- acceso a buffers
- reentrancia
- race conditions

---

# 12. Memoria y buffers

Revisa cuidadosamente:

- buffers estáticos
- buffers dinámicos
- tamaños máximos
- overflow
- underflow
- lifetime
- ownership
- copias innecesarias
- uso de stack
- uso de heap

Para un módulo embebido, prioriza soluciones deterministas y predecibles.

Evita memoria dinámica si no es necesaria.

---

# 13. Logging y debugging

Integra correctamente el sistema de logging de Zephyr.

Debe ser posible habilitar diferentes niveles:

```text
ERR
WRN
INF
DBG
```

Los logs de debugging del protocolo deben ser opcionales.

Evita llenar el código con `printk()` si existe una alternativa adecuada mediante logging.

---

# 14. Documentación

El módulo debe quedar correctamente documentado.

Incluye:

- README
- API pública
- arquitectura
- flujo de comunicación
- ejemplos
- configuración
- testing
- limitaciones
- hardware compatible
- cómo agregar nuevas funcionalidades

La documentación debe permitir que otro desarrollador pueda utilizar y extender el módulo sin tener que estudiar todo el código primero.

---

# 15. Compatibilidad con Zephyr

Sigue las convenciones actuales de Zephyr.

Revisa:

- estructura de drivers/modules
- Kconfig
- CMake
- DeviceTree
- logging
- testing
- naming
- documentación
- samples
- APIs recomendadas

**No inventes convenciones propias cuando Zephyr ya proporciona una solución estándar.**

Si existe una API de Zephyr apropiada, evalúa utilizarla.

---

# 16. Proceso de trabajo obligatorio

Quiero que trabajes en las siguientes fases.

## Fase 1 — Auditoría

No modificar código.

Entregar:

```text
1. Arquitectura actual
2. Componentes existentes
3. Problemas encontrados
4. Bugs potenciales
5. Funcionalidades existentes
6. Funcionalidades faltantes
7. Problemas de testing
8. Problemas de arquitectura
9. Riesgos
10. Recomendaciones
```

---

## Fase 2 — Plan de evolución

Crear un plan priorizado:

```text
P0 - críticos
P1 - importantes
P2 - mejoras
P3 - futuras funcionalidades
```

No hacer cambios masivos sin un plan.

---

## Fase 3 — Testing inicial

Antes de modificar comportamiento existente:

1. crear tests para el comportamiento actual que sea válido
2. detectar bugs mediante tests
3. establecer una baseline

Esto debe permitir evitar regresiones.

---

## Fase 4 — Refactor

Realizar mejoras arquitectónicas pequeñas y controladas.

Después de cada cambio:

```text
build
↓
unit tests
↓
integration tests si corresponde
↓
análisis
```

No acumular grandes cantidades de cambios sin validar.

---

## Fase 5 — Nuevas funcionalidades

Implementar funcionalidades faltantes siguiendo TDD:

```text
test
→ implementation
→ test
→ refactor
```

Cada funcionalidad importante debe incorporar sus propios tests.

---

## Fase 6 — Sample

Crear y validar:

```text
samples/dwin-interface/
```

El sample debe compilar correctamente y demostrar el uso real del módulo.

---

## Fase 7 — Validación final

Ejecutar:

- build
- unit tests
- Zephyr Twister cuando corresponda
- static analysis si está disponible
- compilación para las placas objetivo
- revisión de warnings
- revisión de API
- revisión de documentación

---

# 17. Uso de subagentes / skills

Si el entorno permite utilizar **subagentes o skills especializados**, utilízalos de forma coordinada.

Idealmente divide el análisis en:

### Agente 1 — Zephyr

Revisar:

- arquitectura de drivers
- DeviceTree
- Kconfig
- CMake
- UART
- testing
- Twister
- samples

### Agente 2 — DWIN/DGUS

Revisar:

- protocolo
- comandos
- frames
- VP
- SP
- checksum
- funcionalidades faltantes

### Agente 3 — C / Embedded Architecture

Revisar:

- Clean Architecture
- SOLID
- API
- memoria
- concurrencia
- mantenibilidad

### Agente 4 — Testing

Revisar:

- Ztest
- Twister
- native_sim
- unit tests
- integration tests
- cobertura
- edge cases
- estrategia TDD

Después consolida los resultados antes de realizar cambios.

**No permitas que diferentes agentes modifiquen simultáneamente las mismas partes del código sin coordinación.**

---

# 18. Restricciones importantes

No quiero:

- reescribir todo desde cero sin justificación
- introducir complejidad innecesaria
- abstraer por abstraer
- duplicar funcionalidades de Zephyr
- depender innecesariamente de hardware para unit tests
- tests que solamente comprueben que una función devuelve `0`
- código difícil de mantener
- APIs excesivamente complejas
- memoria dinámica innecesaria
- `printk()` como mecanismo principal de debugging
- funcionalidades inventadas que no estén respaldadas por la documentación

Quiero priorizar:

```text
Correctness
Maintainability
Testability
Portability
Simplicity
Determinism
Zephyr conventions
```

en ese orden cuando exista conflicto.

---

# 19. Criterios de aceptación

Consideraremos terminado el trabajo cuando:

- [ ] La arquitectura esté documentada.
- [ ] Los principales problemas existentes hayan sido identificados.
- [ ] Los bugs encontrados tengan tests de regresión.
- [ ] El protocolo pueda probarse sin hardware.
- [ ] Existan unit tests mediante la infraestructura de testing de Zephyr.
- [ ] Los tests puedan ejecutarse mediante Twister cuando corresponda.
- [ ] Exista un sample funcional en `samples/dwin-interface/`.
- [ ] El sample compile para las placas objetivo correspondientes.
- [ ] La API pública esté documentada.
- [ ] Las funcionalidades principales del protocolo estén cubiertas.
- [ ] Los errores estén correctamente gestionados.
- [ ] No existan warnings relevantes de compilación.
- [ ] El módulo sea portable respecto al UART/hardware.
- [ ] La documentación permita a otro desarrollador utilizar y extender el módulo.
- [ ] El diseño permita agregar nuevas funcionalidades sin modificar innecesariamente componentes existentes.

---

# Resultado esperado

El resultado final debe ser un **módulo DWIN DGUS para Zephyr de calidad de producción**, con una arquitectura clara, API mantenible, separación entre protocolo y transporte, testing automatizado, TDD, sample funcional y documentación suficiente para que pueda evolucionar a futuro.

Antes de realizar cambios importantes, **muéstrame primero el diagnóstico y el plan de implementación**.

No quiero únicamente que "corrijas el código": quiero que analices el diseño y me ayudes a convertirlo en un componente de software embebido mantenible y escalable dentro del ecosistema Zephyr.
