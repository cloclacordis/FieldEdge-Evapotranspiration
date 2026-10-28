/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdbool.h>
#include <stddef.h>
#include "calendar-time.h"

#define SECONDS_PER_MINUTE   60U
#define MINUTES_PER_HOUR     60U
#define HOURS_PER_DAY        24U
#define MONTHS_PER_YEAR      12U

#define SECONDS_PER_HOUR     (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
#define SECONDS_PER_DAY      (SECONDS_PER_HOUR   * HOURS_PER_DAY)

#define DAYS_PER_COMMON_YEAR 365U
#define DAYS_PER_LEAP_YEAR   366U
#define FEBRUARY             2U
#define FEBRUARY_LEAP_DAYS   29U
#define FIRST_YEAR           1970U
#define LAST_YEAR            2106U

static const uint8_t DAYS_IN_MONTH[MONTHS_PER_YEAR] = {
    31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U
};

static bool IsLeapYear(const uint16_t year) {
    return ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

/* Month must be in 1..12; callers validate it before calling */
static uint8_t DaysInMonth(const uint16_t year, const uint8_t month) {
    if ((month == FEBRUARY) && IsLeapYear(year)) {
        return (uint8_t)FEBRUARY_LEAP_DAYS;
    }

    return DAYS_IN_MONTH[month - 1U];
}

static uint32_t DaysInYear(const uint16_t year) {
    return IsLeapYear(year) ? DAYS_PER_LEAP_YEAR : DAYS_PER_COMMON_YEAR;
}

Status CalendarTime_ToEpoch(const CalendarTime *t, uint32_t *out_epoch) {
    if ((t == NULL) || (out_epoch == NULL)) {
        return STATUS_NULL_POINTER;
    }

    /* Validate month before DaysInMonth() indexes the lookup table */
    if ((t->year   < FIRST_YEAR) || (t->year > LAST_YEAR) ||
        (t->month  < 1U)   || (t->month > MONTHS_PER_YEAR) ||
        (t->day    < 1U)   || (t->day   > DaysInMonth(t->year, t->month)) ||
        (t->hour   >= HOURS_PER_DAY) ||
        (t->minute >= MINUTES_PER_HOUR) ||
        (t->second >= SECONDS_PER_MINUTE)) {
        return STATUS_INVALID_VALUE;
    }

    uint64_t days = 0U;

    for (uint16_t year = (uint16_t)FIRST_YEAR; year < t->year; ++year) {
        days += DaysInYear(year);
    }

    for (uint8_t month = 1U; month < t->month; ++month) {
        days += DaysInMonth(t->year, month);
    }

    days += (uint64_t)(t->day - 1U);

    const uint64_t seconds = (days * SECONDS_PER_DAY)
        + ((uint64_t)t->hour   * SECONDS_PER_HOUR)
        + ((uint64_t)t->minute * SECONDS_PER_MINUTE)
        + (uint64_t)t->second;

    if (seconds > (uint64_t)UINT32_MAX) {
        return STATUS_INVALID_VALUE;
    }

    *out_epoch = (uint32_t)seconds;

    return STATUS_OK;
}

Status CalendarTime_FromEpoch(const uint32_t epoch, const int32_t utc_offset_min, CalendarTime *out_t) {
    if (out_t == NULL) {
        return STATUS_NULL_POINTER;
    }

    if ((utc_offset_min < CALENDAR_TIME_MIN_UTC_OFFSET_MIN) ||
        (utc_offset_min > CALENDAR_TIME_MAX_UTC_OFFSET_MIN)) {
        return STATUS_INVALID_VALUE;
    }

    /* 64-bit arithmetic prevents overflow while applying the UTC offset */
    const int64_t local = (int64_t)epoch + ((int64_t)utc_offset_min * (int64_t)SECONDS_PER_MINUTE);

    if ((local < 0) || (local > (int64_t)UINT32_MAX)) {
        return STATUS_INVALID_VALUE;
    }

    uint32_t days = (uint32_t)local / SECONDS_PER_DAY;
    const uint32_t seconds_of_day = (uint32_t)local % SECONDS_PER_DAY;
    uint16_t year = (uint16_t)FIRST_YEAR;

    while (days >= DaysInYear(year)) {
        days -= DaysInYear(year);
        ++year;
    }

    uint8_t month = 1U;

    while (days >= DaysInMonth(year, month)) {
        days -= DaysInMonth(year, month);
        ++month;
    }

    CalendarTime result;

    result.year   = year;
    result.month  = month;
    result.day    = (uint8_t)(days + 1U);
    result.hour   = (uint8_t)(seconds_of_day  / SECONDS_PER_HOUR);
    result.minute = (uint8_t)((seconds_of_day % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE);
    result.second = (uint8_t)(seconds_of_day  % SECONDS_PER_MINUTE);

    *out_t = result;
    
    return STATUS_OK;
}
