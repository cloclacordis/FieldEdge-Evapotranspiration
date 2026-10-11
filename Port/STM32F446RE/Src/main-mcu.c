/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdio.h>
#include "gpio.h"
#include "uart.h"
#include "05-orchestration/daily-cycle.h"
#include "03-validation/033-status/status.h"

int main(void) {
    /* Data Watchpoint & Trace: for smoke test only */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;

    UART_Init();
    LD_Init();
    BTN_Init();

    static DailyResults results;
    const char *failed_step = "unknown";

    printf("FieldEdge-Evapotranspiration -- MCU smoke test\n");
    printf("Press B1 to run one daily cycle calculation.\n");

    while (1) {
        if (BTN_State()) {
            while (BTN_State()) {}

            printf("B1 accepted, starting RunDailyCycle...\n");
            LD_Toggle();

            uint32_t cycles_start = DWT->CYCCNT;

            const Status status = RunDailyCycle(&results, &failed_step);

            uint32_t cycles_elapsed = DWT->CYCCNT - cycles_start;
            printf("\nRunDailyCycle: %lu cycles (%.3f ms)\n",
                (unsigned long)cycles_elapsed, (float)cycles_elapsed / 16000.0f);

            printf("\nRunDailyCycle returned\n");

            if (status != STATUS_OK) {
                printf("RunDailyCycle failed at %s: %s\n",
                    failed_step, Status_ToString(status));
            } else {
                printf("RunDailyCycle OK\n");
                PrintReport(&results);
            }
        }
    }
}
