/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include "unity.h"

#include "../02-providers/022-configurations/deployment-config.h"
#include "../02-providers/021-date-provider/date-provider.h"
#include "../02-providers/023-time/calendar-time.h"
#include "../02-providers/023-time/time-provider.h"
#include "../03-validation/033-status/status.h"

/* *** Reference instants, UTC seconds since 1970-01-01T00:00:00Z **** **** **** */
#define EPOCH_Y2000_START         946684800U   /* 2000-01-01T00:00:00Z * * ***** */
#define EPOCH_Y2001_START         978307200U   /* 2001-01-01T00:00:00Z ** * * ** */
#define EPOCH_2024_LEAP_DAY_NOON  1709208000U  /* 2024-02-29T12:00:00Z *** *** * */
#define EPOCH_2026_START          1767225600U  /* 2026-01-01T00:00:00Z * *** * * */
#define EPOCH_2026_10_08_NOON     1791460800U  /* 2026-10-08T12:00:00Z * * *** * */
#define EPOCH_INT32_MAX           2147483647U  /* 2038-01-19T03:14:07Z ***** *** */
#define EPOCH_INT32_MAX_PLUS_1    2147483648U  /* 2038-01-19T03:14:08Z * ** * ** */
#define EPOCH_2099_LAST_SECOND    4102444799U  /* 2099-12-31T23:59:59Z **** ** * */
#define EPOCH_2100_START          4102444800U  /* 2100-01-01T00:00:00Z ** *** ** */
#define EPOCH_UINT32_MAX          4294967295U  /* 2106-02-07T06:28:15Z *** * * * */
#define ROUND_TRIP_STRIDE_S       7919U        /* walks through all times of day */

static Status   stub_status;
static uint32_t stub_now;

Status TimeProvider_ReadTimestamp(uint32_t *out_timestamp) {
    if (out_timestamp == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (stub_status == STATUS_OK) {
        *out_timestamp = stub_now;
    }

    return stub_status;
}

static void PrintTestCase(const char *case_id, const char *test_name) {
    (void)printf("\n>>> %s: %s\n", case_id, test_name);
}

static void PrintCalendarTime(const CalendarTime t) {
    (void)printf("%04u-%02u-%02uT%02u:%02u:%02u",
        (unsigned)t.year, (unsigned)t.month, (unsigned)t.day,
        (unsigned)t.hour, (unsigned)t.minute, (unsigned)t.second);
}

static void AssertStatus(const char *label, const Status actual, const Status expected) {
    char message[160];

    (void)snprintf(message, sizeof(message), "%s: actual=%s expected=%s",
        label, Status_ToString(actual), Status_ToString(expected));

    (void)printf("  %-43s actual=%s expected=%s\n",
        label, Status_ToString(actual), Status_ToString(expected));

    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected, (int)actual, message);
}

static CalendarTime Ct(const unsigned year, const unsigned month, const unsigned day,
    const unsigned hour, const unsigned minute, const unsigned second) {
    CalendarTime t;

    t.year   = (uint16_t)year;
    t.month  = (uint8_t)month;
    t.day    = (uint8_t)day;
    t.hour   = (uint8_t)hour;
    t.minute = (uint8_t)minute;
    t.second = (uint8_t)second;

    return t;
}

static void AssertToEpoch(const uint32_t expected, const CalendarTime t) {
    uint32_t epoch = 0U;
    const Status status = CalendarTime_ToEpoch(&t, &epoch);

    (void)printf("  ToEpoch(");
    PrintCalendarTime(t);

    (void)printf("Z): status=%s, epoch=%" PRIu32 " (expected=%" PRIu32 ")\n",
        Status_ToString(status), epoch, expected);

    TEST_ASSERT_EQUAL_INT_MESSAGE((int)STATUS_OK, (int)status,
        "CalendarTime_ToEpoch() should accept this date");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected, epoch,
        "CalendarTime_ToEpoch() returned an unexpected timestamp");
}

