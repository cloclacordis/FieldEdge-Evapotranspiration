/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "../CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"

void LD_Init(void);
void LD_On(void);
void LD_Off(void);
void LD_Toggle(void);
void BTN_Init(void);
bool BTN_State(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
