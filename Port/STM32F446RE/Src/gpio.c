/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include "../Inc/gpio.h"

#define GPIOAEN  (1U << 0)
#define GPIOCEN  (1U << 2)
#define LD_BS5   (1U << 5)
#define LD_BR5   (1U << 21)
#define BTN_PIN  (1U << 13)
#define LD_PIN   (1U << 5)

void LD_Init(void) {
    RCC->AHB1ENR |=  GPIOAEN;
    GPIOA->MODER |=  (1U << 10);
    GPIOA->MODER &= ~(1U << 11);
}

void LD_On(void) {
    GPIOA->BSRR |= LD_BS5;
}

void LD_Off(void) {
    GPIOA->BSRR |= LD_BR5;
}

void LD_Toggle(void) {
    GPIOA->ODR  ^= LD_PIN;
}

void BTN_Init(void) {
    RCC->AHB1ENR |=  GPIOCEN;
    GPIOC->MODER &= ~(1U << 26);
    GPIOC->MODER &= ~(1U << 27);
}

/* B1 is active low */
bool BTN_State(void) {
    if (GPIOC->IDR & BTN_PIN) {
        return false;
    } else {
        return true;
    }
}
