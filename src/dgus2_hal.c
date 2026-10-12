/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * Capa de Hardware (HAL): Interrupciones UART de Zephyr + sys_ring_buf.
 * Arquitectura canónica Zephyr multi-instancia (Zero Static Globals).
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include <dgus2/dgus2_hal.h>
#include "dgus2_internal.h"

LOG_MODULE_REGISTER(dgus2_hal, CONFIG_LOG_DEFAULT_LEVEL);

static void dgus2_uart_isr(const struct device *uart_dev, void *user_data) {
	const struct device *dev = (const struct device *)user_data;
	if (!dev) {
		return;
	}

	struct dgus2_data *data = dev->data;
	if (!data) {
		return;
	}

	int err = uart_err_check(uart_dev);
	if (err) {
		LOG_WRN("UART error detected: 0x%x", err);
	}

	uart_irq_update(uart_dev);

	while (uart_irq_rx_ready(uart_dev)) {
		uint8_t byte;
		int read = uart_fifo_read(uart_dev, &byte, 1);
		if (read > 0) {
			if (ring_buf_put(&data->rx_ringbuf, &byte, 1) > 0) {
				k_sem_give(&data->rx_sem);
			} else {
				LOG_WRN("RX ring buffer overrun");
			}
		}
	}
}

int dgus2_hal_init(const struct device *dev) {
	if (!dev) {
		return -EINVAL;
	}

	const struct dgus2_config *config = dev->config;
	struct dgus2_data *data = dev->data;

	if (!config || !data) {
		return -EINVAL;
	}

	if (!config->uart_dev || !device_is_ready(config->uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	ring_buf_init(&data->rx_ringbuf, sizeof(data->rx_ringbuf_data), data->rx_ringbuf_data);
	k_sem_init(&data->rx_sem, 0, K_SEM_MAX_LIMIT);
	k_mutex_init(&data->tx_mutex);

	uart_irq_callback_user_data_set(config->uart_dev, dgus2_uart_isr, (void *)dev);
	uart_irq_rx_enable(config->uart_dev);

	return 0;
}

int dgus2_hal_tx_bytes(const struct device *dev, const uint8_t *data, size_t len) {
	if (!dev || !data || len == 0) {
		return -EINVAL;
	}

	const struct dgus2_config *config = dev->config;
	struct dgus2_data *dev_data = dev->data;

	if (!config || !dev_data || !config->uart_dev) {
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->tx_mutex, K_FOREVER);

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(config->uart_dev, data[i]);
	}

	k_mutex_unlock(&dev_data->tx_mutex);
	return 0;
}

int dgus2_hal_get_byte(const struct device *dev, uint8_t *byte, k_timeout_t timeout) {
	if (!dev || !byte) {
		return -EINVAL;
	}

	struct dgus2_data *data = dev->data;
	if (!data) {
		return -EINVAL;
	}

	int ret = k_sem_take(&data->rx_sem, timeout);
	if (ret < 0) {
		return ret; /* Timeout o abort */
	}

	uint32_t read = ring_buf_get(&data->rx_ringbuf, byte, 1);
	return (read == 1) ? 0 : -EAGAIN;
}
