/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver Canónico Multi-Instancia DGUS II (DWIN T5L) para Zephyr RTOS.
 */

#include <dgus2/dgus2.h>
#include <dgus2/dgus2_hal.h>
#include <dgus2/dgus2_protocol.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include "dgus2_internal.h"
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(dgus2, CONFIG_DGUS2_LOG_LEVEL);

#define DT_DRV_COMPAT dwin_dgus2

/*
 * Despacho de tramas y procesamiento en hilo receptor
 */
static void dgus2_dispatch_frame(const struct device *dev, const struct dgus2_frame *frame) {
	if (!dev || !frame) {
		return;
	}

	struct dgus2_data *data = dev->data;

	/* 1. Verificar si corresponde a una respuesta a lectura síncrona pendiente */
	k_mutex_lock(&data->sync_data_mutex, K_FOREVER);
	if (data->sync_target_vp != 0xFFFF && frame->vp_addr == data->sync_target_vp) {
		data->sync_frame = *frame;
		k_sem_give(&data->sync_sem);
	}
	k_mutex_unlock(&data->sync_data_mutex);

	/* 2. Notificar a suscriptores asíncronos (Eventos Touch / VP Updates)
	 * Se copian los suscriptores coincidentes para invocar los callbacks fuera del mutex,
	 * evitando deadlocks recursivos si el callback interactúa con la API de DGUS.
	 */
	struct dgus2_vp_subscriber matched_subs[CONFIG_DGUS2_MAX_SUBSCRIBERS];
	size_t match_count = 0;

	k_mutex_lock(&data->sub_mutex, K_FOREVER);
	for (size_t i = 0; i < data->subscriber_count; i++) {
		if (data->subscribers[i].vp_addr == frame->vp_addr && data->subscribers[i].cb) {
			matched_subs[match_count++] = data->subscribers[i];
		}
	}
	k_mutex_unlock(&data->sub_mutex);

	for (size_t i = 0; i < match_count; i++) {
		matched_subs[i].cb(
		    frame->vp_addr, frame->data, frame->data_len, matched_subs[i].user_data);
	}
}

static void dgus2_rx_thread(void *p1, void *p2, void *p3) {
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = (const struct device *)p1;
	struct dgus2_data *data = dev->data;

	dgus2_protocol_parser_init(&data->parser);

	while (1) {
		uint8_t byte;
		int ret = dgus2_hal_get_byte(dev, &byte, K_FOREVER);
		if (ret == 0) {
			struct dgus2_frame frame;
			if (dgus2_protocol_parse_byte(&data->parser, byte, &frame)) {
				dgus2_dispatch_frame(dev, &frame);
			}
		}
	}
}

/*
 * API Pública Canónica Multi-Instancia
 */
int dgus2_init(const struct device *dev) {
	if (!dev) {
		return -EINVAL;
	}

	struct dgus2_data *data = dev->data;
	const struct dgus2_config *config = dev->config;

	if (!data || !config) {
		return -EINVAL;
	}

	if (data->initialized) {
		return 0;
	}

	int ret = dgus2_hal_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize DGUS2 HAL: %d", ret);
		return ret;
	}

	dgus2_protocol_parser_init(&data->parser);

	k_mutex_init(&data->sub_mutex);
	k_mutex_init(&data->sync_req_mutex);
	k_mutex_init(&data->sync_data_mutex);
	k_sem_init(&data->sync_sem, 0, 1);
	data->sync_target_vp = 0xFFFF;
	data->subscriber_count = 0;

	data->rx_tid = k_thread_create(&data->rx_thread, data->rx_stack,
	    K_KERNEL_STACK_SIZEOF(data->rx_stack), dgus2_rx_thread, (void *)dev, NULL, NULL,
	    CONFIG_DGUS2_RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	k_thread_name_set(&data->rx_thread, "dgus2_rx");
	data->initialized = true;

	return 0;
}

