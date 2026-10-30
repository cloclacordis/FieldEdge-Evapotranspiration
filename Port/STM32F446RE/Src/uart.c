/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stddef.h>
#include <stdint.h>
#include "uart.h"

/* Current clock tree: HSI = 16 MHz, AHB /1, APB1 /1 */
#define BAUDRATE       (115200U)
#define USART2_PCLK_HZ (16000000U)

static void UART_SetBaudrate(uint32_t periph_clk, uint32_t baudrate);
static void UART_Write(int ch);

int __io_putchar(int ch) {
    if (ch == '\n') {
        UART_Write('\r');
    }

    UART_Write(ch);
    return ch;
}

void UART_Init(void) {
    const uint32_t UART_PinModeMask = (3U << 4U)  | (3U << 6U);
    const uint32_t UART_PinAFMask   = (15U << 8U) | (15U << 12U);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER  = (GPIOA->MODER  & ~UART_PinModeMask) | (2U << 4U) | (2U << 6U);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~UART_PinAFMask)   | (7U << 8U) | (7U << 12U);

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    UART_SetBaudrate(USART2_PCLK_HZ, BAUDRATE);
    USART2->CR1   = USART_CR1_TE | USART_CR1_RE;
    USART2->CR1  |= USART_CR1_UE;
}

bool UART_TryReadChar(char *character) {
    if (character == NULL) {
        return false;
    }

    const uint32_t status = USART2->SR;

    if ((status & (USART_SR_RXNE | USART_SR_ORE)) == 0U) {
        return false;
    }

    const char received = (char)(USART2->DR & 0xFFU);

    if ((status & USART_SR_RXNE) == 0U) {
        return false;
    }

    *character = received;
    return true;
}

static void UART_Write(int ch) {
    while (!(USART2->SR & USART_SR_TXE)) {}
    USART2->DR = (ch & 0xFF);
}

static uint16_t UART_ComputeBaudrate(uint32_t periph_clk, uint32_t baudrate) {
    return((periph_clk + (baudrate / 2U)) / baudrate);
}

static void UART_SetBaudrate(uint32_t periph_clk, uint32_t baudrate) {
    USART2->BRR = UART_ComputeBaudrate(periph_clk, baudrate);
}
