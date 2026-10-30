/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "rtc.h"
#include "uart.h"
#include "service-console.h"

#include "03-validation/033-status/status.h"
#include "02-providers/023-time/calendar-time.h"
#include "02-providers/023-time/time-provider.h"
#include "02-providers/021-date-provider/date-provider.h"

#define LINE_CAPACITY  48U
#define ISO_UTC_LENGTH 20U
#define CMD_SET_PREFIX "SET-UTC "

static char s_line[LINE_CAPACITY];
static uint32_t s_length = 0U;
static bool s_overflow   = false;

static bool ParseDigits(const char *text, const uint32_t count, uint32_t *out_value) {
    uint32_t value = 0U;

    for (uint32_t i = 0U; i < count; ++i) {
        if ((text[i] < '0') || (text[i] > '9')) {
            return false;
        }

        value = (value * 10U) + (uint32_t)(text[i] - '0');
    }

    *out_value = value;
    return true;
}

/* Accept exactly YYYY-MM-DDTHH:MM:SSZ; Z means UTC */
static Status ParseUtcTimestamp(const char *text, uint32_t *out_timestamp) {
    if (strlen(text) != ISO_UTC_LENGTH) {
        return STATUS_INVALID_VALUE;
    }

    uint32_t year   = 0U;
    uint32_t month  = 0U;
    uint32_t day    = 0U;
    uint32_t hour   = 0U;
    uint32_t minute = 0U;
    uint32_t second = 0U;

    if ((text[4]  != '-') || (text[7]  != '-') || (text[10] != 'T') ||
        (text[13] != ':') || (text[16] != ':') || (text[19] != 'Z') ||
        !ParseDigits(&text[0],  4U, &year)   ||
        !ParseDigits(&text[5],  2U, &month)  ||
        !ParseDigits(&text[8],  2U, &day)    ||
        !ParseDigits(&text[11], 2U, &hour)   ||
        !ParseDigits(&text[14], 2U, &minute) ||
        !ParseDigits(&text[17], 2U, &second)) {
        return STATUS_INVALID_VALUE;
    }

    const CalendarTime utc = {
        (uint16_t)year, (uint8_t)month,  (uint8_t)day,
        (uint8_t)hour,  (uint8_t)minute, (uint8_t)second
    };

    return CalendarTime_ToEpoch(&utc, out_timestamp);
}

void Console_PrintCurrentTime(void) {
    const RTCState state = RTC_GetState();

    (void)printf("\nRTC state: %s\n", RTCState_ToString(state));

    uint32_t timestamp = 0U;
    const Status time_status = TimeProvider_ReadTimestamp(&timestamp);

    if (time_status != STATUS_OK) {
        (void)printf("UTC time unavailable: %s\n", Status_ToString(time_status));
        return;
    }

    CalendarTime utc;
    const Status calendar_status = CalendarTime_FromEpoch(timestamp, 0, &utc);

    if (calendar_status != STATUS_OK) {
        (void)printf("UTC conversion failed: %s\n", Status_ToString(calendar_status));
        return;
    }

    (void)printf("UTC: %04u-%02u-%02uT%02u:%02u:%02uZ\n", (unsigned)utc.year,
        (unsigned)utc.month,  (unsigned)utc.day, (unsigned)utc.hour,
        (unsigned)utc.minute, (unsigned)utc.second);

    DateData station_date;
    const Status date_status = DateProvider_Read(&station_date);

    if (date_status == STATUS_OK) {
        (void)printf("\nStation date: %04u-%02u-%02u\n", (unsigned)station_date.year,
            (unsigned)station_date.month, (unsigned)station_date.day);
    } else {
        (void)printf("\nStation date unavailable: %s\n", Status_ToString(date_status));
    }
}

static void PrintStartupStatus(void) {
    const RTCState state = RTC_GetState();

    (void)printf("\nUART service console ready (115200 baud).\n");
    (void)printf("\nCommands: TIME | SET-UTC YYYY-MM-DDTHH:MM:SSZ | RUN\n");
    (void)printf("SET-UTC uses UTC, 24-hour time.\n");
    (void)printf("\nRTC state: %s\n", RTCState_ToString(state));

    if (state == RTC_STATE_TRUSTED) {
        (void)printf(
        	"RTC time is trusted.\n"
        	"Enter TIME to verify it, or RUN to calculate.\n");
    } else if (state == RTC_STATE_CLOCK_FAULT) {
        (void)printf(
        	"RTC hardware is unavailable. Fix the RTC clock before calculating.\n");
    } else {
        (void)printf(
        	"\nRTC time is not trusted. Close the serial terminal,\n"
        	"run Tools/RTC/rtc-set.py from the host, then reopen the terminal before RUN.\n");
    }
}

static bool HandleLine(const char *line) {
    if (strcmp(line, "TIME") == 0) {
        Console_PrintCurrentTime();
        return false;
    }

    if (strncmp(line, CMD_SET_PREFIX, sizeof(CMD_SET_PREFIX) - 1U) == 0) {
        uint32_t timestamp = 0U;

        Status status = ParseUtcTimestamp(&line[sizeof(CMD_SET_PREFIX) - 1U], &timestamp);

        if (status != STATUS_OK) {
            (void)printf("SET-UTC: %s\n", Status_ToString(status));
            (void)printf("Expected: SET-UTC YYYY-MM-DDTHH:MM:SSZ (UTC).\n");
            return false;
        }

        status = RTC_SetEpoch(timestamp);

        /* Response line for both operators and host tooling */
        (void)printf("\nSET-UTC: %s\n", Status_ToString(status));

        if (status == STATUS_OK) {
            (void)printf("\nRTC set and hardware read-back verified.\n");
        }

        Console_PrintCurrentTime();
        return false;
    }
    
    if (strcmp(line, "RUN") == 0) {
        return true;
    }

    (void)printf("\nCommand rejected. Use TIME, SET-UTC YYYY-MM-DDTHH:MM:SSZ, or RUN.\n");
    
    return false;
}

void Console_Init(void) {
    s_length   = 0U;
    s_overflow = false;

    PrintStartupStatus();
}

bool Console_Poll(void) {
    bool run_requested = false;
    char character;

    while (UART_TryReadChar(&character)) {
        if ((character == '\r') || (character == '\n')) {
            if (s_overflow) {
                (void)printf("Command rejected: line too long.\n");
            } else if (s_length  > 0U) {
                s_line[s_length] = '\0';

                if (HandleLine(s_line)) {
                    run_requested = true;
                }
            }

            s_length   = 0U;
            s_overflow = false;

        } else if (s_length  < (LINE_CAPACITY - 1U)) {
            s_line[s_length] = character;
            ++s_length;

        } else {
            s_overflow = true;
        }
    }

    return run_requested;
}
