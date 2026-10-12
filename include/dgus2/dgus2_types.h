/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tipos de datos, enums y estructuras para el módulo DGUS II (DWIN T5L).
 */

#ifndef ZEPHYR_MODULES_DWIN_DGUS2_TYPES_H_
#define ZEPHYR_MODULES_DWIN_DGUS2_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define DGUS2_HEADER1             0x5A
#define DGUS2_HEADER2             0xA5
#define DGUS2_MAX_PAYLOAD_LEN     250

/* Direcciones VP especiales del sistema DGUS */
#define DGUS2_VP_SYSTEM_PAGE      0x0084  /* Cambio de página activa */

/* Comandos DGUS II (T5L Guide V2.921) */
typedef enum {
	DGUS2_CMD_WRITE_REG   = 0x80,
	DGUS2_CMD_READ_REG    = 0x81,
	DGUS2_CMD_WRITE_VP    = 0x82,
	DGUS2_CMD_READ_VP     = 0x83,
	DGUS2_CMD_WRITE_CURVE = 0x84,
} dgus2_cmd_t;

/* Estructura de Trama DGUS II Desensamblada */
struct dgus2_frame {
	uint8_t cmd;
	uint16_t vp_addr;
	uint8_t word_count;
	bool is_ack;
	uint8_t data[DGUS2_MAX_PAYLOAD_LEN];
	uint16_t data_len;
};

/* Callback para Notificaciones Asíncronas (Eventos Touch / VP Updates) */
typedef void (*dgus2_vp_event_cb_t)(uint16_t vp_addr, const uint8_t *data, uint16_t len, void *user_data);

/* Registro de Suscriptor a Eventos de VP */
struct dgus2_vp_subscriber {
	uint16_t vp_addr;
	dgus2_vp_event_cb_t cb;
	void *user_data;
};

#endif /* ZEPHYR_MODULES_DWIN_DGUS2_TYPES_H_ */