static void AssertFromEpoch(const uint32_t epoch, const int32_t offset_min,
    const CalendarTime expected) {
    CalendarTime actual = Ct(1U, 1U, 1U, 0U, 0U, 0U);
    const Status status = CalendarTime_FromEpoch(epoch, offset_min, &actual);

    (void)printf("  FromEpoch(epoch=%" PRIu32 ", offset=%" PRId32 " min): ", epoch, offset_min);
    (void)printf("status=%s, actual=", Status_ToString(status));

    PrintCalendarTime(actual);
    (void)printf(", expected=");

    PrintCalendarTime(expected);
    (void)printf("\n");

    TEST_ASSERT_EQUAL_INT_MESSAGE((int)STATUS_OK, (int)status,
        "CalendarTime_FromEpoch() should accept this timestamp and offset");

    TEST_ASSERT_EQUAL_UINT16(expected.year, actual.year);
    TEST_ASSERT_EQUAL_UINT8(expected.month, actual.month);
    TEST_ASSERT_EQUAL_UINT8(expected.day, actual.day);
    TEST_ASSERT_EQUAL_UINT8(expected.hour, actual.hour);
    TEST_ASSERT_EQUAL_UINT8(expected.minute, actual.minute);
    TEST_ASSERT_EQUAL_UINT8(expected.second, actual.second);
}

