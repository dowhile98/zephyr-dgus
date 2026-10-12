/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    dgus2.h
 * @brief   Public API for the DWIN DGUS II smart display driver in Zephyr RTOS.
 * @ingroup dgus2_api
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
 * @defgroup dgus2_api DGUS II Display Driver API
 * @brief    Public API and control functions for DWIN DGUS II smart displays.
 * @{
 */

/**
 * @brief Initializes the DGUS II display driver instance.
 *
 * Configures the underlying UART interface and spawns the driver RX worker thread.
 * This call is idempotent if the device was already initialized during kernel boot.
 *
 * @param[in] dev DGUS II device instance pointer (e.g. DEVICE_DT_GET(DT_NODELABEL(dgus0))).
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL or invalid.
 * @retval -ENODEV  If the underlying UART device is not ready.
 * @retval negative Negative errno code on failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_init(const struct device *dev);

/**
 * @brief Writes an unsigned 16-bit integer (uint16_t) to a VP address.
 *
 * Transmits 1 word (2 bytes) encoded in Big-Endian format using Command 0x82.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Target DGUS II VP address (e.g. 0x1000).
 * @param[in] value   16-bit value to transmit (encoded automatically in Big-Endian).
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL or device data is invalid.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_vp_u16(const struct device *dev, uint16_t vp_addr, uint16_t value);

/**
 * @brief Writes an unsigned 32-bit integer (uint32_t, 2 words) to a VP address.
 *
 * Transmits 2 words (4 bytes) encoded in Big-Endian format using Command 0x82.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Target DGUS II VP address.
 * @param[in] value   32-bit value to transmit (encoded automatically in Big-Endian).
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL or device data is invalid.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_vp_u32(const struct device *dev, uint16_t vp_addr, uint32_t value);

/**
 * @brief Writes a signed 16-bit integer (int16_t, 1 word) to a VP address.
 *
 * Transmits 1 word (2 bytes) encoded in two's complement Big-Endian using Command 0x82.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Target DGUS II VP address.
 * @param[in] value   Signed 16-bit value to transmit.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL or device data is invalid.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_vp_i16(const struct device *dev, uint16_t vp_addr, int16_t value);

/**
 * @brief Writes a signed 32-bit integer (int32_t, 2 words) to a VP address.
 *
 * Transmits 2 words (4 bytes) encoded in two's complement Big-Endian using Command 0x82.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Target DGUS II VP address.
 * @param[in] value   Signed 32-bit value to transmit.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL or device data is invalid.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_vp_i32(const struct device *dev, uint16_t vp_addr, int32_t value);

/**
 * @brief Writes an arbitrary buffer of bytes to a VP address.
 *
 * Packages raw bytes into a Command 0x82 write transaction.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Target DGUS II VP address.
 * @param[in] data    Pointer to the byte buffer to transmit.
 * @param[in] len     Length of the buffer in bytes (must fit within protocol limits).
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev or @p data is NULL, or @p len exceeds DGUS2_MAX_PAYLOAD_LEN.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_vp_bytes(const struct device *dev, uint16_t vp_addr, const uint8_t *data,
			 uint16_t len);

/**
 * @brief Writes a null-terminated ASCII string to a VP address.
 *
 * Automatically appends the DGUS II 0xFFFF string terminator if payload capacity permits.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Target DGUS II VP address assigned to a text display control.
 * @param[in] str     Null-terminated string to send to the display.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev or @p str is NULL, or string length exceeds limits.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_vp_string(const struct device *dev, uint16_t vp_addr, const char *str);

/**
 * @brief Synchronously queries and reads raw bytes from a VP address with a timeout.
 *
 * Transmits a Command 0x83 read frame and blocks the calling thread until the corresponding
 * response frame arrives or @p timeout expires.
 *
 * @param[in]  dev     DGUS II device instance pointer.
 * @param[in]  vp_addr Target DGUS II VP address to read.
 * @param[out] out_buf Destination buffer where received payload bytes will be copied.
 * @param[in]  max_len Capacity of @p out_buf in bytes.
 * @param[out] out_len Pointer to store the number of bytes actually received.
 * @param[in]  timeout Maximum wait duration for the display response.
 *
 * @retval 0         On success.
 * @retval -EINVAL   If pointers are NULL or @p max_len is 0.
 * @retval -ETIMEDOUT If no response was received within @p timeout.
 * @retval negative  Negative errno code on transmission or protocol error.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_read_vp_sync(const struct device *dev, uint16_t vp_addr, uint8_t *out_buf,
		       uint16_t max_len, uint16_t *out_len, k_timeout_t timeout);

/**
 * @brief Reads an unsigned 16-bit integer (uint16_t) synchronously from a VP address.
 *
 * Requests 1 word from the display and decodes it from Big-Endian format.
 *
 * @param[in]  dev     DGUS II device instance pointer.
 * @param[in]  vp_addr Target DGUS II VP address.
 * @param[out] value   Pointer to variable where decoded 16-bit value will be stored.
 * @param[in]  timeout Maximum wait duration for the display response.
 *
 * @retval 0         On success.
 * @retval -EINVAL   If @p dev or @p value is NULL.
 * @retval -ETIMEDOUT If no response was received within @p timeout.
 * @retval negative  Negative errno code on failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_read_vp_u16_sync(const struct device *dev, uint16_t vp_addr, uint16_t *value,
			   k_timeout_t timeout);

/**
 * @brief Reads an unsigned 32-bit integer (uint32_t) synchronously from a VP address.
 *
 * Requests 2 words from the display and decodes them from Big-Endian format.
 *
 * @param[in]  dev     DGUS II device instance pointer.
 * @param[in]  vp_addr Target DGUS II VP address.
 * @param[out] value   Pointer to variable where decoded 32-bit value will be stored.
 * @param[in]  timeout Maximum wait duration for the display response.
 *
 * @retval 0         On success.
 * @retval -EINVAL   If @p dev or @p value is NULL.
 * @retval -ETIMEDOUT If no response was received within @p timeout.
 * @retval negative  Negative errno code on failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_read_vp_u32_sync(const struct device *dev, uint16_t vp_addr, uint32_t *value,
			   k_timeout_t timeout);

/**
 * @brief Requests an active page switch on the DGUS II display.
 *
 * Sends a 4-byte command sequence (0x5A 0x01 + PageID) to VP 0x0084.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] page_id Page number to display (0x0000 .. 0xFFFF).
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_set_page(const struct device *dev, uint16_t page_id);

/**
 * @brief Synchronously queries the currently active page ID from the display.
 *
 * Queries system register 0x14 (or VP 0x0084) and returns the current Page ID.
 *
 * @param[in]  dev     DGUS II device instance pointer.
 * @param[out] page_id Pointer to store the active page ID.
 * @param[in]  timeout Maximum wait duration for the display response.
 *
 * @retval 0         On success.
 * @retval -EINVAL   If @p dev or @p page_id is NULL.
 * @retval -ETIMEDOUT If no response was received within @p timeout.
 * @retval negative  Negative errno code on failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_get_page_sync(const struct device *dev, uint16_t *page_id, k_timeout_t timeout);

/**
 * @brief Sets backlight brightness percentage (0% to 100%).
 *
 * Transmits brightness level to VP 0x0082. Values > 100 are automatically clamped to 100.
 *
 * @param[in] dev            DGUS II device instance pointer.
 * @param[in] brightness_pct Backlight level from 0 to 100 percent.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_set_brightness(const struct device *dev, uint8_t brightness_pct);

/**
 * @brief Activates the internal buzzer on the DGUS II display.
 *
 * Scales the requested duration using 8ms increments (duration / 8, minimum 1 if > 0)
 * and writes to VP 0x00A0.
 *
 * @param[in] dev         DGUS II device instance pointer.
 * @param[in] duration_ms Buzzer activation duration in milliseconds.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev is NULL.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_trigger_buzzer(const struct device *dev, uint16_t duration_ms);

/**
 * @brief Writes real-time trend curve points using DGUS II Command 0x84.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] channel Curve channel index (0x00 .. 0x07).
 * @param[in] points  Array of 16-bit point values to plot.
 * @param[in] count   Number of 16-bit points (must satisfy count * 2 <= DGUS2_MAX_PAYLOAD_LEN - 4).
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev or @p points is NULL, or @p count is 0 / out of bounds.
 * @retval negative Negative errno code on transmission failure.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_write_curve(const struct device *dev, uint8_t channel, const uint16_t *points,
		      uint16_t count);

/**
 * @brief Registers an asynchronous notification callback for a VP address.
 *
 * When the display transmits a touch event or VP update for @p vp_addr, the registered
 * callback function @p cb will be executed by the driver RX thread.
 *
 * @param[in] dev       DGUS II device instance pointer.
 * @param[in] vp_addr   Target VP address to monitor.
 * @param[in] cb        Callback function to invoke on event.
 * @param[in] user_data Context pointer passed to the callback.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev or @p cb is NULL.
 * @retval -ENOMEM  If subscriber table is full (exceeds CONFIG_DGUS2_MAX_SUBSCRIBERS).
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_register_vp_callback(const struct device *dev, uint16_t vp_addr, dgus2_vp_event_cb_t cb,
			       void *user_data);

/**
 * @brief Unregisters a previously registered VP callback.
 *
 * Safely removes the callback from the subscriber registry and compacts the array.
 *
 * @param[in] dev     DGUS II device instance pointer.
 * @param[in] vp_addr Monitored VP address.
 * @param[in] cb      Callback function to unregister.
 *
 * @retval 0        On success.
 * @retval -EINVAL  If @p dev or @p cb is NULL.
 * @retval -ENOENT  If matching subscriber registration was not found.
 *
 * @note Thread-safe, non-ISR context. May sleep waiting on mutex/semaphore.
 */
int dgus2_unregister_vp_callback(const struct device *dev, uint16_t vp_addr,
				 dgus2_vp_event_cb_t cb);

/**
 * @brief Returns the cumulative FSM parser error count for diagnostic monitoring.
 *
 * @param[in] dev DGUS II device instance pointer.
 *
 * @return Total number of frames rejected due to invalid headers, length, or framing errors.
 *
 * @note Thread-safe.
 */
uint32_t dgus2_get_parse_error_count(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_DWIN_DGUS2_H_ */
