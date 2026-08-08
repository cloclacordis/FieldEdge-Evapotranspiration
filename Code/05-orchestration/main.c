/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdio.h>
#include "daily-cycle.h"
#include "../03-validation/033-status/status.h"

static int PrintStatusAndReturn(const char *failed_step, const Status status) {
    (void)fprintf(stderr, "Daily cycle failed at %s: %s\n", failed_step, Status_ToString(status));
    return 1;
}

int main(void) {
    DailyResults results;
    const char *failed_step = "unknown";
    const Status status = RunDailyCycle(&results, &failed_step);

    if (status != STATUS_OK) {
        return PrintStatusAndReturn(failed_step, status);
    }

    PrintReport(&results);

    return 0;
}
