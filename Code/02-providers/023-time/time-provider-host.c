/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <time.h>
#include <stdint.h>
#include "time-provider.h"

/* *** Host implementation used by the PC application and tests *** */
Status TimeProvider_ReadTimestamp(uint32_t *out_timestamp) {
    if (out_timestamp == NULL) {
        return STATUS_NULL_POINTER;
    }

    const time_t now = time(NULL);

    if (now == (time_t)-1) {
        return STATUS_UNAVAILABLE;
    }

    /* Convert before narrowing; negative values become larger than uint32_t */
    const uintmax_t timestamp = (uintmax_t)now;

    if ((timestamp < (uintmax_t)TIME_PROVIDER_MIN_EPOCH) ||
        (timestamp > (uintmax_t)TIME_PROVIDER_MAX_EPOCH)) {
        return STATUS_UNAVAILABLE;
    }

    *out_timestamp = (uint32_t)timestamp;

    return STATUS_OK;
}
