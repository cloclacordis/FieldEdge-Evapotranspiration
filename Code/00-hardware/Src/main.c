/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdio.h>

#include "../Inc/gpio.h"
#include "../Inc/uart.h"

#include "../CMSIS/Include/core_cm4.h" // SCB->CPACR; DWT->CYCCNT

#include "../../05-orchestration/daily-cycle.h"
#include "../../03-validation/033-status/status.h"

int main(void) {
    /* FPU */
    SCB->CPACR |= (3UL << 20) | (3UL << 22);
    __DSB();
    __ISB();

    /* Data Watchpoint & Trace */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    UART_Init();
    LD_Init();
    BTN_Init();

    static DailyResults results;
    const char *failed_step = "unknown";

    printf("FieldEdge-Evapotranspiration -- MCU smoke test\r\n");
    printf("Press B1 to run one daily cycle calculation.\r\n");

    while (1) {
        if (BTN_State()) {
            while (BTN_State()) {}

            printf("B1 accepted, starting RunDailyCycle...\r\n");
            LD_Toggle();

            uint32_t cycles_start = DWT->CYCCNT;

            const Status status = RunDailyCycle(&results, &failed_step);

            uint32_t cycles_elapsed = DWT->CYCCNT - cycles_start;
            printf("\r\nRunDailyCycle: %lu cycles (%.3f ms)\r\n",
                (unsigned long)cycles_elapsed, (float)cycles_elapsed / 16000.0f);

            printf("\r\nRunDailyCycle returned\r\n");

            if (status != STATUS_OK) {
                printf("RunDailyCycle failed at %s: %s\r\n",
                    failed_step, Status_ToString(status));
            } else {
            	printf("RunDailyCycle OK\r\n");
                PrintReport(&results);
            }
        }
    }
}
