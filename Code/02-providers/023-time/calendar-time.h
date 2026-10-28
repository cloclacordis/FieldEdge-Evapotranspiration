/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef CALENDAR_TIME_H
#define CALENDAR_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"

/* Fixed UTC offsets; minutes east of UTC */
#define CALENDAR_TIME_MIN_UTC_OFFSET_MIN (-720)
#define CALENDAR_TIME_MAX_UTC_OFFSET_MIN (840)

/* A Gregorian calendar date/time; the caller defines
 * whether it represents UTC or station standard time */
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
} CalendarTime;

/** Convert a UTC calendar date/time to unsigned seconds since the Unix epoch.
 * Supports 1970-01-01 through 2106-02-07T06:28:15Z.
 * On failure, out_epoch is left unchanged. */
Status CalendarTime_ToEpoch(const CalendarTime *t, uint32_t *out_epoch);

/** Convert a Unix timestamp to calendar date/time using a fixed UTC offset.
 * Use utc_offset_min = 0 for UTC. Daylight-saving rules are not applied.
 * On failure, out_t is left unchanged. */
Status CalendarTime_FromEpoch(uint32_t epoch, int32_t utc_offset_min, CalendarTime *out_t);

#ifdef __cplusplus
}
#endif

#endif /* CALENDAR_TIME_H */
