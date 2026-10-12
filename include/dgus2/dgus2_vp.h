/*
 * Copyright (c) 2026 DAECGE TECNA SMART LAB
 * SPDX-License-Identifier: Apache-2.0
 *
 * Mapa de Direcciones Variable Pointer (VP) y Registros del Sistema DGUS II (T5L Guide V2.921).
 */

#ifndef ZEPHYR_MODULES_DWIN_DGUS2_VP_H_
#define ZEPHYR_MODULES_DWIN_DGUS2_VP_H_

/* ---- Registros de Sistema (Comando 0x80 / 0x81 o VP 0x0080-0x008F) ---- */
#define DGUS2_VP_SYSTEM_CONFIG    0x0080  /* Configuración general de sistema */
#define DGUS2_VP_SYSTEM_RTC       0x009C  /* RTC interno de pantalla (YY:MM:DD:WW:HH:MM:SS) */
#define DGUS2_VP_SYSTEM_BRIGHT    0x0082  /* Brillo de pantalla (0x00..0x64 = 0..100%) */
#define DGUS2_VP_SYSTEM_PAGE      0x0084  /* Control/Lectura de página activa (2 bytes: 0x5A01 + PageID) */
#define DGUS2_VP_CURRENT_PAGE     0x0014  /* Lectura directa de ID de página actual (Registro 0x14) */
#define DGUS2_VP_SYSTEM_BUZZER    0x00A0  /* Control de Buzzer interno (2 bytes: Time_MSB, Time_LSB) */
#define DGUS2_VP_TOUCH_STATUS     0x0016  /* Estado del toque (Coordenadas X, Y, Touch Status) */


#endif /* ZEPHYR_MODULES_DWIN_DGUS2_VP_H_ */