int dgus2_write_vp_bytes(
    const struct device *dev, uint16_t vp_addr, const uint8_t *data, uint16_t len) {
	if (!dev || !data) {
		return -EINVAL;
	}

	uint8_t tx_buf[DGUS2_MAX_PAYLOAD_LEN + 10];
	size_t tx_len = 0;

	int ret =
	    dgus2_protocol_build_write_vp(tx_buf, sizeof(tx_buf), &tx_len, vp_addr, data, len);
	if (ret < 0) {
		return ret;
	}

	return dgus2_hal_tx_bytes(dev, tx_buf, tx_len);
}

int dgus2_write_vp_u16(const struct device *dev, uint16_t vp_addr, uint16_t value) {
	if (!dev) {
		return -EINVAL;
	}

	uint8_t payload[2];
	sys_put_be16(value, payload);
	return dgus2_write_vp_bytes(dev, vp_addr, payload, sizeof(payload));
}

int dgus2_write_vp_u32(const struct device *dev, uint16_t vp_addr, uint32_t value) {
	if (!dev) {
		return -EINVAL;
	}

	uint8_t payload[4];
	sys_put_be32(value, payload);
	return dgus2_write_vp_bytes(dev, vp_addr, payload, sizeof(payload));
}

int dgus2_write_vp_i16(const struct device *dev, uint16_t vp_addr, int16_t value) {
	if (!dev) {
		return -EINVAL;
	}

	uint8_t payload[2];
	sys_put_be16((uint16_t)value, payload);
	return dgus2_write_vp_bytes(dev, vp_addr, payload, sizeof(payload));
}

int dgus2_write_vp_i32(const struct device *dev, uint16_t vp_addr, int32_t value) {
	if (!dev) {
		return -EINVAL;
	}

	uint8_t payload[4];
	sys_put_be32((uint32_t)value, payload);
	return dgus2_write_vp_bytes(dev, vp_addr, payload, sizeof(payload));
}

int dgus2_write_vp_string(const struct device *dev, uint16_t vp_addr, const char *str) {
	if (!dev || !str) {
		return -EINVAL;
	}

	size_t len = strlen(str);
	if (len > DGUS2_MAX_PAYLOAD_LEN) {
		return -EINVAL;
	}

	uint8_t payload[DGUS2_MAX_PAYLOAD_LEN];
	memcpy(payload, str, len);

	size_t total_len = len;
	if (total_len + 2 <= sizeof(payload)) {
		payload[total_len++] = 0xFF;
		payload[total_len++] = 0xFF;
	}

	return dgus2_write_vp_bytes(dev, vp_addr, payload, (uint16_t)total_len);
}

int dgus2_read_vp_sync(const struct device *dev, uint16_t vp_addr, uint8_t *out_buf,
    uint16_t max_len, uint16_t *out_len, k_timeout_t timeout) {
	if (!dev || !out_buf || !out_len) {
		return -EINVAL;
	}

	struct dgus2_data *data = dev->data;
	if (!data) {
		return -EINVAL;
	}

	/* Proteger toda la transacción síncrona contra llamadas concurrentes */
	int ret = k_mutex_lock(&data->sync_req_mutex, timeout);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->sync_data_mutex, K_FOREVER);
	data->sync_target_vp = vp_addr;
	k_sem_reset(&data->sync_sem);
	k_mutex_unlock(&data->sync_data_mutex);

	/* Solicitar words según max_len */
	uint8_t word_count = (max_len + 1) / 2;
	if (word_count == 0) {
		word_count = 1;
	}

	uint8_t tx_buf[16];
	size_t tx_len = 0;
	ret = dgus2_protocol_build_read_vp(tx_buf, sizeof(tx_buf), &tx_len, vp_addr, word_count);
	if (ret < 0) {
		k_mutex_lock(&data->sync_data_mutex, K_FOREVER);
		data->sync_target_vp = 0xFFFF;
		k_mutex_unlock(&data->sync_data_mutex);
		k_mutex_unlock(&data->sync_req_mutex);
		return ret;
	}

	ret = dgus2_hal_tx_bytes(dev, tx_buf, tx_len);
	if (ret < 0) {
		k_mutex_lock(&data->sync_data_mutex, K_FOREVER);
		data->sync_target_vp = 0xFFFF;
		k_mutex_unlock(&data->sync_data_mutex);
		k_mutex_unlock(&data->sync_req_mutex);
		return ret;
	}

	/* Esperar la respuesta devuelta por la FSM en el hilo RX */
	ret = k_sem_take(&data->sync_sem, timeout);

	k_mutex_lock(&data->sync_data_mutex, K_FOREVER);
	if (ret < 0) {
		data->sync_target_vp = 0xFFFF;
		k_mutex_unlock(&data->sync_data_mutex);
		k_mutex_unlock(&data->sync_req_mutex);
		return -ETIMEDOUT;
	}

	uint16_t copy_len =
	    data->sync_frame.data_len > max_len ? max_len : data->sync_frame.data_len;
	memcpy(out_buf, data->sync_frame.data, copy_len);
	*out_len = copy_len;

	data->sync_target_vp = 0xFFFF;
	k_mutex_unlock(&data->sync_data_mutex);

	k_mutex_unlock(&data->sync_req_mutex);
	return 0;
}

