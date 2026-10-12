/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * Definiciones y estructuras internas privadas del driver DGUS II.
 */

#ifndef DGUS2_INTERNAL_H_
#define DGUS2_INTERNAL_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>
#include <dgus2/dgus2.h>
#include <dgus2/dgus2_protocol.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef CONFIG_DGUS2_RING_BUFFER_SIZE
#define CONFIG_DGUS2_RING_BUFFER_SIZE 512
#endif

#ifndef CONFIG_DGUS2_MAX_SUBSCRIBERS
#define CONFIG_DGUS2_MAX_SUBSCRIBERS 16
#endif

#ifndef CONFIG_DGUS2_RX_THREAD_STACK_SIZE
#define CONFIG_DGUS2_RX_THREAD_STACK_SIZE 2048
#endif

#ifndef CONFIG_DGUS2_RX_THREAD_PRIORITY
#define CONFIG_DGUS2_RX_THREAD_PRIORITY 5
#endif

#ifndef CONFIG_APPLICATION_INIT_PRIORITY
#define CONFIG_APPLICATION_INIT_PRIORITY 90
#endif

/*
 * Configuración inmutable por instancia (Devicetree / ROM)
 */
struct dgus2_config {
	const struct device *uart_dev;
};

/*
 * Estado mutable y primitivas de sincronización por instancia (RAM)
 */
struct dgus2_data {
	/* HAL / UART RX */
	struct ring_buf rx_ringbuf;
	uint8_t rx_ringbuf_data[CONFIG_DGUS2_RING_BUFFER_SIZE];
	struct k_sem rx_sem;
	struct k_mutex tx_mutex;

	/* Parser FSM */
	struct dgus2_parser parser;

	/* RX Thread */
	struct k_thread rx_thread;
	k_tid_t rx_tid;
	K_KERNEL_STACK_MEMBER(rx_stack, CONFIG_DGUS2_RX_THREAD_STACK_SIZE);

	/* Subscribers */
	struct dgus2_vp_subscriber subscribers[CONFIG_DGUS2_MAX_SUBSCRIBERS];
	size_t subscriber_count;
	struct k_mutex sub_mutex;

	/* Sync read channel */
	uint16_t sync_target_vp;
	struct dgus2_frame sync_frame;
	struct k_sem sync_sem;
	struct k_mutex sync_req_mutex;
	struct k_mutex sync_data_mutex;

	/* Driver state */
	bool initialized;
};

#endif /* DGUS2_INTERNAL_H_ */
