/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    dgus2_types.h
 * @brief   Data types, frame definitions, and callbacks for DGUS II driver.
 * @ingroup dgus2_api
 */

#ifndef ZEPHYR_MODULES_DWIN_DGUS2_TYPES_H_
#define ZEPHYR_MODULES_DWIN_DGUS2_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** @brief Frame start delimiter byte 1 (0x5A). */
#define DGUS2_HEADER1 0x5A

/** @brief Frame start delimiter byte 2 (0xA5). */
#define DGUS2_HEADER2 0xA5

/** @brief Maximum payload capacity in bytes for a single DGUS II packet. */
#define DGUS2_MAX_PAYLOAD_LEN 250

#ifndef DGUS2_VP_SYSTEM_PAGE
/** @brief System register for active page control/status. */
#define DGUS2_VP_SYSTEM_PAGE 0x0084
#endif

/**
 * @brief DGUS II command codes (T5L Guide V2.921).
 */
typedef enum {
	DGUS2_CMD_WRITE_REG = 0x80, /**< Write System/OS Register (0x80) */
	DGUS2_CMD_READ_REG = 0x81, /**< Read System/OS Register (0x81) */
	DGUS2_CMD_WRITE_VP = 0x82, /**< Write Variable Pointer (0x82) */
	DGUS2_CMD_READ_VP = 0x83, /**< Read Variable Pointer (0x83) */
	DGUS2_CMD_WRITE_CURVE = 0x84, /**< Real-time Trend Curve write (0x84) */
} dgus2_cmd_t;

/**
 * @brief Parsed DGUS II communication frame representation.
 */
struct dgus2_frame {
	uint8_t cmd; /**< Command opcode (0x80..0x84). */
	uint16_t vp_addr; /**< Target VP or register address. */
	uint8_t word_count; /**< Requested word count (for read commands). */
	bool is_ack; /**< True if frame represents an ACK or response. */
	uint8_t data[DGUS2_MAX_PAYLOAD_LEN]; /**< Raw payload bytes. */
	uint16_t data_len; /**< Number of valid bytes in payload. */
};

/**
 * @brief Callback signature for asynchronous VP notification events (touch events).
 *
 * @param[in] vp_addr   VP address where event occurred.
 * @param[in] data      Pointer to payload bytes received from display.
 * @param[in] len       Length of payload in bytes.
 * @param[in] user_data User-provided context pointer.
 */
typedef void (*dgus2_vp_event_cb_t)(
    uint16_t vp_addr, const uint8_t *data, uint16_t len, void *user_data);

/**
 * @brief Registered subscriber record for VP event routing.
 */
struct dgus2_vp_subscriber {
	uint16_t vp_addr; /**< VP address monitored by this subscriber. */
	dgus2_vp_event_cb_t cb; /**< User callback function pointer. */
	void *user_data; /**< Context pointer passed to callback. */
};

#endif /* ZEPHYR_MODULES_DWIN_DGUS2_TYPES_H_ */
