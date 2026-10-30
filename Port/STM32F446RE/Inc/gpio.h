/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef GPIO_H
#define GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"

void LD_Init(void);
void LD_On(void);
void LD_Off(void);
void LD_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
