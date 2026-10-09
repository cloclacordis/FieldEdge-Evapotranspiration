/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdint.h>
#include "../Inc/uart.h"

#define GPIOAEN     (1U << 0)
#define UART2EN     (1U << 17)

#define BAUDRATE    (115200)
#define FREQ        (16000000)
#define APB1_CLK    FREQ

#define CR1_TE      (1U << 3)
#define CR1_UE      (1U << 13)
#define SR_TXE      (1U << 7)

static void UART_SetBaudrate(uint32_t periph_clk, uint32_t baudrate);
static void UART_Write(int ch);

int __io_putchar(int ch) {
	UART_Write(ch);
	return ch;
}

void UART_Init(void) {
	RCC->AHB1ENR  |=  GPIOAEN;
	GPIOA->MODER  &= ~(1U << 4);
	GPIOA->MODER  |=  (1U << 5);
	GPIOA->AFR[0] |=  (1U << 8);
	GPIOA->AFR[0] |=  (1U << 9);
	GPIOA->AFR[0] |=  (1U << 10);
	GPIOA->AFR[0] &= ~(1U << 11);

	RCC->APB1ENR  |=  UART2EN;
	UART_SetBaudrate(APB1_CLK, BAUDRATE);
	USART2->CR1    =  CR1_TE;
	USART2->CR1   |=  CR1_UE;
}

static void UART_Write(int ch) {
	while (!(USART2->SR & SR_TXE)) {}
	USART2->DR = (ch & 0xFF);
}

static uint16_t UART_ComputeBaudrate(uint32_t periph_clk, uint32_t baudrate) {
	return((periph_clk + (baudrate / 2U)) / baudrate);
}

static void UART_SetBaudrate(uint32_t periph_clk, uint32_t baudrate) {
	USART2->BRR = UART_ComputeBaudrate(periph_clk, baudrate);
}
