/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "uart.h"
#include "rtc.h"
#include "service-console.h"
#include "system_stm32f4xx.h"

#include "05-orchestration/daily-cycle.h"
#include "03-validation/033-status/status.h"

int main(void) {
    /* CMSIS timing conversions */
    SystemCoreClockUpdate();

    /* DWT cycle counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    UART_Init();
    LD_Init();
    BTN_Init();

    (void)printf("\nFieldEdge-Evapotranspiration -- STM32F446RE\n");

    const Status rtc_status = RTC_Init();
    (void)printf("\nRTC init: %s\n", Status_ToString(rtc_status));
    (void)printf("\nPress B1 to run one daily cycle calculation.\n");

    Console_Init();

    static DailyResults results;
    const char *failed_step = "unknown";

    while (1) {
        Console_Poll();

        if (BTN_State()) {
            while (BTN_State()) {
                Console_Poll();
            }

            const RTCState rtc_state = RTC_GetState();

            if (rtc_state != RTC_STATE_TRUSTED) {
                (void)printf(
                    "\nCalculation not started: RTC state is %s.\n"
                    "Set/verify UTC time first.\n", RTCState_ToString(rtc_state));
                continue;
            }

            (void)printf("\nB1 accepted, starting RunDailyCycle...\n");
            LD_Toggle();

            /* RunDailyCycle() and a cycle counter surrounding it */
            const uint32_t cycles_start = DWT->CYCCNT;
            const Status status = RunDailyCycle(&results, &failed_step);
            const uint32_t cycles_elapsed = DWT->CYCCNT - cycles_start;
            const float elapsed_ms = ((float)cycles_elapsed * 1000.0f) / (float)SystemCoreClock;
            (void)printf("\nRunDailyCycle: %s (%lu cycles, %.3f ms)\n",
                Status_ToString(status), (unsigned long)cycles_elapsed, (double)elapsed_ms);

            /* Show live RTC time after the calculation attempt */
            Console_PrintCurrentTime();

            if (status != STATUS_OK) {
                (void)printf("\nRunDailyCycle failed at %s: %s.\n", failed_step, Status_ToString(status));
            } else {
                (void)printf(
                    "\nRunDailyCycle OK\n"
                    "Station date used by calculation: %04u-%02u-%02u\n"
                    "\n", (unsigned)results.date.year, (unsigned)results.date.month, (unsigned)results.date.day);
                PrintReport(&results);
            }
        }
    }
}
