/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stddef.h>
#include "date-provider.h"
#include "../023-time/calendar-time.h"
#include "../023-time/time-provider.h"
#include "../022-configurations/deployment-config.h"

_Static_assert((CONFIG_STATION_UTC_OFFSET_MIN >= CALENDAR_TIME_MIN_UTC_OFFSET_MIN) &&
    (CONFIG_STATION_UTC_OFFSET_MIN <= CALENDAR_TIME_MAX_UTC_OFFSET_MIN),
    "CONFIG_STATION_UTC_OFFSET_MIN must be in the range UTC-12:00 ... UTC+14:00");

Status DateProvider_Read(DateData *date) {
    if (date == NULL) {
        return STATUS_NULL_POINTER;
    }

    uint32_t timestamp = 0U;
    Status status = TimeProvider_ReadTimestamp(&timestamp);

    if (status != STATUS_OK) {
        return status;
    }

    CalendarTime station_time;
    status = CalendarTime_FromEpoch(timestamp, CONFIG_STATION_UTC_OFFSET_MIN, &station_time);

    if (status != STATUS_OK) {
        return status;
    }

    const DateData result = { station_time.year, station_time.month, station_time.day };
    *date = result;

    return STATUS_OK;
}
