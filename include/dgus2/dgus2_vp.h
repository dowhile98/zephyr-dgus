/*
 * Copyright (c) 2026 StructCore Embedded
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file    dgus2_vp.h
 * @brief   DGUS II Variable Pointer (VP) addresses and system register definitions.
 * @ingroup dgus2_sys_regs
 */

#ifndef ZEPHYR_MODULES_DWIN_DGUS2_VP_H_
#define ZEPHYR_MODULES_DWIN_DGUS2_VP_H_

/**
 * @defgroup dgus2_sys_regs DGUS II System Registers and Addresses
 * @ingroup  dgus2_api
 * @brief    System register VP addresses and protocol magic constants for DWIN DGUS II / T5L.
 * @{
 */

/** @brief Magic code required by DGUS II firmware for page switching (0x5A01). */
#define DGUS2_PAGE_SWITCH_MAGIC 0x5A01

/** @brief Maximum addressable page number in DGUS II architecture. */
#define DGUS2_PAGE_MAX 0xFFFF

/* ---- System Registers (Command 0x80 / 0x81 or VP 0x0080-0x008F) ---- */

/** @brief General system configuration register (0x0080). */
#define DGUS2_VP_SYSTEM_CONFIG 0x0080

/** @brief Internal RTC register (YY:MM:DD:WW:HH:MM:SS) (0x009C). */
#define DGUS2_VP_SYSTEM_RTC 0x009C

/** @brief Backlight brightness register (Standby % / Active %) (0x0082). */
#define DGUS2_VP_SYSTEM_BRIGHT 0x0082

#ifndef DGUS2_VP_SYSTEM_PAGE
/** @brief Active page switch/status register (4 bytes: 0x5A01 + PageID) (0x0084). */
#define DGUS2_VP_SYSTEM_PAGE 0x0084
#endif

/** @brief Direct current page ID read register (Register 0x14). */
#define DGUS2_VP_CURRENT_PAGE 0x0014

/** @brief Internal buzzer control register (Time in 8ms units) (0x00A0). */
#define DGUS2_VP_SYSTEM_BUZZER 0x00A0

/** @brief Touch status and coordinates register (0x0016). */
#define DGUS2_VP_TOUCH_STATUS 0x0016

/** @} */

#endif /* ZEPHYR_MODULES_DWIN_DGUS2_VP_H_ */
