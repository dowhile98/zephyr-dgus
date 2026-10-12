/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implementación de la Capa de Protocolo (FSM Parser + Frame Builder).
 */

#include <dgus2/dgus2_protocol.h>
#include <string.h>
#include <errno.h>

void dgus2_protocol_parser_init(struct dgus2_parser *parser)
{
	if (!parser) {
		return;
	}

	memset(parser, 0, sizeof(*parser));
	parser->state = DGUS2_STATE_WAIT_HEADER1;
}

bool dgus2_protocol_parse_byte(struct dgus2_parser *parser, uint8_t byte,
			       struct dgus2_frame *out_frame)
{
	if (!parser || !out_frame) {
		return false;
	}

	switch (parser->state) {
	case DGUS2_STATE_WAIT_HEADER1:
		if (byte == DGUS2_HEADER1) {
			parser->state = DGUS2_STATE_WAIT_HEADER2;
		}
		break;

	case DGUS2_STATE_WAIT_HEADER2:
		if (byte == DGUS2_HEADER2) {
			parser->state = DGUS2_STATE_WAIT_LEN;
		} else if (byte != DGUS2_HEADER1) {
			parser->state = DGUS2_STATE_WAIT_HEADER1;
			parser->parse_errors++;
		}
		break;

	case DGUS2_STATE_WAIT_LEN:
		if (byte < 3 || byte > (DGUS2_MAX_PAYLOAD_LEN + 3)) {
			/* Longitud de datos invalida (debe contener al menos CMD + VP_MSB + VP_LSB
			 * o CMD + ACK_OK) */
			parser->state = DGUS2_STATE_WAIT_HEADER1;
			parser->parse_errors++;
		} else {
			parser->rx_len = byte;
			parser->bytes_read = 0;
			memset(&parser->current_frame, 0, sizeof(parser->current_frame));
			parser->state = DGUS2_STATE_WAIT_CMD;
		}
		break;

	case DGUS2_STATE_WAIT_CMD:
		parser->current_frame.cmd = byte;
		parser->bytes_read++;
		parser->state = DGUS2_STATE_WAIT_VP_MSB;
		break;

	case DGUS2_STATE_WAIT_VP_MSB:
		parser->current_frame.vp_addr = ((uint16_t)byte) << 8;
		parser->bytes_read++;
		parser->state = DGUS2_STATE_WAIT_VP_LSB;
		break;

	case DGUS2_STATE_WAIT_VP_LSB:
		parser->current_frame.vp_addr |= byte;
		parser->bytes_read++;

		if (parser->bytes_read >= parser->rx_len) {
			/* Trama de longitud 3 (CMD + 2 bytes):
			 * Comprobar si es ACK ('OK' = 0x4F4B) */
			if ((parser->current_frame.cmd == DGUS2_CMD_WRITE_VP ||
			     parser->current_frame.cmd == DGUS2_CMD_WRITE_REG ||
			     parser->current_frame.cmd == DGUS2_CMD_WRITE_CURVE) &&
			    (parser->current_frame.vp_addr == 0x4F4B)) {
				parser->current_frame.is_ack = true;
				parser->current_frame.vp_addr = 0;
			}
			parser->current_frame.data_len = 0;
			*out_frame = parser->current_frame;
			parser->state = DGUS2_STATE_WAIT_HEADER1;
			return true;
		}

		if (parser->current_frame.cmd == DGUS2_CMD_READ_VP) {
			parser->state = DGUS2_STATE_WAIT_WORD_COUNT;
		} else {
			parser->current_frame.data_len = 0;
			parser->state = DGUS2_STATE_WAIT_DATA;
		}
		break;

	case DGUS2_STATE_WAIT_WORD_COUNT:
		parser->current_frame.word_count = byte;
		parser->bytes_read++;

		if (parser->bytes_read >= parser->rx_len) {
			parser->current_frame.data_len = 0;
			*out_frame = parser->current_frame;
			parser->state = DGUS2_STATE_WAIT_HEADER1;
			return true;
		}

		parser->current_frame.data_len = 0;
		parser->state = DGUS2_STATE_WAIT_DATA;
		break;

	case DGUS2_STATE_WAIT_DATA:
		if (parser->current_frame.data_len < DGUS2_MAX_PAYLOAD_LEN) {
			parser->current_frame.data[parser->current_frame.data_len++] = byte;
		}
		parser->bytes_read++;

		if (parser->bytes_read >= parser->rx_len) {
			*out_frame = parser->current_frame;
			parser->state = DGUS2_STATE_WAIT_HEADER1;
			return true;
		}
		break;

	default:
		parser->state = DGUS2_STATE_WAIT_HEADER1;
		break;
	}

	return false;
}