static void AssertToEpochRejects(const CalendarTime t) {
    const uint32_t sentinel = UINT32_C(0xDEADBEEF);
    uint32_t epoch = sentinel;

    const Status status = CalendarTime_ToEpoch(&t, &epoch);

    (void)printf("  ToEpoch(");
    PrintCalendarTime(t);

    (void)printf("Z): status=%s (expected=%s), output=0x%08" PRIX32,
        Status_ToString(status), Status_ToString(STATUS_INVALID_VALUE), epoch);

    if (epoch == sentinel) {
        (void)printf(" (unchanged)\n");
    } else {
        (void)printf(" (CHANGED; expected unchanged)\n");
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE((int)STATUS_INVALID_VALUE, (int)status,
        "CalendarTime_ToEpoch() should reject this date");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(sentinel, epoch,
        "Output must remain unchanged when conversion fails");
}

void setUp(void) {
    stub_status = STATUS_OK;
    stub_now = EPOCH_2026_10_08_NOON;
}   /* Called before each test */

/* Called after each test */
void tearDown(void) {}

/* *** * * * ****** * * ******* * * *** * *** * * * *** * * ***** * * * **** *** *
 *                               TEST SUITES                                     *
 * * * * ****** * * **** * * ** * * ** **** * *** **** * ***** * *** * ** **** * */

/* Host tests of the calendar-time functions: CalendarTime_ToEpoch(),
 * CalendarTime_FromEpoch(), DateProvider_Read() *** * * ****** * * ***** ** *** */

/* *** CalendarTime_ToEpoch() *** */

static void test_ToEpoch_KnownInstants(void) {
    PrintTestCase("TC01", __func__);

    AssertToEpoch(0U, Ct(1970U, 1U, 1U, 0U, 0U, 0U));
    AssertToEpoch(EPOCH_Y2000_START, Ct(2000U, 1U, 1U, 0U, 0U, 0U));
    AssertToEpoch(EPOCH_2024_LEAP_DAY_NOON, Ct(2024U, 2U, 29U, 12U, 0U, 0U));
    AssertToEpoch(EPOCH_2026_START, Ct(2026U, 1U, 1U, 0U, 0U, 0U));
    AssertToEpoch(EPOCH_2026_10_08_NOON, Ct(2026U, 10U, 8U, 12U, 0U, 0U));
}

static void test_ToEpoch_ProviderWindowMatchesCalendar(void) {
    PrintTestCase("TC02", __func__);

    AssertToEpoch(TIME_PROVIDER_MIN_EPOCH, Ct(2001U, 1U, 1U, 0U, 0U, 0U));
    TEST_ASSERT_EQUAL_UINT32(EPOCH_Y2001_START, TIME_PROVIDER_MIN_EPOCH);

    AssertToEpoch(TIME_PROVIDER_MAX_EPOCH, Ct(2099U, 12U, 31U, 23U, 59U, 59U));
    
    TEST_ASSERT_TRUE(TIME_PROVIDER_MIN_EPOCH > 0U);
    TEST_ASSERT_TRUE(TIME_PROVIDER_MIN_EPOCH < TIME_PROVIDER_MAX_EPOCH);
}

static void test_ToEpoch_Year2038_ExceedsSignedInt32(void) {
    PrintTestCase("TC03", __func__);

    /* A signed 32-bit time_t ends here; the unsigned 32-bit timestamp goes on */
    AssertToEpoch(EPOCH_INT32_MAX, Ct(2038U, 1U, 19U, 3U, 14U, 7U));
    AssertToEpoch(EPOCH_INT32_MAX_PLUS_1, Ct(2038U, 1U, 19U, 3U, 14U, 8U));
}

static void test_ToEpoch_Year2100_IsNotLeap(void) {
    PrintTestCase("TC04", __func__);

    AssertToEpoch(EPOCH_2100_START, Ct(2100U, 1U, 1U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2100U, 2U, 29U, 0U, 0U, 0U));
}

static void test_ToEpoch_Uint32Limit(void) {
    PrintTestCase("TC05", __func__);

    AssertToEpoch(EPOCH_UINT32_MAX, Ct(2106U, 2U, 7U, 6U, 28U, 15U));
    AssertToEpochRejects(Ct(2106U, 2U, 7U, 6U, 28U, 16U));
    AssertToEpochRejects(Ct(2106U, 12U, 31U, 23U, 59U, 59U));
    AssertToEpochRejects(Ct(2107U, 1U, 1U, 0U, 0U, 0U));
}

static void test_ToEpoch_LeapYearRule(void) {
    PrintTestCase("TC06", __func__);

    uint32_t epoch = 0U;
    const CalendarTime leap_400 = Ct(2000U, 2U, 29U, 0U, 0U, 0U); /* divisible by 400 */
    const CalendarTime leap_4   = Ct(2024U, 2U, 29U, 0U, 0U, 0U); /* divisible by 4, not by 100 */

    Status status = CalendarTime_ToEpoch(&leap_400, &epoch);
    (void)printf("  Leap date 2000-02-29: status=%s, epoch=%" PRIu32 "\n", Status_ToString(status), epoch);
    AssertStatus("2000-02-29 accepted", status, STATUS_OK);

    status = CalendarTime_ToEpoch(&leap_4, &epoch);
    (void)printf("  Leap date 2024-02-29: status=%s, epoch=%" PRIu32 "\n", Status_ToString(status), epoch);
    AssertStatus("2024-02-29 accepted", status, STATUS_OK);

    AssertToEpochRejects(Ct(2023U, 2U, 29U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 2U, 29U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2100U, 2U, 29U, 0U, 0U, 0U));    /* div. by 100 */
    AssertToEpochRejects(Ct(2024U, 2U, 30U, 0U, 0U, 0U));
}

static void test_ToEpoch_RejectsFieldsOutOfRange(void) {
    PrintTestCase("TC07", __func__);

    AssertToEpochRejects(Ct(1969U, 12U, 31U, 23U, 59U, 59U));
    AssertToEpochRejects(Ct(2026U, 0U, 1U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 13U, 1U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 1U, 0U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 1U, 32U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 4U, 31U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 6U, 31U, 0U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 1U, 1U, 24U, 0U, 0U));
    AssertToEpochRejects(Ct(2026U, 1U, 1U, 0U, 60U, 0U));
    AssertToEpochRejects(Ct(2026U, 1U, 1U, 0U, 0U, 60U));
}

static void test_ToEpoch_NullPointers(void) {
    PrintTestCase("TC08", __func__);

    const CalendarTime t = Ct(2026U, 1U, 1U, 0U, 0U, 0U);
    uint32_t epoch = 0U;

    AssertStatus("CalendarTime_ToEpoch(NULL, &epoch)",
        CalendarTime_ToEpoch(NULL, &epoch), STATUS_NULL_POINTER);
    AssertStatus("CalendarTime_ToEpoch(&date, NULL)",
        CalendarTime_ToEpoch(&t, NULL), STATUS_NULL_POINTER);
}

/* *** CalendarTime_FromEpoch() *** */

static void test_FromEpoch_KnownInstants(void) {
    PrintTestCase("TC09", __func__);

    AssertFromEpoch(0U, 0, Ct(1970U, 1U, 1U, 0U, 0U, 0U));
    AssertFromEpoch(EPOCH_Y2000_START, 0, Ct(2000U, 1U, 1U, 0U, 0U, 0U));
    AssertFromEpoch(EPOCH_2026_10_08_NOON, 0, Ct(2026U, 10U, 8U, 12U, 0U, 0U));
    AssertFromEpoch(EPOCH_INT32_MAX_PLUS_1, 0, Ct(2038U, 1U, 19U, 3U, 14U, 8U));
    AssertFromEpoch(EPOCH_2100_START, 0, Ct(2100U, 1U, 1U, 0U, 0U, 0U));
    AssertFromEpoch(EPOCH_UINT32_MAX, 0, Ct(2106U, 2U, 7U, 6U, 28U, 15U));
}

static void test_FromEpoch_DayMonthYearBoundaries(void) {
    PrintTestCase("TC10", __func__);

    AssertFromEpoch(EPOCH_2024_LEAP_DAY_NOON + 43199U, 0, Ct(2024U, 2U, 29U, 23U, 59U, 59U));
    AssertFromEpoch(EPOCH_2024_LEAP_DAY_NOON + 43200U, 0, Ct(2024U, 3U, 1U, 0U, 0U, 0U));
    AssertFromEpoch(EPOCH_2026_START - 1U, 0, Ct(2025U, 12U, 31U, 23U, 59U, 59U));
    AssertFromEpoch(EPOCH_2026_START, 0, Ct(2026U, 1U, 1U, 0U, 0U, 0U));
    AssertFromEpoch(EPOCH_2100_START - 1U, 0, Ct(2099U, 12U, 31U, 23U, 59U, 59U));
}

static void test_FromEpoch_StationOffsetMovesDayBoundary(void) {
    PrintTestCase("TC11", __func__);

    /* UTC+10:00: local midnight of 2026-10-21 is 2026-10-20T14:00:00Z */
    const uint32_t utc_2026_10_20_1400 = EPOCH_2026_10_08_NOON + (12U * 86400U) + (2U * 3600U);
    AssertFromEpoch(utc_2026_10_20_1400 - 1U, 600, Ct(2026U, 10U, 20U, 23U, 59U, 59U));
    AssertFromEpoch(utc_2026_10_20_1400, 600, Ct(2026U, 10U, 21U, 0U, 0U, 0U));

    /* UTC-05:00: local midnight of 2026-10-21 is 2026-10-21T05:00:00Z */
    const uint32_t utc_2026_10_21_0500 = utc_2026_10_20_1400 + (15U * 3600U);
    AssertFromEpoch(utc_2026_10_21_0500 - 1U, -300, Ct(2026U, 10U, 20U, 23U, 59U, 59U));
    AssertFromEpoch(utc_2026_10_21_0500, -300, Ct(2026U, 10U, 21U, 0U, 0U, 0U));

    /* UTC+05:45 (not a whole number of hours) */
    AssertFromEpoch(utc_2026_10_20_1400 + 36000U, 345, Ct(2026U, 10U, 21U, 5U, 45U, 0U) );
}

static void test_FromEpoch_OffsetLimits(void) {
    PrintTestCase("TC12", __func__);

    CalendarTime t;

    AssertStatus("Minimum UTC offset accepted",
        CalendarTime_FromEpoch(EPOCH_2026_START, CALENDAR_TIME_MIN_UTC_OFFSET_MIN, &t), STATUS_OK);
    AssertStatus("Maximum UTC offset accepted",
        CalendarTime_FromEpoch(EPOCH_2026_START, CALENDAR_TIME_MAX_UTC_OFFSET_MIN, &t), STATUS_OK);
    AssertStatus("Offset below minimum rejected",
        CalendarTime_FromEpoch(EPOCH_2026_START, CALENDAR_TIME_MIN_UTC_OFFSET_MIN - 1, &t), STATUS_INVALID_VALUE);
    AssertStatus("Offset above maximum rejected",
        CalendarTime_FromEpoch(EPOCH_2026_START, CALENDAR_TIME_MAX_UTC_OFFSET_MIN + 1, &t), STATUS_INVALID_VALUE);
}

static void test_FromEpoch_ResultOutsideUint32Range(void) {
    PrintTestCase("TC13", __func__);

    CalendarTime t = Ct(2026U, 1U, 1U, 0U, 0U, 0U);

    AssertStatus("Negative local epoch rejected",
        CalendarTime_FromEpoch(0U, -1, &t), STATUS_INVALID_VALUE);
    AssertStatus("Local epoch beyond UINT32_MAX rejected",
        CalendarTime_FromEpoch(EPOCH_UINT32_MAX, 1, &t), STATUS_INVALID_VALUE);

    (void)printf("  Output year after failed conversions: %u (expected unchanged: 2026)\n", (unsigned)t.year);
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(2026U, t.year, "Output must remain unchanged on failure");
}

static void test_FromEpoch_NullPointer(void) {
    PrintTestCase("TC14", __func__);

    AssertStatus("CalendarTime_FromEpoch(..., NULL)",
        CalendarTime_FromEpoch(EPOCH_2026_START, 0, NULL), STATUS_NULL_POINTER);
}

static void test_RoundTrip_WholeUint32Range(void) {
    PrintTestCase("TC15", __func__);

    uint32_t checked = 0U;
    for (uint32_t epoch = 0U; epoch <= (EPOCH_UINT32_MAX - ROUND_TRIP_STRIDE_S); epoch += ROUND_TRIP_STRIDE_S) {
        CalendarTime t;
        uint32_t back = 0U;

        TEST_ASSERT_EQUAL_INT(STATUS_OK, CalendarTime_FromEpoch(epoch, 0, &t));
        TEST_ASSERT_EQUAL_INT(STATUS_OK, CalendarTime_ToEpoch(&t, &back));
        TEST_ASSERT_EQUAL_UINT32(epoch, back);
        ++checked;
    }

    (void)printf("  Round-trip checks passed: %" PRIu32 ", step=%" PRIu32 " s, UTC offset=0\n",
        checked, ROUND_TRIP_STRIDE_S);
}

/* *** DateProvider_Read() on the test double *** */

static void test_DateProvider_ReturnsStationDate(void) {
    PrintTestCase("TC16", __func__);

    DateData date;
    CalendarTime expected;
    stub_now = EPOCH_2026_10_08_NOON;

    AssertStatus("Build expected station date",
        CalendarTime_FromEpoch(stub_now, CONFIG_STATION_UTC_OFFSET_MIN, &expected), STATUS_OK);
    AssertStatus("DateProvider_Read", DateProvider_Read(&date), STATUS_OK);

    (void)printf("  Controlled source: UTC epoch=%" PRIu32 ", station offset=%d min\n",
        stub_now, CONFIG_STATION_UTC_OFFSET_MIN);
    (void)printf("  Station date: actual=%04u-%02u-%02u, expected=%04u-%02u-%02u\n",
        (unsigned)date.year, (unsigned)date.month, (unsigned)date.day,
        (unsigned)expected.year, (unsigned)expected.month, (unsigned)expected.day);

    TEST_ASSERT_EQUAL_UINT16(expected.year, date.year);
    TEST_ASSERT_EQUAL_UINT8(expected.month, date.month);
    TEST_ASSERT_EQUAL_UINT8(expected.day, date.day);
}

static void test_DateProvider_DayChangesAtStationMidnight(void) {
    PrintTestCase("TC17", __func__);

    /* Independent of the configured offset: find the UTC instant of the station's local midnight
     * (2026-10-21T00:00 station time) and step over it */
    const CalendarTime local_midnight = Ct(2026U, 10U, 21U, 0U, 0U, 0U);
    uint32_t local_midnight_as_utc = 0U;
    DateData date;

    AssertStatus("Convert station midnight to UTC calendar epoch",
        CalendarTime_ToEpoch(&local_midnight, &local_midnight_as_utc), STATUS_OK);

    const uint32_t boundary = (uint32_t)((int64_t)local_midnight_as_utc - ((int64_t)CONFIG_STATION_UTC_OFFSET_MIN * 60));

    (void)printf("  Station midnight 2026-10-21 occurs at UTC epoch=%" PRIu32
        " (offset=%d min)\n", boundary, CONFIG_STATION_UTC_OFFSET_MIN);

    stub_now = boundary - 1U;

    AssertStatus("DateProvider_Read before midnight", DateProvider_Read(&date), STATUS_OK);
    (void)printf("  One second before: %04u-%02u-%02u (expected 2026-10-20)\n",
        (unsigned)date.year, (unsigned)date.month, (unsigned)date.day);

    TEST_ASSERT_EQUAL_UINT16(2026U, date.year);
    TEST_ASSERT_EQUAL_UINT8(10U, date.month);
    TEST_ASSERT_EQUAL_UINT8(20U, date.day);

    stub_now = boundary;

    AssertStatus("DateProvider_Read at midnight", DateProvider_Read(&date), STATUS_OK);
    (void)printf("  At midnight:       %04u-%02u-%02u (expected 2026-10-21)\n",
        (unsigned)date.year, (unsigned)date.month, (unsigned)date.day);

    TEST_ASSERT_EQUAL_UINT8(21U, date.day);
}

static void test_DateProvider_PropagatesUnavailableTime(void) {
    PrintTestCase("TC18", __func__);

    DateData date;
    date.day = 7U;
    date.month = 7U;
    date.year = 2007U;
    stub_status = STATUS_UNAVAILABLE;

    AssertStatus("DateProvider_Read when clock unavailable",
        DateProvider_Read(&date), STATUS_UNAVAILABLE);

    (void)printf("  Output after error: %04u-%02u-%02u (expected unchanged: 2007-07-07)\n",
        (unsigned)date.year, (unsigned)date.month, (unsigned)date.day);

    TEST_ASSERT_EQUAL_UINT8(7U, date.day);
    TEST_ASSERT_EQUAL_UINT8(7U, date.month);
    TEST_ASSERT_EQUAL_UINT16(2007U, date.year);
}

static void test_DateProvider_NullPointer(void) {
    PrintTestCase("TC19", __func__);

    AssertStatus("DateProvider_Read(NULL)", DateProvider_Read(NULL),
        STATUS_NULL_POINTER);
}

/* *** * * * ****** * * ******* * * *** * *** * * * *** * * ***** * * * **** *** *
 * main() is Unity runner
 * UNITY_BEGIN() opens the session
 * RUN_TEST() registers & executes each test
 * UNITY_END() prints result & returns 0 (all passed) or 1 (errors)
 * * * * ****** * * **** * * ** * * ** **** * *** **** * ***** * *** * ** **** * */

int main(void) {
    UNITY_BEGIN();

    (void)printf("FieldEdge calendar/time tests\n");
    (void)printf("Calendar conversions use fixed test instants; no PC clock or RTC is read.\n");

    RUN_TEST(test_ToEpoch_KnownInstants);
    RUN_TEST(test_ToEpoch_ProviderWindowMatchesCalendar);
    RUN_TEST(test_ToEpoch_Year2038_ExceedsSignedInt32);
    RUN_TEST(test_ToEpoch_Year2100_IsNotLeap);
    RUN_TEST(test_ToEpoch_Uint32Limit);
    RUN_TEST(test_ToEpoch_LeapYearRule);
    RUN_TEST(test_ToEpoch_RejectsFieldsOutOfRange);
    RUN_TEST(test_ToEpoch_NullPointers);
    RUN_TEST(test_FromEpoch_KnownInstants);
    RUN_TEST(test_FromEpoch_DayMonthYearBoundaries);
    RUN_TEST(test_FromEpoch_StationOffsetMovesDayBoundary);
    RUN_TEST(test_FromEpoch_OffsetLimits);
    RUN_TEST(test_FromEpoch_ResultOutsideUint32Range);
    RUN_TEST(test_FromEpoch_NullPointer);
    RUN_TEST(test_RoundTrip_WholeUint32Range);
    RUN_TEST(test_DateProvider_ReturnsStationDate);
    RUN_TEST(test_DateProvider_DayChangesAtStationMidnight);
    RUN_TEST(test_DateProvider_PropagatesUnavailableTime);
    RUN_TEST(test_DateProvider_NullPointer);

    return UNITY_END();
}