int dgus2_read_vp_u16_sync(
    const struct device *dev, uint16_t vp_addr, uint16_t *value, k_timeout_t timeout) {
	if (!dev || !value) {
		return -EINVAL;
	}

	uint8_t buf[2];
	uint16_t rx_len = 0;
	int ret = dgus2_read_vp_sync(dev, vp_addr, buf, sizeof(buf), &rx_len, timeout);
	if (ret < 0) {
		return ret;
	}

	if (rx_len < 2) {
		return -EBADMSG;
	}

	*value = sys_get_be16(buf);
	return 0;
}

int dgus2_read_vp_u32_sync(
    const struct device *dev, uint16_t vp_addr, uint32_t *value, k_timeout_t timeout) {
	if (!dev || !value) {
		return -EINVAL;
	}

	uint8_t buf[4];
	uint16_t rx_len = 0;
	int ret = dgus2_read_vp_sync(dev, vp_addr, buf, sizeof(buf), &rx_len, timeout);
	if (ret < 0) {
		return ret;
	}

	if (rx_len < 4) {
		return -EBADMSG;
	}

	*value = sys_get_be32(buf);
	return 0;
}

int dgus2_set_page(const struct device *dev, uint16_t page_id) {
	if (!dev) {
		return -EINVAL;
	}

	/* DGUS II exige 4 bytes para cambio de página: 0x5A 0x01 [PAGE_MSB] [PAGE_LSB] */
	uint8_t payload[4];
	sys_put_be16(DGUS2_PAGE_SWITCH_MAGIC, &payload[0]);
	sys_put_be16(page_id, &payload[2]);
	return dgus2_write_vp_bytes(dev, DGUS2_VP_SYSTEM_PAGE, payload, sizeof(payload));
}

int dgus2_set_brightness(const struct device *dev, uint8_t brightness_pct) {
	if (!dev) {
		return -EINVAL;
	}

	if (brightness_pct > 100) {
		brightness_pct = 100;
	}

	/* VP 0x0082: 2 bytes = [Standby Brightness] [Active Brightness] */
	uint8_t payload[2];
	payload[0] = brightness_pct;
	payload[1] = brightness_pct;
	return dgus2_write_vp_bytes(dev, DGUS2_VP_SYSTEM_BRIGHT, payload, sizeof(payload));
}

int dgus2_get_page_sync(const struct device *dev, uint16_t *page_id, k_timeout_t timeout) {
	if (!dev || !page_id) {
		return -EINVAL;
	}

	uint8_t buf[4];
	uint16_t rx_len = 0;

	/* Consultar el VP 0x0014 o VP 0x0084 */
	int ret =
	    dgus2_read_vp_sync(dev, DGUS2_VP_CURRENT_PAGE, buf, sizeof(buf), &rx_len, timeout);
	if (ret < 0) {
		/* Intentar fallback leyendo VP 0x0084 */
		ret = dgus2_read_vp_sync(
		    dev, DGUS2_VP_SYSTEM_PAGE, buf, sizeof(buf), &rx_len, timeout);
		if (ret < 0) {
			return ret;
		}
	}

	if (rx_len >= 2) {
		*page_id = sys_get_be16(&buf[rx_len - 2]);
		return 0;
	}

	return -EBADMSG;
}

