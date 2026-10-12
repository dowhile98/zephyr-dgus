/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * Declaraciones internas de la Capa HAL.
 */

#ifndef DGUS2_HAL_H_
#define DGUS2_HAL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <stddef.h>
#include <stdint.h>

int dgus2_hal_init(const struct device *dev);
int dgus2_hal_tx_bytes(const struct device *dev, const uint8_t *data, size_t len);
int dgus2_hal_get_byte(const struct device *dev, uint8_t *byte, k_timeout_t timeout);

#endif /* DGUS2_HAL_H_ */
