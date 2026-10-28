/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include "rtc.h"
#include "02-providers/023-time/time-provider.h"

/* STM32 implementation of the shared time-source interface */
Status TimeProvider_ReadTimestamp(uint32_t *out_timestamp) {
    return RTC_ReadEpoch(out_timestamp);
}
