/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdio.h>
#include "daily-cycle.h"
#include "../03-validation/033-status/status.h"

/**
 * @brief Reports a RunDailyCycle() failure and produces the process
 *        exit code.
 *
 * Prints whatever acquisition diagnostics were captured before the
 * failure (via PrintTrace()), then a one-line failure message to
 * stderr naming the failed step and its status.
 *
 * @param[in] results     Partially filled results from the failed
 *                        run; only results->trace is read. Must not
 *                        be NULL.
 * @param[in] failed_step Name of the step that failed, as set by
 *                        RunDailyCycle() via out_failed_step. Must
 *                        not be NULL.
 * @param[in] status      The Status value returned by RunDailyCycle().
 *
 * @return 1, unconditionally (used directly as the process exit code).
 */
static int PrintStatusAndReturn(const DailyResults *results, const char *failed_step, const Status status) {
    PrintTrace(&results->trace); /* Print diagnostics captured before the failure, if any */
    (void)fprintf(stderr, "Daily cycle failed at %s: %s\n", failed_step, Status_ToString(status));
    return 1;
}

/**
 * @brief Program entry point: runs one daily cycle and reports the outcome.
 *
 * Calls RunDailyCycle() exactly once. On success, prints the full
 * report via PrintReport(). On failure, reports it via
 * PrintStatusAndReturn().
 *
 * @return 0 on success (STATUS_OK); 1 if RunDailyCycle() failed.
 */
int main(void) {
    DailyResults results;
    const char *failed_step = "unknown";
    const Status status = RunDailyCycle(&results, &failed_step);

    if (status != STATUS_OK) {
        return PrintStatusAndReturn(&results, failed_step, status);
    }

    PrintReport(&results);

    return 0;
}
