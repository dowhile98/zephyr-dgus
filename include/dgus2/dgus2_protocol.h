/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    dgus2_protocol.h
 * @brief   Hardware-independent protocol layer: FSM frame parser, builder and CRC-16.
 * @ingroup dgus2_protocol
 */

#ifndef DGUS2_PROTOCOL_H_
#define DGUS2_PROTOCOL_H_

#include <dgus2/dgus2_types.h>

/**
 * @defgroup dgus2_protocol DGUS II Protocol Layer
 * @brief    Pure C99 protocol encoders, decoders, and finite state machine parser.
 * @{
 */

/**
 * @brief Finite state machine parser states for byte-by-byte frame reception.
 */
typedef enum {
	DGUS2_STATE_WAIT_HEADER1, /**< Waiting for first sync byte (0x5A). */
	DGUS2_STATE_WAIT_HEADER2, /**< Waiting for second sync byte (0xA5). */
	DGUS2_STATE_WAIT_LEN, /**< Waiting for payload length byte. */
	DGUS2_STATE_WAIT_CMD, /**< Waiting for command byte. */
	DGUS2_STATE_WAIT_VP_MSB, /**< Waiting for VP address MSB byte. */
	DGUS2_STATE_WAIT_VP_LSB, /**< Waiting for VP address LSB byte. */
	DGUS2_STATE_WAIT_WORD_COUNT, /**< Waiting for word count (read response). */
	DGUS2_STATE_WAIT_DATA /**< Collecting payload data bytes. */
} dgus2_fsm_state_t;

/**
 * @brief FSM frame parser state container.
 */
struct dgus2_parser {
	dgus2_fsm_state_t state; /**< Current parser FSM state. */
	uint8_t rx_len; /**< Total frame length expected according to length byte. */
	uint8_t bytes_read; /**< Number of payload bytes received so far. */
	struct dgus2_frame current_frame; /**< Frame being actively assembled. */
	uint32_t parse_errors; /**< Cumulative count of parsing errors. */
};

/**
 * @brief Initializes or resets the protocol parser state machine.
 *
 * @param[in,out] parser Pointer to the parser instance.
 */
void dgus2_protocol_parser_init(struct dgus2_parser *parser);

/**
 * @brief Ingests a single byte into the parser FSM and updates state.
 *
 * @param[in,out] parser    Pointer to the parser state container.
 * @param[in]     byte      Byte received over the UART interface.
 * @param[out]    out_frame Destination buffer where completed frame is copied.
 *
 * @return true if a complete and valid frame was assembled, false otherwise.
 *
 * @note Reentrant, hardware-independent. Deterministic execution time per byte.
 */
bool dgus2_protocol_parse_byte(
    struct dgus2_parser *parser, uint8_t byte, struct dgus2_frame *out_frame);

/**
 * @brief Builds a VP write frame (Command 0x82).
 *
 * Wire format: 5A A5 [LEN] 82 [VP_MSB] [VP_LSB] [DATA...]
 *
 * @param[out] buf      Destination buffer for the serialized packet.
 * @param[in]  max_buf  Maximum capacity of @p buf in bytes.
 * @param[out] out_len  Pointer where total packet length in bytes will be stored.
 * @param[in]  vp_addr  Target DGUS II VP address.
 * @param[in]  data     Pointer to data payload bytes.
 * @param[in]  data_len Number of bytes in @p data.
 *
 * @retval 0       On success.
 * @retval -EINVAL If parameters are NULL or lengths are invalid.
 * @retval -ENOSPC If @p max_buf is insufficient to hold the frame.
 */
int dgus2_protocol_build_write_vp(uint8_t *buf, size_t max_buf, size_t *out_len, uint16_t vp_addr,
    const uint8_t *data, uint16_t data_len);

