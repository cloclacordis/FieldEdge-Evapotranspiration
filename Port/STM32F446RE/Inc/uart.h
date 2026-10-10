/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef UART_H
#define UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"
void UART_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_H */
