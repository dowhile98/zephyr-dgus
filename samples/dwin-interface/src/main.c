/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <dgus2/dgus2.h>

LOG_MODULE_REGISTER(dwin_sample, LOG_LEVEL_INF);

#if !DT_NODE_EXISTS(DT_CHOSEN(zephyr_dgus))
#error "zephyr,dgus chosen node not defined in devicetree overlay"
#endif

/*
 * Callback para eventos táctiles (VP 0x3000)
 */
static void on_touch_button_press(uint16_t vp_addr, const uint8_t *data, uint16_t len, void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("Touch event received on VP: 0x%04X, len: %u", vp_addr, len);

	if (data != NULL && len >= 2) {
		uint16_t key_val = ((uint16_t)data[0] << 8) | data[1];
		LOG_INF("Button key code: 0x%04X (%u)", key_val, key_val);
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_dgus));
	int ret;

	LOG_INF("==================================================");
	LOG_INF("  DWIN DGUS II (T5L) Intelligent Display Sample   ");
	LOG_INF("==================================================");

	/* 1. Verificación del dispositivo */
	if (!device_is_ready(dev)) {
		LOG_ERR("Device %s is not ready!", dev->name);
		return -ENODEV;
	}
	LOG_INF("Display device '%s' is ready", dev->name);

	/* 2. Registro de callback para eventos táctiles */
	ret = dgus2_register_vp_callback(dev, 0x3000, on_touch_button_press, NULL);
	if (ret < 0) {
		LOG_WRN("Failed to register touch callback on VP 0x3000: %d", ret);
	} else {
		LOG_INF("Touch callback registered on VP 0x3000");
	}

	/* 3. Secuencia de inicio de la pantalla */
	LOG_INF("Starting display initialization sequence...");

	/* Cambio a página 0 */
	ret = dgus2_set_page(dev, 0);
	if (ret < 0) {
		LOG_WRN("dgus2_set_page failed: %d", ret);
	} else {
		LOG_INF("Active page set to 0");
	}

	/* Brillo al 85% */
	ret = dgus2_set_brightness(dev, 85);
	if (ret < 0) {
		LOG_WRN("dgus2_set_brightness failed: %d", ret);
	} else {
		LOG_INF("Backlight brightness set to 85%%");
	}

	/* Beep de confirmación de 100 ms */
	ret = dgus2_trigger_buzzer(dev, 100);
	if (ret < 0) {
		LOG_WRN("dgus2_trigger_buzzer failed: %d", ret);
	} else {
		LOG_INF("Buzzer triggered for 100 ms");
	}

	/* Texto de bienvenida en VP 0x2000 */
	ret = dgus2_write_vp_string(dev, 0x2000, "Zephyr RTOS DGUS II");
	if (ret < 0) {
		LOG_WRN("dgus2_write_vp_string failed: %d", ret);
	} else {
		LOG_INF("Welcome text written to VP 0x2000");
	}

	/* Lectura síncrona de página actual con timeout */
	uint16_t current_page = 0;
	ret = dgus2_get_page_sync(dev, &current_page, K_MSEC(500));
	if (ret < 0) {
		LOG_INF("Sync page query timeout/unanswered (normal if no physical display attached): %d", ret);
	} else {
		LOG_INF("Sync page confirmed: %u", current_page);
	}

	/* 4. Bucle de control periódico con telemetría */
	LOG_INF("Entering periodic telemetry update loop...");

	int16_t temp_celsius_x10 = 245; /* 24.5 °C */
	uint16_t pressure_hpa = 1013;   /* 1013 hPa */
	uint32_t uptime_sec = 0;
	uint16_t curve_base = 100;

	while (1) {
		uptime_sec++;

		/* Simulación de fluctuaciones de sensores */
		temp_celsius_x10 = 240 + (int16_t)(uptime_sec % 15);
		pressure_hpa = 1010 + (uint16_t)(uptime_sec % 7);

		/* Telemetría a VP registers */
		dgus2_write_vp_i16(dev, 0x1000, temp_celsius_x10);
		dgus2_write_vp_u16(dev, 0x1002, pressure_hpa);
		dgus2_write_vp_u32(dev, 0x1004, uptime_sec);

		/* Trazado de puntos de curva en tiempo real en canal 0 */
		uint16_t curve_pts[4] = {
			(uint16_t)(curve_base + 10),
			(uint16_t)(curve_base + 25),
			(uint16_t)(curve_base - 10),
			(uint16_t)(curve_base + 5),
		};
		dgus2_write_curve(dev, 0, curve_pts, 4);
		curve_base = (curve_base + 15) % 400;

		/* Diagnóstico de salud del bus y parser */
		uint32_t parse_errors = dgus2_get_parse_error_count(dev);

		LOG_INF("Telemetry: Uptime=%us, Temp=%d.%d C, Press=%u hPa | Parse Errors=%u",
			uptime_sec,
			temp_celsius_x10 / 10,
			(temp_celsius_x10 < 0 ? -temp_celsius_x10 : temp_celsius_x10) % 10,
			pressure_hpa,
			parse_errors);

		k_sleep(K_SECONDS(1));
	}

	return 0;
}