int dgus2_protocol_build_write_vp(uint8_t *buf, size_t max_buf, size_t *out_len, uint16_t vp_addr,
				  const uint8_t *data, uint16_t data_len)
{
	if (!buf || !out_len || (data_len > 0 && !data)) {
		return -EINVAL;
	}

	/* Trama de salida: 5A A5 [LEN] [82] [VP_MSB] [VP_LSB] [DATA...] */
	size_t total_len = 6 + data_len;
	if (total_len > max_buf) {
		return -ENOBUFS;
	}

	buf[0] = DGUS2_HEADER1;
	buf[1] = DGUS2_HEADER2;
	buf[2] = (uint8_t)(3 + data_len); /* LEN = CMD(1) + VP(2) + DATA */
	buf[3] = DGUS2_CMD_WRITE_VP;
	buf[4] = (uint8_t)(vp_addr >> 8);   /* VP MSB */
	buf[5] = (uint8_t)(vp_addr & 0xFF); /* VP LSB */

	if (data_len > 0) {
		memcpy(&buf[6], data, data_len);
	}

	*out_len = total_len;
	return 0;
}

int dgus2_protocol_build_read_vp(uint8_t *buf, size_t max_buf, size_t *out_len, uint16_t vp_addr,
				 uint8_t word_count)
{
	if (!buf || !out_len || word_count == 0) {
		return -EINVAL;
	}

	/* Trama de lectura: 5A A5 [04] [83] [VP_MSB] [VP_LSB] [WORD_COUNT] */
	size_t total_len = 7;
	if (total_len > max_buf) {
		return -ENOBUFS;
	}

	buf[0] = DGUS2_HEADER1;
	buf[1] = DGUS2_HEADER2;
	buf[2] = 0x04; /* LEN = CMD(1) + VP(2) + WORD_COUNT(1) */
	buf[3] = DGUS2_CMD_READ_VP;
	buf[4] = (uint8_t)(vp_addr >> 8);
	buf[5] = (uint8_t)(vp_addr & 0xFF);
	buf[6] = word_count;

	*out_len = total_len;
	return 0;
}

int dgus2_protocol_build_write_reg(uint8_t *buf, size_t max_buf, size_t *out_len, uint8_t reg_page,
				   uint8_t reg_addr, const uint8_t *data, uint16_t data_len)
{
	if (!buf || !out_len || (data_len > 0 && !data)) {
		return -EINVAL;
	}

	/* Trama: 5A A5 [LEN] 80 [REG_PAGE] [REG_ADDR] [DATA...]
	 * LEN = CMD(1) + REG_PAGE(1) + REG_ADDR(1) + DATA_LEN = 3 + data_len */
	size_t total_len = 6 + data_len; /* 5A A5 LEN CMD PAGE ADDR = 6 */
	if (total_len > max_buf) {
		return -ENOBUFS;
	}

	buf[0] = DGUS2_HEADER1;
	buf[1] = DGUS2_HEADER2;
	buf[2] = (uint8_t)(3 + data_len); /* LEN */
	buf[3] = DGUS2_CMD_WRITE_REG;
	buf[4] = reg_page;
	buf[5] = reg_addr;

	if (data_len > 0) {
		memcpy(&buf[6], data, data_len);
	}

	*out_len = total_len;
	return 0;
}

int dgus2_protocol_build_read_reg(uint8_t *buf, size_t max_buf, size_t *out_len, uint8_t reg_page,
				  uint8_t reg_addr, uint8_t len)
{
	if (!buf || !out_len || len == 0) {
		return -EINVAL;
	}

	/* Trama: 5A A5 04 81 [REG_PAGE] [REG_ADDR] [LEN] */
	size_t total_len = 7;
	if (total_len > max_buf) {
		return -ENOBUFS;
	}

	buf[0] = DGUS2_HEADER1;
	buf[1] = DGUS2_HEADER2;
	buf[2] = 0x04; /* LEN = CMD(1) + REG_PAGE(1) + REG_ADDR(1) + LEN(1) */
	buf[3] = DGUS2_CMD_READ_REG;
	buf[4] = reg_page;
	buf[5] = reg_addr;
	buf[6] = len;

	*out_len = total_len;
	return 0;
}

int dgus2_protocol_build_write_curve(uint8_t *buf, size_t max_buf, size_t *out_len, uint8_t channel,
				     const uint16_t *points, uint16_t count)
{
	if (!buf || !out_len || !points || count == 0) {
		return -EINVAL;
	}

	/* Trama: 5A A5 [LEN] 84 [CHANNEL] [POINTS_BE16...]
	 * LEN = CMD(1) + CHANNEL(1) + 2 * count = 2 + 2 * count */
	size_t total_len = 5 + (size_t)count * 2;
	if (total_len > max_buf) {
		return -ENOBUFS;
	}

	buf[0] = DGUS2_HEADER1;
	buf[1] = DGUS2_HEADER2;
	buf[2] = (uint8_t)(2 + count * 2);
	buf[3] = DGUS2_CMD_WRITE_CURVE;
	buf[4] = channel;

	for (uint16_t i = 0; i < count; i++) {
		buf[5 + i * 2] = (uint8_t)(points[i] >> 8);
		buf[5 + i * 2 + 1] = (uint8_t)(points[i] & 0xFF);
	}

	*out_len = total_len;
	return 0;
}

uint16_t dgus2_protocol_crc16(const uint8_t *data, size_t len)
{
	if (!data || len == 0) {
		return 0;
	}

	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int j = 0; j < 8; j++) {
			if (crc & 0x0001) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}

	return crc;
}