/**
 * @brief Builds a VP read request frame (Command 0x83).
 *
 * Wire format: 5A A5 04 83 [VP_MSB] [VP_LSB] [WORD_COUNT]
 *
 * @param[out] buf        Destination buffer for the serialized packet.
 * @param[in]  max_buf    Maximum capacity of @p buf in bytes.
 * @param[out] out_len    Pointer where total packet length will be stored.
 * @param[in]  vp_addr    Target DGUS II VP address to read.
 * @param[in]  word_count Number of 16-bit words to query.
 *
 * @retval 0       On success.
 * @retval -EINVAL If parameters are NULL or @p word_count is 0.
 * @retval -ENOSPC If @p max_buf is smaller than required 7 bytes.
 */
int dgus2_protocol_build_read_vp(
    uint8_t *buf, size_t max_buf, size_t *out_len, uint16_t vp_addr, uint8_t word_count);

/**
 * @brief Builds a system/OS register write frame (Command 0x80).
 *
 * Wire format: 5A A5 [LEN] 80 [REG_PAGE] [REG_ADDR] [DATA...]
 *
 * @param[out] buf      Destination buffer for the serialized packet.
 * @param[in]  max_buf  Maximum capacity of @p buf in bytes.
 * @param[out] out_len  Pointer where total packet length will be stored.
 * @param[in]  reg_page Register page identifier.
 * @param[in]  reg_addr Register address offset.
 * @param[in]  data     Pointer to data payload bytes.
 * @param[in]  data_len Number of payload bytes.
 *
 * @retval 0       On success.
 * @retval -EINVAL If parameters are NULL or invalid.
 * @retval -ENOSPC If @p max_buf is insufficient.
 */
int dgus2_protocol_build_write_reg(uint8_t *buf, size_t max_buf, size_t *out_len, uint8_t reg_page,
    uint8_t reg_addr, const uint8_t *data, uint16_t data_len);

/**
 * @brief Builds a system/OS register read request frame (Command 0x81).
 *
 * Wire format: 5A A5 04 81 [REG_PAGE] [REG_ADDR] [LEN]
 *
 * @param[out] buf      Destination buffer for the serialized packet.
 * @param[in]  max_buf  Maximum capacity of @p buf in bytes.
 * @param[out] out_len  Pointer where total packet length will be stored.
 * @param[in]  reg_page Register page identifier.
 * @param[in]  reg_addr Register address offset.
 * @param[in]  len      Number of bytes to read.
 *
 * @retval 0       On success.
 * @retval -EINVAL If parameters are NULL or invalid.
 * @retval -ENOSPC If @p max_buf is smaller than 7 bytes.
 */
int dgus2_protocol_build_read_reg(
    uint8_t *buf, size_t max_buf, size_t *out_len, uint8_t reg_page, uint8_t reg_addr, uint8_t len);

/**
 * @brief Builds a real-time trend curve write frame (Command 0x84).
 *
 * Wire format: 5A A5 [LEN] 84 [CHANNEL] [POINTS_BE16...]
 *
 * @param[out] buf      Destination buffer for the serialized packet.
 * @param[in]  max_buf  Maximum capacity of @p buf in bytes.
 * @param[out] out_len  Pointer where total packet length will be stored.
 * @param[in]  channel  Curve channel index (0x00 .. 0x07).
 * @param[in]  points   Array of 16-bit point values.
 * @param[in]  count    Number of points in @p points array.
 *
 * @retval 0       On success.
 * @retval -EINVAL If parameters are NULL, @p count is 0, or payload bounds exceeded.
 * @retval -ENOSPC If @p max_buf is insufficient.
 */
int dgus2_protocol_build_write_curve(uint8_t *buf, size_t max_buf, size_t *out_len, uint8_t channel,
    const uint16_t *points, uint16_t count);

/**
 * @brief Calculates CRC-16 Modbus (polynomial 0xA001, initial value 0xFFFF).
 *
 * Used for DGUS II extended packet verification when CRC mode is enabled.
 *
 * @param[in] data Pointer to the buffer over which to calculate CRC-16.
 * @param[in] len  Length of data buffer in bytes.
 *
 * @return Calculated 16-bit CRC value (0 if @p data is NULL or @p len is 0).
 */
uint16_t dgus2_protocol_crc16(const uint8_t *data, size_t len);

/** @} */

#endif /* DGUS2_PROTOCOL_H_ */
