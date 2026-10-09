/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

// #include <time.h>
#include <stddef.h>
#include "date-provider.h"

Status DateProvider_Read(DateData* date) {
    if (date == NULL) {
        return STATUS_NULL_POINTER;
    }

    /*time_t now = time(NULL);

    if (now == (time_t)(-1)) {
        return STATUS_INVALID_VALUE;
    }

    struct tm* t = localtime(&now);

    if (t == NULL) {
        return STATUS_INVALID_VALUE;
    }

    date->year  = (uint16_t)(1900 + t->tm_year);
    date->month = (uint8_t)(t->tm_mon + 1);
    date->day   = (uint8_t)t->tm_mday;
    */

    /* SMOKE TEST ONLY, see issues-v01x.md item 3 */
    date->year  = 2026;
    date->month = 10;
    date->day   = 8;

    return STATUS_OK;
}
