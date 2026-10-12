/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * Capa de Protocolo: Máquina de estados FSM parser y formateador de tramas DGUS II.
 */

#ifndef DGUS2_PROTOCOL_H_
#define DGUS2_PROTOCOL_H_

#include <dgus2/dgus2_types.h>

/* Estados FSM del Parser de Tramas DGUS II */
typedef enum {
	DGUS2_STATE_WAIT_HEADER1,
	DGUS2_STATE_WAIT_HEADER2,
	DGUS2_STATE_WAIT_LEN,
	DGUS2_STATE_WAIT_CMD,
	DGUS2_STATE_WAIT_VP_MSB,
	DGUS2_STATE_WAIT_VP_LSB,
	DGUS2_STATE_WAIT_WORD_COUNT,
	DGUS2_STATE_WAIT_DATA
} dgus2_fsm_state_t;

/* Instancia del Parser */
struct dgus2_parser {
	dgus2_fsm_state_t state;
	uint8_t rx_len;
	uint8_t bytes_read;
	struct dgus2_frame current_frame;
	uint32_t parse_errors;
};

void dgus2_protocol_parser_init(struct dgus2_parser *parser);

/**
 * @brief Procesa un byte entrante y hace avanzar la FSM del parser.
 *
 * @param parser Estado actual del parser
 * @param byte Byte recibido por la UART
 * @param out_frame Puntero donde se copiará la trama al completarse
 * @return true si se ha completado una trama válida.
 */
bool dgus2_protocol_parse_byte(struct dgus2_parser *parser, uint8_t byte, struct dgus2_frame *out_frame);

/**
 * @brief Construye una trama de escritura de VP (Command 0x82).
 */
int dgus2_protocol_build_write_vp(uint8_t *buf, size_t max_buf, size_t *out_len, uint16_t vp_addr, const uint8_t *data, uint16_t data_len);

/**
 * @brief Construye una trama de lectura de VP (Command 0x83).
 */
int dgus2_protocol_build_read_vp(uint8_t *buf, size_t max_buf, size_t *out_len, uint16_t vp_addr, uint8_t word_count);

/**
 * @brief Construye una trama de escritura de Registro (Command 0x80).
 *
 * Formato: 5A A5 [LEN] 80 [REG_PAGE] [REG_ADDR] [DATA...]
 * Usado para registros de sistema y de OS (0x00-0xFF): backlight, RTC, config.
 */
int dgus2_protocol_build_write_reg(uint8_t *buf, size_t max_buf, size_t *out_len,
				   uint8_t reg_page, uint8_t reg_addr,
				   const uint8_t *data, uint16_t data_len);

/**
 * @brief Construye una trama de lectura de Registro (Command 0x81).
 *
 * Formato: 5A A5 04 81 [REG_PAGE] [REG_ADDR] [LEN]
 */
int dgus2_protocol_build_read_reg(uint8_t *buf, size_t max_buf, size_t *out_len,
				  uint8_t reg_page, uint8_t reg_addr, uint8_t len);

/**
 * @brief Construye una trama de escritura de Curva de Tendencia (Command 0x84).
 *
 * Formato: 5A A5 [LEN] 84 [CHANNEL] [POINTS_BE16...]
 */
int dgus2_protocol_build_write_curve(uint8_t *buf, size_t max_buf, size_t *out_len,
				     uint8_t channel, const uint16_t *points, uint16_t count);

/**
 * @brief Calcula el CRC-16 Modbus (polinomio 0xA001, valor inicial 0xFFFF).
 *
 * @param data Puntero al buffer de datos
 * @param len Longitud en bytes
 * @return Valor CRC-16 calculado (0 si data es NULL o len es 0)
 */
uint16_t dgus2_protocol_crc16(const uint8_t *data, size_t len);

#endif /* DGUS2_PROTOCOL_H_ */
