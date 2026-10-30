/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include "gpio.h"

#define LD_PIN   (1U << 5)

void LD_Init(void) {
    RCC->AHB1ENR |=  RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER |=  (1U << 10);
    GPIOA->MODER &= ~(1U << 11);
}

void LD_On(void) {
    GPIOA->BSRR |= GPIO_BSRR_BS5;
}

void LD_Off(void) {
    GPIOA->BSRR |= GPIO_BSRR_BR5;
}

void LD_Toggle(void) {
    GPIOA->ODR  ^= LD_PIN;
}
