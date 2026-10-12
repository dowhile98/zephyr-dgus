/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * API pública principal del módulo driver DGUS II (DWIN T5L).
 */

#ifndef ZEPHYR_MODULES_DWIN_DGUS2_H_
#define ZEPHYR_MODULES_DWIN_DGUS2_H_

#include <dgus2/dgus2_types.h>
#include <dgus2/dgus2_vp.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el módulo DGUS II para la instancia especificada.
 *
 * Configura la UART subyacente e inicia el hilo RX del driver.
 * Es idempotente si el dispositivo ya fue inicializado por el kernel (POST_KERNEL).
 *
 * @param dev Instancia del dispositivo DGUS II (ej. DEVICE_DT_GET(DT_NODELABEL(dgus0)))
 * @return 0 en éxito, negativo en error.
 */
int dgus2_init(const struct device *dev);

/**
 * @brief Escribe un entero de 16 bits (uint16_t) en una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP de destino (ej. 0x1000)
 * @param value Valor a enviar (codificado automáticamente en Big-Endian)
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_vp_u16(const struct device *dev, uint16_t vp_addr, uint16_t value);

/**
 * @brief Escribe un entero sin signo de 32 bits (uint32_t, 2 words) en una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP de destino
 * @param value Valor a enviar (codificado en Big-Endian)
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_vp_u32(const struct device *dev, uint16_t vp_addr, uint32_t value);

/**
 * @brief Escribe un entero con signo de 16 bits (int16_t, 1 word) en una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP de destino
 * @param value Valor a enviar (codificado en Big-Endian complemento a dos)
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_vp_i16(const struct device *dev, uint16_t vp_addr, int16_t value);

/**
 * @brief Escribe un entero con signo de 32 bits (int32_t, 2 words) en una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP de destino
 * @param value Valor a enviar (codificado en Big-Endian complemento a dos)
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_vp_i32(const struct device *dev, uint16_t vp_addr, int32_t value);

/**
 * @brief Escribe un buffer de bytes arbitrarios en una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP de destino
 * @param data Puntero a los datos
 * @param len Longitud en bytes
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_vp_bytes(const struct device *dev, uint16_t vp_addr, const uint8_t *data, uint16_t len);

/**
 * @brief Escribe una cadena de texto (String) en una dirección VP.
 *
 * Añade automáticamente el terminador 0xFFFF de fin de string DGUS II si hay espacio.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP del control de texto
 * @param str Cadena de caracteres terminada en nulo
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_vp_string(const struct device *dev, uint16_t vp_addr, const char *str);

/**
 * @brief Solicita y lee de forma síncrona (polling bloqueante con timeout) los datos de un VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP a consultar
 * @param out_buf Buffer donde guardar los datos devueltos
 * @param max_len Capacidad del buffer
 * @param out_len Bytes realmente recibidos
 * @param timeout Tiempo máximo de espera (ej. K_MSEC(300))
 * @return 0 en éxito, -ETIMEDOUT en tiempo agotado, o error negativo.
 */
int dgus2_read_vp_sync(const struct device *dev, uint16_t vp_addr, uint8_t *out_buf, uint16_t max_len, uint16_t *out_len, k_timeout_t timeout);

/**
 * @brief Lee de forma síncrona un entero sin signo de 16 bits (uint16_t) desde una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP a consultar
 * @param value Puntero donde se almacenará el valor leído (decodificado de Big-Endian)
 * @param timeout Tiempo máximo de espera
 * @return 0 en éxito, -ETIMEDOUT en tiempo agotado, o error negativo.
 */
int dgus2_read_vp_u16_sync(const struct device *dev, uint16_t vp_addr, uint16_t *value, k_timeout_t timeout);

/**
 * @brief Lee de forma síncrona un entero sin signo de 32 bits (uint32_t) desde una dirección VP.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP a consultar
 * @param value Puntero donde se almacenará el valor leído (decodificado de Big-Endian)
 * @param timeout Tiempo máximo de espera
 * @return 0 en éxito, -ETIMEDOUT en tiempo agotado, o error negativo.
 */
int dgus2_read_vp_u32_sync(const struct device *dev, uint16_t vp_addr, uint32_t *value, k_timeout_t timeout);

/**
 * @brief Solicita el cambio de página activa en la pantalla DWIN.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param page_id Número de página a activar (0..65535)
 * @return 0 en éxito, negativo en error.
 */
int dgus2_set_page(const struct device *dev, uint16_t page_id);

/**
 * @brief Consulta de forma síncrona el número de página actualmente activa en la pantalla.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param page_id Puntero donde se almacenará el ID de la página leída
 * @param timeout Tiempo máximo de espera
 * @return 0 en éxito, -ETIMEDOUT en tiempo agotado o error negativo.
 */
int dgus2_get_page_sync(const struct device *dev, uint16_t *page_id, k_timeout_t timeout);

/**
 * @brief Configura el nivel de brillo del backlight de la pantalla DWIN (0 a 100%).
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param brightness_pct Porcentaje de brillo (0..100)
 * @return 0 en éxito, negativo en error.
 */
int dgus2_set_brightness(const struct device *dev, uint8_t brightness_pct);

/**
 * @brief Activa el buzzer interno de la pantalla DWIN por una duración determinada.
 *
 * Escala con base de tiempo de 8 ms (duración / 8, mínimo 1 unidad si duration_ms > 0).
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param duration_ms Duración del beep en milisegundos
 * @return 0 en éxito, negativo en error.
 */
int dgus2_trigger_buzzer(const struct device *dev, uint16_t duration_ms);

/**
 * @brief Escribe una serie de puntos en un canal de curva de tendencia en tiempo real (Command 0x84).
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param channel Canal de la curva (0x00 .. 0x07)
 * @param points Arreglo de valores de 16 bits para graficar
 * @param count Cantidad de puntos a transmitir
 * @return 0 en éxito, negativo en error.
 */
int dgus2_write_curve(const struct device *dev, uint8_t channel, const uint16_t *points, uint16_t count);

/**
 * @brief Registra una función callback para notificaciones asíncronas de la pantalla (Eventos Touch).
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP a monitorear
 * @param cb Función callback invocada al recibir datos de la pantalla
 * @param user_data Contexto del usuario pasado al callback
 * @return 0 en éxito, -ENOMEM si se excede CONFIG_DGUS2_MAX_SUBSCRIBERS.
 */
int dgus2_register_vp_callback(const struct device *dev, uint16_t vp_addr, dgus2_vp_event_cb_t cb, void *user_data);

/**
 * @brief Elimina una función callback registrada previamente para una dirección VP.
 *
 * Compacta el arreglo interno de suscriptores con seguridad de concurrencia.
 *
 * @param dev Instancia del dispositivo DGUS II
 * @param vp_addr Dirección VP monitoreada
 * @param cb Función callback que se desea desregistrar
 * @return 0 en éxito, -ENOENT si no se encontró el callback, o error negativo.
 */
int dgus2_unregister_vp_callback(const struct device *dev, uint16_t vp_addr, dgus2_vp_event_cb_t cb);

/**
 * @brief Retorna el contador de tramas con error de parseo FSM de la instancia (diagnóstico).
 *
 * @param dev Instancia del dispositivo DGUS II
 * @return Cantidad de tramas descartadas por checksum o encabezado erróneo.
 */
uint32_t dgus2_get_parse_error_count(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_DWIN_DGUS2_H_ */