int dgus2_trigger_buzzer(const struct device *dev, uint16_t duration_ms) {
	if (!dev) {
		return -EINVAL;
	}

	/* DWIN T5L VP 0x00A0: Escala de tiempo en base de 8 ms (mínimo 1 si duration_ms > 0) */
	uint16_t units = duration_ms / 8;
	if (units == 0 && duration_ms > 0) {
		units = 1;
	}

	uint8_t payload[2];
	sys_put_be16(units, payload);
	return dgus2_write_vp_bytes(dev, DGUS2_VP_SYSTEM_BUZZER, payload, sizeof(payload));
}

int dgus2_write_curve(
    const struct device *dev, uint8_t channel, const uint16_t *points, uint16_t count) {
	if (!dev || !points || count == 0 || (count * 2) > (DGUS2_MAX_PAYLOAD_LEN - 4)) {
		return -EINVAL;
	}

	uint8_t tx_buf[DGUS2_MAX_PAYLOAD_LEN + 10];
	size_t tx_len = 0;

	int ret = dgus2_protocol_build_write_curve(
	    tx_buf, sizeof(tx_buf), &tx_len, channel, points, count);
	if (ret < 0) {
		return ret;
	}

	return dgus2_hal_tx_bytes(dev, tx_buf, tx_len);
}

int dgus2_register_vp_callback(
    const struct device *dev, uint16_t vp_addr, dgus2_vp_event_cb_t cb, void *user_data) {
	if (!dev || !cb) {
		return -EINVAL;
	}

	struct dgus2_data *data = dev->data;
	if (!data) {
		return -EINVAL;
	}

	k_mutex_lock(&data->sub_mutex, K_FOREVER);
	if (data->subscriber_count >= CONFIG_DGUS2_MAX_SUBSCRIBERS) {
		k_mutex_unlock(&data->sub_mutex);
		return -ENOMEM;
	}

	data->subscribers[data->subscriber_count].vp_addr = vp_addr;
	data->subscribers[data->subscriber_count].cb = cb;
	data->subscribers[data->subscriber_count].user_data = user_data;
	data->subscriber_count++;

	k_mutex_unlock(&data->sub_mutex);
	return 0;
}

int dgus2_unregister_vp_callback(
    const struct device *dev, uint16_t vp_addr, dgus2_vp_event_cb_t cb) {
	if (!dev || !cb) {
		return -EINVAL;
	}

	struct dgus2_data *data = dev->data;
	if (!data) {
		return -EINVAL;
	}

	bool found = false;
	k_mutex_lock(&data->sub_mutex, K_FOREVER);
	for (size_t i = 0; i < data->subscriber_count;) {
		if (data->subscribers[i].vp_addr == vp_addr && data->subscribers[i].cb == cb) {
			found = true;
			for (size_t j = i; j < data->subscriber_count - 1; j++) {
				data->subscribers[j] = data->subscribers[j + 1];
			}
			data->subscriber_count--;
		} else {
			i++;
		}
	}
	k_mutex_unlock(&data->sub_mutex);

	return found ? 0 : -ENOENT;
}

uint32_t dgus2_get_parse_error_count(const struct device *dev) {
	if (!dev) {
		return 0;
	}

	struct dgus2_data *data = dev->data;
	if (!data) {
		return 0;
	}

	return data->parser.parse_errors;
}

/*
 * Registro Canónico Multi-Instancia de Dispositivos Zephyr
 */
#define DGUS2_DEVICE_INIT(inst)                                                                    \
	static struct dgus2_data dgus2_data_##inst;                                                \
	static const struct dgus2_config dgus2_config_##inst = {                                   \
	    .uart_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, uart)),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, dgus2_init, NULL, &dgus2_data_##inst, &dgus2_config_##inst,    \
	    POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DGUS2_DEVICE_INIT)
