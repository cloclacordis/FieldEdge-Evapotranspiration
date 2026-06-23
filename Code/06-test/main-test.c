/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
    #include <windows.h>    /* Sleep(ms) */
#else
    #include <unistd.h>     /* usleep(us) */
#endif

#include "unity.h"

#include "test-config.h"

#include "../01-measurement/011-air-temperature-read/air-temperature-read.h"
#include "../01-measurement/012-air-humidity-read/air-humidity-read.h"
#include "../01-measurement/014-sunshine-lux-read/sunshine-lux-read.h"
#include "../01-measurement/015-wind-speed-read/wind-speed-read.h"

#include "../02-providers/021-date-provider/date-provider.h"
#include "../02-providers/022-configurations/deployment-config.h"

#include "../03-validation/033-status/status.h"
#include "../03-validation/032-validation/validation.h"
#include "../03-validation/031-value-source/value-source.h"

#include "../04-calculation/041-air-temperature-calc/air-temperature-calc.h"
#include "../04-calculation/042-air-humidity-calc/air-humidity-calc.h"
#include "../04-calculation/043-vapour-pressure-calc/vapour-pressure-calc.h"
#include "../04-calculation/044-atmospheric-calc/atm-pressure-model.h"
#include "../04-calculation/044-atmospheric-calc/psychrometric-calc.h"

#include "../04-calculation/045-radiation-calc/geolocation-calc.h"
#include "../04-calculation/045-radiation-calc/day-in-year-calc.h"
#include "../04-calculation/045-radiation-calc/sunshine-lux-calc.h"

#include "../04-calculation/045-radiation-calc/extrater-radiation-calc.h"
#include "../04-calculation/045-radiation-calc/solar-radiation-calc.h"
#include "../04-calculation/045-radiation-calc/net-radiation-calc.h"

#include "../04-calculation/046-wind-speed-calc/wind-speed-calc.h"

#include "../04-calculation/047-evapotranspiration-calc/eto-calc.h"

#define PI (3.14159265358979323846)

/* *** Helper functions *** */

/* Pause in milliseconds (for real-time emulation) */
static void SleepMs(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep((useconds_t)(ms * 1000U));
#endif
}

/* Check Double: prints values & invokes Unity assertion */
static void AssertDouble(const char *label, const double actual,
                         const double expected, const double tol) {
    char msg[192];
    double abs_diff = fabs(actual - expected);

    snprintf(msg, sizeof(msg),
             "%s: actual=%.6f  expected=%.6f  diff=%.6f  tol=%.6f",
             label, actual, expected, abs_diff, tol);
             
    (void)printf("  %-36s actual=%.4f  expected=%.4f\n", label, actual, expected);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(tol, expected, actual, msg);    /* Unity macro */
}

/* Check Status: prints status string & invokes Unity assertion */
static void AssertStatus(const char *label, const Status actual,
                         const Status expected) {
    char msg[192];

    snprintf(msg, sizeof(msg), "%s: actual=%s  expected=%s",
             label, Status_ToString(actual), Status_ToString(expected));
             
    (void)printf("  %-36s %s\n", label, Status_ToString(actual));
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected, (int)actual, msg);    /* Unity macro */
}

void setUp(void)    { /* nothing */ }    /* Called before each test */
void tearDown(void) { /* nothing */ }    /* Called after  each test */

/* *** * * * ****** * * ******* * * *** * *** * * * *** * * ***** * * * **** *** *
 *                               TEST SUITES                                     *
 * * * * ****** * * **** * * ** * * ** **** * *** **** * ***** * *** * ** **** * */

/* *** TC1-5: air temperature & saturation vapour pressure modules *** */

/* TC1: normal path T = 20C, FAO56 ann. 2, tab. 2.3, 2.4 */
static void test_AirTemperature_NormalPath_T20(void) {
    (void)printf("\n>>> TC1: %s\n", __func__);

    AirTemperatureData data;
    double e_tmean = 0.0, delta = 0.0;
    AirTemperature_Init(&data);

    AssertStatus("AirTemperature_Update(20.0)",
                 AirTemperature_Update(&data, TEST_TEMP_INST_C, 0U), STATUS_OK);
    AssertDouble("T_mean", data.T_mean_C, TEST_TEMP_MEAN_C, TOL_AIR_TEMP);

    AssertStatus("Calc_SVP(Tmean)",
                 Calc_SaturationVapourPressure(data.T_mean_C, &e_tmean), STATUS_OK);
    AssertDouble("e(T_mean) [kPa]", e_tmean, TEST_E_TMEAN_EXPECTED, TOL_KPA);

    AssertStatus("Calc_SlopeDelta",
                 Calc_SlopeDelta(&data, &delta), STATUS_OK);
    AssertDouble("delta [kPa/C]", delta, TEST_DELTA_EXPECTED, TOL_KPA_PER_C);
}

/* TC2: T = 150C, out of range [-100, +100] */
static void test_AirTemperature_OutOfRange(void) {
    (void)printf("\n>>> TC2: %s\n", __func__);

    AirTemperatureData data;
    AirTemperature_Init(&data);

    AssertStatus("AirTemperature_Update(150.0)",
                 AirTemperature_Update(&data, TEST_TEMP_OUT_OF_RANGE, 0U),
                 STATUS_INVALID_VALUE);
}

/* TC3: NaN */
static void test_AirTemperature_NaN(void) {
    (void)printf("\n>>> TC3: %s\n", __func__);

    AirTemperatureData data;
    AirTemperature_Init(&data);

    AssertStatus("AirTemperature_Update(NaN)",
                 AirTemperature_Update(&data, NAN, 0U), STATUS_INVALID_VALUE);
}

/* TC4: INFINITY */
static void test_AirTemperature_Infinity(void) {
    (void)printf("\n>>> TC4: %s\n", __func__);

    AirTemperatureData data;
    AirTemperature_Init(&data);

    AssertStatus("AirTemperature_Update(INF)",
                 AirTemperature_Update(&data, INFINITY, 0U), STATUS_INVALID_VALUE);
}

/* TC5: three readings -> Tmin = 10, Tmax = 30, Tmean = (10 + 30) / 2 = 20 */
static void test_AirTemperature_MinMaxTracking(void) {
    (void)printf("\n>>> TC5: %s\n", __func__);

    AirTemperatureData data;
    AirTemperature_Init(&data);

    AssertStatus("Update(20.0)", AirTemperature_Update(&data, TEST_TEMP_INST_C, 0U),
                 STATUS_OK);
    AssertStatus("Update(10.0)", AirTemperature_Update(&data, TEST_TEMP_MIN_C,  1U),
                 STATUS_OK);
    AssertStatus("Update(30.0)", AirTemperature_Update(&data, TEST_TEMP_MAX_C,  2U),
                 STATUS_OK);
                 
    AssertDouble("T_min",  data.T_min_C,  TEST_TEMP_MIN_C,  TOL_AIR_TEMP);
    AssertDouble("T_max",  data.T_max_C,  TEST_TEMP_MAX_C,  TOL_AIR_TEMP);
    AssertDouble("T_mean", data.T_mean_C, TEST_TEMP_MEAN_C, TOL_AIR_TEMP);
}

/* *** TC6-8: validation, date conversion *** */

/* TC6: ValidDayOfYear, boundary values */
static void test_ValidDayOfYear_BoundaryValues(void) {
    (void)printf("\n>>> TC6: %s\n", __func__);

    TEST_ASSERT_TRUE_MESSAGE(ValidDayOfYear(1U),     "J=1 valid");
    TEST_ASSERT_TRUE_MESSAGE(ValidDayOfYear(366U),   "J=366 valid");
    TEST_ASSERT_FALSE_MESSAGE(ValidDayOfYear(0U),    "J=0 invalid");
    TEST_ASSERT_FALSE_MESSAGE(ValidDayOfYear(367U),  "J=367 invalid");
    TEST_ASSERT_TRUE_MESSAGE(ValidDayOfYear(246U),   "J=246 valid");
}

/* TC7: ValidLatitudeRad, boundary values [-π/2, +π/2] */
static void test_ValidLatitudeRad_BoundaryValues(void) {
    (void)printf("\n>>> TC7: %s\n", __func__);

    const double PI_2 = PI / 2.0;

    TEST_ASSERT_TRUE_MESSAGE(ValidLatitudeRad(0.0),     "lat=0 valid");
    TEST_ASSERT_TRUE_MESSAGE(ValidLatitudeRad(PI_2),    "lat=+π/2 valid");
    TEST_ASSERT_TRUE_MESSAGE(ValidLatitudeRad(-PI_2),   "lat=-π/2 valid");
    TEST_ASSERT_FALSE_MESSAGE(ValidLatitudeRad(2.0),    "lat=2.0 > π/2 invalid");
    TEST_ASSERT_FALSE_MESSAGE(ValidLatitudeRad(-2.0),   "lat=-2.0 < -π/2 invalid");
    TEST_ASSERT_FALSE_MESSAGE(ValidLatitudeRad(1.5708), "lat=1.5708 > π/2 invalid"); /* 1.5708 > π/2 ≈ 1.57079632... boundary case */
}

/* TC8: DayCalc_JFromDate, calendar dates & leap years */
static void test_DayCalc_JFromDate_CalendarDates(void) {
    (void)printf("\n>>> TC8: %s\n", __func__);

    TEST_ASSERT_EQUAL_UINT16(247U, DayCalc_JFromDate(3U,  9U,  2024U));    /* 3  Sep 2024 (leap)     -> J = 247     */
    TEST_ASSERT_EQUAL_UINT16(246U, DayCalc_JFromDate(3U,  9U,  2023U));    /* 3  Sep 2023 (non-leap) -> J = 246     */
    TEST_ASSERT_EQUAL_UINT16(1U,   DayCalc_JFromDate(1U,  1U,  2023U));    /* 1  Jan                 -> J = 1       */
    TEST_ASSERT_EQUAL_UINT16(365U, DayCalc_JFromDate(31U, 12U, 2023U));    /* 31 Dec non-leap        -> J = 365     */
    TEST_ASSERT_EQUAL_UINT16(366U, DayCalc_JFromDate(31U, 12U, 2024U));    /* 31 Dec leap            -> J = 366     */
    TEST_ASSERT_EQUAL_UINT16(59U,  DayCalc_JFromDate(28U, 2U,  2023U));    /* 28 Feb non-leap        -> J = 59      */
    TEST_ASSERT_EQUAL_UINT16(60U,  DayCalc_JFromDate(1U,  3U,  2023U));    /* 1  Mar non-leap        -> J = 60      */
    TEST_ASSERT_EQUAL_UINT16(61U,  DayCalc_JFromDate(1U,  3U,  2024U));    /* 1  Mar leap            -> J = 61      */
}

/* *** TC9-11: DayCalc_Update, error statuses *** */

/* TC9: NULL pointer */
static void test_DayCalc_Update_NullPointer(void) {
    (void)printf("\n>>> TC9: %s\n", __func__);

    LocationData loc; Location_Init(&loc);
    DayData dd;       DayCalc_Init(&dd);

    AssertStatus("DayCalc_Update(NULL, 246, loc)",
                 DayCalc_Update(NULL,  246U, &loc), STATUS_NULL_POINTER);
    AssertStatus("DayCalc_Update(dd, 246, NULL)",
                 DayCalc_Update(&dd,   246U, NULL), STATUS_NULL_POINTER);
}

/* TC10: invalid J, 0 & 367 */
static void test_DayCalc_Update_InvalidJ(void) {
    (void)printf("\n>>> TC10: %s\n", __func__);

    LocationData loc; Location_Init(&loc);
    DayData dd;       DayCalc_Init(&dd);

    AssertStatus("DayCalc_Update(J=0)",
                 DayCalc_Update(&dd, 0U, &loc),   STATUS_INVALID_VALUE);
    AssertStatus("DayCalc_Update(J=367)",
                 DayCalc_Update(&dd, 367U, &loc),  STATUS_INVALID_VALUE);
}

/* TC11: invalid latitude (lat_rad outside [-π/2, +π/2]);
 * use Location_Init() to set initialized = true, then override latitude_rad with invalid value */
static void test_DayCalc_Update_InvalidLatitude(void) {
    (void)printf("\n>>> TC11: %s\n", __func__);

    LocationData loc; Location_Init(&loc);
    DayData dd;       DayCalc_Init(&dd);

    loc.latitude_rad = 2.0;
    AssertStatus("DayCalc_Update(lat=+2.0 rad)",
                 DayCalc_Update(&dd, 246U, &loc), STATUS_INVALID_VALUE);

    loc.latitude_rad = -2.0;
    AssertStatus("DayCalc_Update(lat=-2.0 rad)",
                 DayCalc_Update(&dd, 246U, &loc), STATUS_INVALID_VALUE);
}

/* *** TC12-13: Location_DMS_to_decimal, FAO56 ex.7 *** */

/* TC12: Bangkok 1344'N */
static void test_Location_DMS_Bangkok(void) {
    (void)printf("\n>>> TC12: %s\n", __func__);

    LocationData loc;

    AssertStatus("DMS_to_decimal(Bangkok)",
                 Location_DMS_to_decimal(TEST_BANGKOK_LAT_DEG, TEST_BANGKOK_LAT_MIN,
                                         &loc.latitude_deg), STATUS_OK);

    loc.latitude_rad = loc.latitude_deg * (PI / 180.0);
    AssertDouble("latitude_deg", loc.latitude_deg, TEST_BANGKOK_LAT_EXPECTED, TOL_DEGREE);
    AssertDouble("latitude_rad", loc.latitude_rad, TEST_BANGKOK_RAD_EXPECTED, TOL_RADIANS);
}

/* TC13: Rio de Janeiro 2254'S */
static void test_Location_DMS_Rio(void) {
    (void)printf("\n>>> TC13: %s\n", __func__);

    LocationData loc;

    AssertStatus("DMS_to_decimal(Rio)",
                 Location_DMS_to_decimal(TEST_RIO_LAT_DEG, TEST_RIO_LAT_MIN,
                                         &loc.latitude_deg), STATUS_OK);

    loc.latitude_rad = loc.latitude_deg * (PI / 180.0);
    AssertDouble("latitude_deg", loc.latitude_deg, TEST_RIO_LAT_EXPECTED, TOL_DEGREE);
    AssertDouble("latitude_rad", loc.latitude_rad, TEST_RIO_RAD_EXPECTED, TOL_RADIANS);
}

/* *** TC14-16: extraterrestrial radiation Ra *** */

/* TC14: FAO56 ex.8, ex.9, 20S, J = 246;
 * reference: dr = 0.985, δ = 0.120 rad, ωs = 1.527 rad, N = 11.7 h, Ra = 32.2 MJ m-2 day-1 */
static void test_Calc_Ra_FAO56_ex8(void) {
    (void)printf("\n>>> TC14: %s\n", __func__);

    LocationData loc; Location_Init(&loc);
    DayData      dd;  DayCalc_Init(&dd);
    RaData       rd;  RaCalc_Init(&rd);

    AssertStatus("DayCalc_Update", DayCalc_Update(&dd, TEST_EX8_J, &loc), STATUS_OK);
    AssertDouble("dr",             dd.dr,          TEST_EX8_DR_EXPECTED,        TOL_ANGLE);
    AssertDouble("delta [rad]",    dd.delta_rad,   TEST_EX8_DELTA_RAD_EXPECTED, TOL_ANGLE);
    AssertDouble("omega_s [rad]",  dd.omega_s_rad, TEST_EX8_OMEGA_S_EXPECTED,   TOL_ANGLE);
    AssertDouble("N [h]",          dd.N_hours,     TEST_EX8_N_EXPECTED,         TOL_HOURS);

    AssertStatus("Calc_Ra", Calc_Ra(&rd, &dd, &loc), STATUS_OK);
    AssertDouble("Ra [MJ m-2 day-1]", rd.Ra_daily, TEST_EX8_RA_EXPECTED, TOL_RA);
}

/* TC15: polar night, 80N, J = 355 -> ωs = 0, N = 0, Ra = 0 */
static void test_Calc_Ra_PolarNight(void) {
    (void)printf("\n>>> TC15: %s\n", __func__);

    LocationData loc;

    loc.latitude_deg = TEST_POLAR_LAT_DEG;
    loc.latitude_rad = TEST_POLAR_LAT_DEG * (PI / 180.0);
    loc.elevation_m  = 0.0;
    loc.initialized  = true;

    DayData dd; DayCalc_Init(&dd);
    RaData  rd; RaCalc_Init(&rd);

    AssertStatus("DayCalc_Update",    DayCalc_Update(&dd, TEST_POLAR_J, &loc), STATUS_OK);
    AssertDouble("omega_s [rad]",     dd.omega_s_rad, TEST_POLAR_OMEGA_S_EXPECTED,  1e-9);
    AssertDouble("N [h]",             dd.N_hours,     TEST_POLAR_N_EXPECTED,  1e-9);

    AssertStatus("Calc_Ra", Calc_Ra(&rd, &dd, &loc), STATUS_OK);
    AssertDouble("Ra [MJ m-2 day-1]", rd.Ra_daily,   TEST_POLAR_RA_EXPECTED, 1e-9);
}

/* TC16: STATUS_INVALID_VALUE, DayData not initialized (initialized = false) */
static void test_Calc_Ra_UninitializedDayData(void) {
    (void)printf("\n>>> TC16: %s\n", __func__);

    LocationData loc; Location_Init(&loc);
    DayData      dd;  DayCalc_Init(&dd);    /* initialized = false - DayCalc_Update not called */
    RaData       rd;  RaCalc_Init(&rd);

    AssertStatus("Calc_Ra(uninitialized DayData)",
                 Calc_Ra(&rd, &dd, &loc), STATUS_INVALID_VALUE);
                 
    AssertDouble("Ra_daily remains 0.0", rd.Ra_daily, 0.0, 1e-9);    /* Ra_daily must not have changed */
}

/* *** TC17-19: sunshine accumulator module SunshineLux *** */

/* TC17: full sunshine day, 60 bright samples -> n = 1.0 h;
 * all samples above threshold: lux = 50000 > threshold = 20000; expected: bright_samples = 60, n_hours = 1.0 h */
static void test_SunshineLux_FullSunshine(void) {
    (void)printf("\n>>> TC17: %s\n", __func__);

    SunshineLuxData sd;

    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);
    AssertStatus("SunshineLux_ResetDay", SunshineLux_ResetDay(&sd), STATUS_OK);

    for (uint32_t i = 0U; i < 60U; ++i) {
        Status s = SunshineLux_Update(&sd, 50000.0, SENSOR_VALUE_MEASURED);

        if (s != STATUS_OK) {
            char msg[80];
            snprintf(msg, sizeof(msg), "SunshineLux_Update[%u] failed: %s",
                     (unsigned)i, Status_ToString(s));
            TEST_FAIL_MESSAGE(msg);
        }
    }

    (void)printf("  SunshineLux_Update * 60         all STATUS_OK"
                 "  (lux=50000 > threshold=20000)\n");

    (void)printf("  bright_samples=%-4u  total_samples=%u\n",
                 (unsigned)sd.bright_samples, (unsigned)sd.total_samples);

    AssertStatus("SunshineLux_FinalizeDay", SunshineLux_FinalizeDay(&sd), STATUS_OK);
    AssertDouble("n_hours", sd.n_hours, 1.0, 0.001);
}

/* TC18: no sunshine, 60 dark samples -> n = 0;
 * all samples below threshold: lux = 1000 < threshold = 20000; expected: bright_samples = 0, n_hours = 0 */
static void test_SunshineLux_NoSunshine(void) {
    (void)printf("\n>>> TC18: %s\n", __func__);

    SunshineLuxData sd;

    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);
    AssertStatus("SunshineLux_ResetDay", SunshineLux_ResetDay(&sd), STATUS_OK);

    for (uint32_t i = 0U; i < 60U; ++i) {
        Status s = SunshineLux_Update(&sd, 1000.0, SENSOR_VALUE_MEASURED);

        if (s != STATUS_OK) {
            char msg[80];
            snprintf(msg, sizeof(msg), "SunshineLux_Update[%u] failed: %s",
                     (unsigned)i, Status_ToString(s));
            TEST_FAIL_MESSAGE(msg);
        }
    }

    (void)printf("  SunshineLux_Update * 60        all STATUS_OK"
                 "  (lux=1000 < threshold=20000)\n");

    (void)printf("  bright_samples=%-4u  total_samples=%u\n",
                 (unsigned)sd.bright_samples, (unsigned)sd.total_samples);

    AssertStatus("SunshineLux_FinalizeDay", SunshineLux_FinalizeDay(&sd), STATUS_OK);
    AssertDouble("n_hours", sd.n_hours, 0.0, 1e-9);
}

/* TC19: mixed day, 30 bright + 30 dark -> n = 0.5 h */
static void test_SunshineLux_MixedDay(void) {
    (void)printf("\n>>> TC19: %s\n", __func__);

    SunshineLuxData sd;

    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);
    AssertStatus("SunshineLux_ResetDay", SunshineLux_ResetDay(&sd), STATUS_OK);

    for (uint32_t i = 0U; i < 30U; ++i) {
        AssertStatus("Update(bright)",
                     SunshineLux_Update(&sd, 50000.0, SENSOR_VALUE_MEASURED), STATUS_OK);
    }

    for (uint32_t i = 0U; i < 30U; ++i) {
        AssertStatus("Update(dark)",
                     SunshineLux_Update(&sd, 1000.0, SENSOR_VALUE_MEASURED), STATUS_OK);
    }

    AssertStatus("SunshineLux_FinalizeDay", SunshineLux_FinalizeDay(&sd), STATUS_OK);
    AssertDouble("n_hours", sd.n_hours, 0.5, 0.001);
}

/* *** TC20-24: solar radiation Rs, Rso *** */

/* TC20: FAO56 ex. 10, Rio de Janeiro, 15 May (J = 135);
 * reference: Ra = 25.1, N = 10.9 h, n = 7.1 h -> Rs = 14.5, Rso = 18.8 MJ m-2 day-1 */
static void test_SolarRadiation_FAO56_ex10(void) {
    (void)printf("\n>>> TC20: %s\n", __func__);

    LocationData loc;

    AssertStatus("DMS_to_decimal",
                 Location_DMS_to_decimal(TEST_RIO_LAT_DEG, TEST_RIO_LAT_MIN,
                                         &loc.latitude_deg), STATUS_OK);

    loc.latitude_rad = loc.latitude_deg * (PI / 180.0);
    loc.elevation_m  = TEST_ELEVATION_SEA_LEVEL;
    loc.initialized  = true;

    DayData            dd;  DayCalc_Init(&dd);
    RaData             rd;  RaCalc_Init(&rd);
    SolarRadiationData rsd; SolarRadiation_Init(&rsd);
    AngstromValues     ang; AngstromValues_Default(&ang);

    AssertStatus("DayCalc_Update", DayCalc_Update(&dd, TEST_EX10_J, &loc), STATUS_OK);
    AssertDouble("N [h]", dd.N_hours, TEST_EX10_N_DAYLIGHT_EXPECTED, TOL_HOURS);

    AssertStatus("Calc_Ra", Calc_Ra(&rd, &dd, &loc), STATUS_OK);
    AssertDouble("Ra [MJ m-2 day-1]", rd.Ra_daily, TEST_EX10_RA_EXPECTED, TOL_RA);

    SunshineLuxData sd;
    AssertStatus("SunshineLux_Init",
                 SunshineLux_Init(&sd, CONFIG_BRIGHT_LUX_THRESHOLD,
                                  CONFIG_SAMPLE_PERIOD_SEC), STATUS_OK);

    sd.n_hours     = TEST_EX10_N_HOURS;
    sd.initialized = true;

    AssertStatus("SolarRadiation_Calc",
                 SolarRadiation_Calc(&ang, &rsd, &rd, &dd, &sd, &loc), STATUS_OK);
                 
    AssertDouble("Rs  [MJ m-2 day-1]",  rsd.Rs_daily,  TEST_EX10_RS_EXPECTED,  TOL_RS);
    AssertDouble("Rso [MJ m-2 day-1]",  rsd.Rso_daily, TEST_EX10_RSO_EXPECTED, TOL_RSO);
}

/* TC21: polar night -> Rs = 0, Rso = 0 */
static void test_SolarRadiation_PolarNight(void) {
    (void)printf("\n>>> TC21: %s\n", __func__);

    LocationData loc;

    loc.latitude_deg = TEST_POLAR_LAT_DEG;
    loc.latitude_rad = TEST_POLAR_LAT_DEG * (PI / 180.0);
    loc.elevation_m  = 0.0;
    loc.initialized  = true;

    DayData            dd;  DayCalc_Init(&dd);
    RaData             rd;  RaCalc_Init(&rd);
    SolarRadiationData rsd; SolarRadiation_Init(&rsd);
    AngstromValues     ang; AngstromValues_Default(&ang);

    AssertStatus("DayCalc_Update", DayCalc_Update(&dd, TEST_POLAR_J, &loc), STATUS_OK);
    AssertStatus("Calc_Ra", Calc_Ra(&rd, &dd, &loc), STATUS_OK);

    SunshineLuxData sd;
    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);

    sd.n_hours     = 0.0;
    sd.initialized = true;

    AssertStatus("SolarRadiation_Calc",
                 SolarRadiation_Calc(&ang, &rsd, &rd, &dd, &sd, &loc), STATUS_OK);
                 
    AssertDouble("Rs",  rsd.Rs_daily,  0.0, 1e-9);
    AssertDouble("Rso", rsd.Rso_daily, 0.0, 1e-9);
}

/* TC22: limit n > N; result: Rs = (as + bs * 1.0) * Ra = 0.75 * Ra */
static void test_SolarRadiation_NClampedToN(void) {
    (void)printf("\n>>> TC22: %s\n", __func__);

    LocationData       loc; Location_Init(&loc);
    DayData            dd;  DayCalc_Init(&dd);
    RaData             rd;  RaCalc_Init(&rd);
    SolarRadiationData rsd; SolarRadiation_Init(&rsd);
    AngstromValues     ang; AngstromValues_Default(&ang);

    AssertStatus("DayCalc_Update", DayCalc_Update(&dd, TEST_EX8_J, &loc), STATUS_OK);
    AssertStatus("Calc_Ra", Calc_Ra(&rd, &dd, &loc), STATUS_OK);

    SunshineLuxData sd;
    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);

    sd.n_hours     = dd.N_hours + 5.0;  /* intentionally > N */
    sd.initialized = true;

    AssertStatus("SolarRadiation_Calc",
                 SolarRadiation_Calc(&ang, &rsd, &rd, &dd, &sd, &loc), STATUS_OK);
    AssertDouble("Rs limited", rsd.Rs_daily, 0.75 * rd.Ra_daily, TOL_RS);
}

/* TC23: invalid Angström coefficients, as + bs = 0.8 + 0.5 = 1.3 > 1.0 */
static void test_SolarRadiation_InvalidAngstrom(void) {
    (void)printf("\n>>> TC23: %s\n", __func__);

    LocationData       loc; Location_Init(&loc);
    DayData            dd;  DayCalc_Init(&dd);
    RaData             rd;  RaCalc_Init(&rd);
    SolarRadiationData rsd; SolarRadiation_Init(&rsd);
    AngstromValues     ang;

    ang.a_s = 0.8;
    ang.b_s = 0.5;

    AssertStatus("DayCalc_Update", DayCalc_Update(&dd, TEST_EX8_J, &loc), STATUS_OK);
    AssertStatus("Calc_Ra", Calc_Ra(&rd, &dd, &loc), STATUS_OK);

    SunshineLuxData sd;
    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);

    sd.n_hours     = 5.0;
    sd.initialized = true;

    AssertStatus("SolarRadiation_Calc(invalid ang)",
                 SolarRadiation_Calc(&ang, &rsd, &rd, &dd, &sd, &loc),
                 STATUS_INVALID_VALUE);
}

/* TC24: RaData & DayData uninitialized (initialized = false) */
static void test_SolarRadiation_UninitializedData(void) {
    (void)printf("\n>>> TC24: %s\n", __func__);

    LocationData       loc; Location_Init(&loc);
    DayData            dd;  DayCalc_Init(&dd);  /* initialized = false */
    RaData             rd;  RaCalc_Init(&rd);   /* initialized = false */
    SolarRadiationData rsd; SolarRadiation_Init(&rsd);
    AngstromValues     ang; AngstromValues_Default(&ang);

    SunshineLuxData sd;
    AssertStatus("SunshineLux_Init", SunshineLux_Init(&sd, 20000.0, 60U), STATUS_OK);

    sd.n_hours     = 5.0;
    sd.initialized = true;

    AssertStatus("SolarRadiation_Calc(uninitialized)",
                 SolarRadiation_Calc(&ang, &rsd, &rd, &dd, &sd, &loc),
                 STATUS_INVALID_VALUE);
}

/* *** TC25-27: date provider & time emulation *** */

/* TC25: DateProvider_Read, basic functionality on PC */
static void test_DateProvider_BasicPC(void) {
    (void)printf("\n>>> TC25: %s\n", __func__);

    DateData date;
    AssertStatus("DateProvider_Read", DateProvider_Read(&date), STATUS_OK);

    char msg[64];

    snprintf(msg, sizeof(msg), "year=%u not in [2024, 2099]", date.year);
    TEST_ASSERT_TRUE_MESSAGE((date.year >= 2024U) && (date.year <= 2099U), msg);

    snprintf(msg, sizeof(msg), "month=%u not in [1, 12]", (unsigned)date.month);
    TEST_ASSERT_TRUE_MESSAGE((date.month >= 1U) && (date.month <= 12U), msg);

    snprintf(msg, sizeof(msg), "day=%u not in [1, 31]", (unsigned)date.day);
    TEST_ASSERT_TRUE_MESSAGE((date.day >= 1U) && (date.day <= 31U), msg);

    uint16_t j = DayCalc_JFromDate(date.day, date.month, date.year);
    snprintf(msg, sizeof(msg), "J=%u not in [1, 366]", j);
    TEST_ASSERT_TRUE_MESSAGE((j >= 1U) && (j <= 366U), msg);

    (void)printf("  Current date: %04u-%02u-%02u  J=%u\n",
                 date.year, (unsigned)date.month, (unsigned)date.day, j);
}

/* TC26: DateProvider_Read - NULL pointer */
static void test_DateProvider_NullPointer(void) {
    (void)printf("\n>>> TC26: %s\n", __func__);

    AssertStatus("DateProvider_Read(NULL)",
                 DateProvider_Read(NULL), STATUS_NULL_POINTER);
}

/* TC27: real-time lux polling emulation
 *
 * Checks three properties independently:
 *   (a) accumulator correctly handles samples with real delays;
 *   (b) n_hours is computed algebraically and does not depend on real time;
 *   (c) timestamps are monotonically non-decreasing;
 *
 * 6 samples * 200 ms ≈ 1.2 s, this test adds ~1.2 s to the suite */
static void test_SunshineLux_RealTimeEmulation(void) {
    (void)printf("\n>>> TC27: %s\n", __func__);

    const double emul_n_expected = 3.0 * (double)CONFIG_SAMPLE_PERIOD_SEC / 3600.0;

    SunshineLuxData   sd;
    SunshineLuxSample lux_sample;
    uint32_t          timestamps[TEST_EMUL_SAMPLES];

    AssertStatus("SunshineLux_Init",
                 SunshineLux_Init(&sd, CONFIG_BRIGHT_LUX_THRESHOLD,
                                  CONFIG_SAMPLE_PERIOD_SEC), STATUS_OK);

    AssertStatus("SunshineLux_ResetDay", SunshineLux_ResetDay(&sd), STATUS_OK);

    (void)printf("  --- %u samples * %u ms ---\n",
                 TEST_EMUL_SAMPLES, TEST_EMUL_DELAY_MS);

    clock_t clk_start = clock();

    for (uint32_t i = 0U; i < TEST_EMUL_SAMPLES; ++i) {
        Status s = SensorLux_ReadInstant(&lux_sample);

        if (s != STATUS_OK) {
            (void)SensorLux_ReadDefault(&lux_sample);
        }

        /* Controlled scenario: even -> bright, odd -> dark */
        lux_sample.lux    = ((i % 2U) == 0U) ? TEST_EMUL_LUX_BRIGHT : TEST_EMUL_LUX_DARK;
        lux_sample.source = SENSOR_VALUE_MEASURED;
        timestamps[i]     = (uint32_t)lux_sample.timestamp;

        AssertStatus("SunshineLux_Update",
                     SunshineLux_Update(&sd, lux_sample.lux, lux_sample.source), STATUS_OK);
                     
        (void)printf("  [%u] lux=%.0f  bright=%u  ts=%u\n",
                     (unsigned)i, lux_sample.lux, sd.bright_samples, timestamps[i]);
                     
        SleepMs(TEST_EMUL_DELAY_MS);
    }

    double elapsed_ms = (double)(clock() - clk_start) /
                        (double)CLOCKS_PER_SEC * 1000.0;

    AssertStatus("SunshineLux_FinalizeDay", SunshineLux_FinalizeDay(&sd), STATUS_OK);

    /* (b) n_hours does not depend on real time (pure calc) */
    AssertDouble("n_hours", sd.n_hours, emul_n_expected, 0.001);

    /* (c) timestamps are monotonically non-decreasing */
    for (uint32_t i = 1U; i < TEST_EMUL_SAMPLES; ++i) {
        char msg[128];

        snprintf(msg, sizeof(msg),
                 "ts[%u]=%u < ts[%u]=%u - timestamps not monotonic",
                 (unsigned)i,        timestamps[i],
                 (unsigned)(i - 1U), timestamps[i - 1U]);

        TEST_ASSERT_TRUE_MESSAGE(timestamps[i] >= timestamps[i - 1U], msg);
    }

    (void)printf("  Real time: %.0f ms (expected ~%u ms)\n",
                 elapsed_ms, TEST_EMUL_SAMPLES * TEST_EMUL_DELAY_MS);
}

/* *** TC28: Location_Init, initialized flag check *** */
static void test_Location_Init_InitializedFlag(void) {
    (void)printf("\n>>> TC28: %s\n", __func__);

    LocationData loc;

    AssertStatus("Location_Init", Location_Init(&loc), STATUS_OK);
    TEST_ASSERT_TRUE_MESSAGE(loc.initialized, "loc.initialized must be true");
    AssertDouble("latitude_deg", loc.latitude_deg, CONFIG_LATITUDE_DEG, TOL_DEGREE);
}

/* *** TC29-32: net radiation, eq. 38, 39, 40 *** */

/* TC29: FAO56 ex. 11 (Rnl) & ex. 12 (Rns, Rn), Rio de Janeiro, May;
 *
 * inputs from ex. 11:
 * Tmax = 25.1C, Tmin = 19.1C, ea = 2.1 kPa;
 * Rs/Rso values taken from ex. 10;
 *
 * checks:
 * STATUS_OK, Rns ≈ 11.1, Rnl ≈ 3.5, Rn ≈ 7.6 MJ m-2 day-1 */
static void test_Calc_NetRadiation_FAO56_ex11_12(void) {
    (void)printf("\n>>> TC29: %s\n", __func__);

    /* Temperature: two readings set Tmax & Tmin */
    AirTemperatureData temp;
    AirTemperature_Init(&temp);

    AssertStatus("AirTemperature_Update(Tmax = 25.1)",
                 AirTemperature_Update(&temp, TEST_EX11_TMAX_C, 0U), STATUS_OK);

    AssertStatus("AirTemperature_Update(Tmin = 19.1)",
                 AirTemperature_Update(&temp, TEST_EX11_TMIN_C, 1U), STATUS_OK);

    /* From known values of ex.10 test eq. 39 in isolation */
    SolarRadiationData solar;
    SolarRadiation_Init(&solar);
    solar.Rs_daily  = TEST_EX10_RS_EXPECTED;   /* 14.5 MJ m-2 day-1 */
    solar.Rso_daily = TEST_EX10_RSO_EXPECTED;  /* 18.8 MJ m-2 day-1 */

    NetRadiationData net;
    NetRadiation_Init(&net);

    AssertStatus("Calc_NetRadiation",
                 Calc_NetRadiation(&net, &temp, &solar, TEST_EX11_EA_KPA), STATUS_OK);

    AssertDouble("Rns [MJ m-2 day-1]", net.Rns_daily, TEST_EX12_RNS_EXPECTED, TOL_RNS);
    AssertDouble("Rnl [MJ m-2 day-1]", net.Rnl_daily, TEST_EX11_RNL_EXPECTED, TOL_RNL);
    AssertDouble("Rn  [MJ m-2 day-1]", net.Rn_daily,  TEST_EX12_RN_EXPECTED,  TOL_RN);
}

/* TC30: Calc_NetRadiation, STATUS_NULL_POINTER for each argument */
static void test_Calc_NetRadiation_NullPointer(void) {
    (void)printf("\n>>> TC30: %s\n", __func__);

    AirTemperatureData temp;  AirTemperature_Init(&temp);
    SolarRadiationData solar; SolarRadiation_Init(&solar);
    NetRadiationData   net;   NetRadiation_Init(&net);

    AssertStatus("Calc_NetRadiation(out=NULL)",
                 Calc_NetRadiation(NULL,  &temp,  &solar, 2.0), STATUS_NULL_POINTER);

    AssertStatus("Calc_NetRadiation(temp=NULL)",
                 Calc_NetRadiation(&net,  NULL,   &solar, 2.0), STATUS_NULL_POINTER);

    AssertStatus("Calc_NetRadiation(solar=NULL)",
                 Calc_NetRadiation(&net,  &temp,  NULL,   2.0), STATUS_NULL_POINTER);
}

/* TC31: Calc_NetRadiation, STATUS_INVALID_VALUE when temp uninitialized;
 * AirTemperature_Init sets initialized = false;
 * without calling AirTemperature_Update temperature data is not ready */
static void test_Calc_NetRadiation_UninitializedTemp(void) {
    (void)printf("\n>>> TC31: %s\n", __func__);

    AirTemperatureData temp;  AirTemperature_Init(&temp);   /* initialized = false */
    SolarRadiationData solar; SolarRadiation_Init(&solar);

    solar.Rs_daily  = TEST_EX10_RS_EXPECTED;
    solar.Rso_daily = TEST_EX10_RSO_EXPECTED;

    NetRadiationData   net;   NetRadiation_Init(&net);

    /* temp.initialized = false -> STATUS_INVALID_VALUE */
    AssertStatus("Calc_NetRadiation(temp uninitialized)",
                 Calc_NetRadiation(&net, &temp, &solar, 2.0), STATUS_INVALID_VALUE);
}

/* TC32: Calc_NetRadiation, STATUS_INVALID_VALUE for invalid ea (ea < 0 physically impossible, vapour pressure ≥ 0) */
static void test_Calc_NetRadiation_InvalidEa(void) {
    (void)printf("\n>>> TC32: %s\n", __func__);

    AirTemperatureData temp;
    AirTemperature_Init(&temp);

    AssertStatus("Update(Tmax)", AirTemperature_Update(&temp, TEST_EX11_TMAX_C, 0U),
                 STATUS_OK);
    AssertStatus("Update(Tmin)", AirTemperature_Update(&temp, TEST_EX11_TMIN_C, 1U),
                 STATUS_OK);

    SolarRadiationData solar; SolarRadiation_Init(&solar);
    solar.Rs_daily  = TEST_EX10_RS_EXPECTED;
    solar.Rso_daily = TEST_EX10_RSO_EXPECTED;

    NetRadiationData   net; NetRadiation_Init(&net);

    AssertStatus("Calc_NetRadiation(ea=-0.5)",
                 Calc_NetRadiation(&net, &temp, &solar, -0.5), STATUS_INVALID_VALUE);
}

/* *** TC33-TC40: air humidity, atmospheric pressure, psychrometry, vapour pressure *** */

/* TC33: AirHumidity_Update, min/max/mean accumulation;
 * two inputs: 82% (morning) & 54% (day) -> RHmax = 82, RHmin = 54, RHmean = 68 */
static void test_AirHumidity_Update_MinMaxTracking(void) {
    (void)printf("\n>>> TC33: %s\n", __func__);

    AirHumidityData data;
    AirHumidity_Init(&data);

    AssertStatus("AirHumidity_Update(82%)",
                 AirHumidity_Update(&data, TEST_EX5_RH_MAX, 0U), STATUS_OK);
                 
    AssertDouble("RH_max after 1st", data.RH_max, TEST_EX5_RH_MAX, 0.001);
    AssertDouble("RH_min after 1st", data.RH_min, TEST_EX5_RH_MAX, 0.001);

    AssertStatus("AirHumidity_Update(54%)",
                 AirHumidity_Update(&data, TEST_EX5_RH_MIN, 1U), STATUS_OK);
                 
    AssertDouble("RH_max",  data.RH_max,  TEST_EX5_RH_MAX,        0.001);
    AssertDouble("RH_min",  data.RH_min,  TEST_EX5_RH_MIN,        0.001);
    AssertDouble("RH_mean", data.RH_mean, TEST_RH_MEAN_EXPECTED,  0.001);
}

/* TC34: Calc_PressureFromElevation, sea level: P = 101.3 kPa */
static void test_Calc_PressureFromElevation_SeaLevel(void) {
    (void)printf("\n>>> TC34: %s\n", __func__);

    double P = 0.0;
    AssertStatus("Calc_PressureFromElevation(z = 0)",
                 Calc_PressureFromElevation(TEST_ELEVATION_SEA_LEVEL, &P), STATUS_OK);
                 
    AssertDouble("P [kPa]", P, TEST_P_SEA_LEVEL_EXPECTED, TOL_P);
}

/* TC35: Calc_PressureFromElevation, FAO56 ex. 2: z = 1800m -> P = 81.8 kPa */
static void test_Calc_PressureFromElevation_FAO56_ex2(void) {
    (void)printf("\n>>> TC35: %s\n", __func__);

    double P = 0.0;
    AssertStatus("Calc_PressureFromElevation(z = 1800)",
                 Calc_PressureFromElevation(TEST_EX2_ELEVATION_M, &P), STATUS_OK);
                 
    AssertDouble("P [kPa]", P, TEST_EX2_P_EXPECTED, TOL_P);
}

/* TC36: Calc_AtmosphericParameters, sea level: P = 101.3 -> γ ≈ 0.0674 */
static void test_Calc_AtmosphericParameters_SeaLevel(void) {
    (void)printf("\n>>> TC36: %s\n", __func__);

    AtmosphericData atmos;
    AtmosphericData_Init(&atmos);

    AssertStatus("Calc_AtmosphericParameters(P = 101.3)",
                 Calc_AtmosphericParameters(&atmos, TEST_P_SEA_LEVEL_EXPECTED), STATUS_OK);
                 
    AssertDouble("P [kPa]",        atmos.P_kPa,           TEST_P_SEA_LEVEL_EXPECTED,     TOL_P);
    AssertDouble("gamma [kPa/C]",  atmos.gamma_kPa_per_C, TEST_GAMMA_SEA_LEVEL_EXPECTED, TOL_GAMMA);
}

/* TC37: Calc_AtmosphericParameters - FAO56 ex. 2: P = 81.8 -> γ = 0.054 */
static void test_Calc_AtmosphericParameters_FAO56_ex2(void) {
    (void)printf("\n>>> TC37: %s\n", __func__);

    AtmosphericData atmos;
    AtmosphericData_Init(&atmos);

    AssertStatus("Calc_AtmosphericParameters(P = 81.8)",
                 Calc_AtmosphericParameters(&atmos, TEST_EX2_P_EXPECTED), STATUS_OK);
                 
    AssertDouble("P [kPa]",        atmos.P_kPa,           TEST_EX2_P_EXPECTED,     TOL_P);
    AssertDouble("gamma [kPa/C]",  atmos.gamma_kPa_per_C, TEST_EX2_GAMMA_EXPECTED, TOL_GAMMA);
}

/* TC38: NULL pointer, Calc_PressureFromElevation & Calc_AtmosphericParameters */
static void test_Calc_AtmosphericParameters_NullPointer(void) {
    (void)printf("\n>>> TC38: %s\n", __func__);

    AssertStatus("Calc_PressureFromElevation(NULL)",
                 Calc_PressureFromElevation(0.0, NULL),         STATUS_NULL_POINTER);
    AssertStatus("Calc_AtmosphericParameters(NULL)",
                 Calc_AtmosphericParameters(NULL, 101.3), STATUS_NULL_POINTER);
}

/* TC39: Calc_SaturationVapourPressure, FAO56 ex. 3 - e(24.5C) = 3.075 kPa, e(15.0C) = 1.705 kPa */
static void test_Calc_SaturationVapourPressure_FAO56_ex3(void) {
    (void)printf("\n>>> TC39: %s\n", __func__);

    double e_max = 0.0;
    double e_min = 0.0;

    AssertStatus("Calc_SVP(Tmax = 24.5)",
                 Calc_SaturationVapourPressure(TEST_EX3_TMAX_C, &e_max), STATUS_OK);
    AssertDouble("e(Tmax) [kPa]", e_max, TEST_EX3_E_TMAX_EXPECTED, TOL_E_SAT);

    AssertStatus("Calc_SVP(Tmin = 15.0)",
                 Calc_SaturationVapourPressure(TEST_EX3_TMIN_C, &e_min), STATUS_OK);
    AssertDouble("e(Tmin) [kPa]", e_min, TEST_EX3_E_TMIN_EXPECTED, TOL_E_SAT);
}

/* TC40: Calc_ActualVapourPressure, FAO56 ex. 5 - Tmin = 18C, Tmax = 25C, RHmax = 82%, RHmin = 54% -> ea = 1.70 kPa */
static void test_Calc_ActualVapourPressure_FAO56_ex5(void) {
    (void)printf("\n>>> TC40: %s\n", __func__);

    /* Temperature: two readings set Tmax & Tmin */
    AirTemperatureData temp;
    AirTemperature_Init(&temp);

    AssertStatus("Update(Tmax = 25)", AirTemperature_Update(&temp, TEST_EX5_TMAX_C, 0U),
                 STATUS_OK);
    AssertStatus("Update(Tmin = 18)", AirTemperature_Update(&temp, TEST_EX5_TMIN_C, 1U),
                 STATUS_OK);

    /* Humidity: two readings set RHmax & RHmin */
    AirHumidityData humidity;
    AirHumidity_Init(&humidity);

    AssertStatus("Update(RHmax = 82)", AirHumidity_Update(&humidity, TEST_EX5_RH_MAX, 0U),
                 STATUS_OK);
    AssertStatus("Update(RHmin = 54)", AirHumidity_Update(&humidity, TEST_EX5_RH_MIN, 1U),
                 STATUS_OK);

    double ea = 0.0;
    AssertStatus("Calc_ActualVapourPressure",
                 Calc_ActualVapourPressure(&ea, &temp, &humidity), STATUS_OK);
    AssertDouble("ea [kPa]", ea, TEST_EX5_EA_EXPECTED, TOL_EA);
}

/* *** TC41-TC46: Wind speed (eq. 47) *** */

/* TC41: Calc_WindSpeedAt2m - normal path.
 * FAO56 ann.2, tab.2.9 / eq.47 / ex. 14: uz = 3.2 m/s at z = 10 m -> u2 = 2.393 m/s.
 * Conversion factor: 4.87 / ln(672.58) = 4.87 / 6.511 ≈ 0.748 (matches tab.2.9). */
static void test_Calc_WindSpeedAt2m_FAO56(void) {
    (void)printf("\n>>> TC41: %s\n", __func__);

    double u2 = 0.0;

    AssertStatus("Calc_WindSpeedAt2m(3.2, 10.0)",
                 Calc_WindSpeedAt2m(TEST_WIND_UZ_MS, TEST_WIND_HEIGHT_M, &u2), STATUS_OK);
    AssertDouble("u2 [m/s]", u2, TEST_WIND_U2_EXPECTED, TOL_WIND_MS);
}

/* TC42: Calc_WindSpeedAt2m, at z = 2 m conversion factor ≈ 1.0;
 * numerator & denominator of eq. 47 match: ln(130.18) ≈ 4.869 ≈ 4.87;
 * if anemometer at 2 m (FAO standard), conversion does not change the value */
static void test_Calc_WindSpeedAt2m_At2m(void) {
    (void)printf("\n>>> TC42: %s\n", __func__);

    double u2 = 0.0;

    AssertStatus("Calc_WindSpeedAt2m(5.0, 2.0)",
                 Calc_WindSpeedAt2m(TEST_WIND_AT2M_UZ_MS, 2.0, &u2), STATUS_OK);
    AssertDouble("u2 ≈ uz at z = 2 m", u2, TEST_WIND_AT2M_EXPECTED, TOL_WIND_MS);
}

/* TC43: WindSpeed_Init + WindSpeed_Update * 3 -> min/max/mean accumulation; then eq. 47;
 * samples: 2.0, 4.0, 3.0 m/s at z = 10 m;
 * expected: min = 2.0, max = 4.0, mean = 3.0; u2 = 3.0 * 0.748 = 2.244 m/s */
static void test_WindSpeed_Update_Accumulation(void) {
    (void)printf("\n>>> TC43: %s\n", __func__);

    WindSpeedData data;

    AssertStatus("WindSpeed_Init", WindSpeed_Init(&data), STATUS_OK);
    TEST_ASSERT_FALSE_MESSAGE(data.initialized, "initialized must be false after Init");

    AssertStatus("Update(2.0, 10.0)",
                 WindSpeed_Update(&data, TEST_WIND_SAMPLE_MS_LOW,  TEST_WIND_HEIGHT_M, 0U),
                 STATUS_OK);

    TEST_ASSERT_TRUE_MESSAGE(data.initialized,
                             "initialized must become true after first Update");

    AssertStatus("Update(4.0, 10.0)",
                 WindSpeed_Update(&data, TEST_WIND_SAMPLE_MS_HIGH, TEST_WIND_HEIGHT_M, 1U),
                 STATUS_OK);

    AssertStatus("Update(3.0, 10.0)",
                 WindSpeed_Update(&data, TEST_WIND_SAMPLE_MS_MID,  TEST_WIND_HEIGHT_M, 2U),
                 STATUS_OK);

    AssertDouble("u_z_min",  data.u_z_min_m_s,  TEST_WIND_SAMPLE_MS_LOW,  TOL_WIND_MS);
    AssertDouble("u_z_max",  data.u_z_max_m_s,  TEST_WIND_SAMPLE_MS_HIGH, TOL_WIND_MS);
    AssertDouble("u_z_mean", data.u_z_mean_m_s, TEST_WIND_MEAN_EXPECTED,  TOL_WIND_MS);

    double u2 = 0.0;

    AssertStatus("Calc_WindSpeedAt2m(mean, height)",
                 Calc_WindSpeedAt2m(data.u_z_mean_m_s, data.height_m, &u2), STATUS_OK);
    AssertDouble("u2 [m/s]", u2, TEST_WIND_MEAN_U2_EXPECTED, TOL_WIND_MS);
}

/* TC44: NULL pointer, all three functions of the module */
static void test_WindSpeed_NullPointer(void) {
    (void)printf("\n>>> TC44: %s\n", __func__);

    double u2 = 0.0;

    AssertStatus("WindSpeed_Init(NULL)", WindSpeed_Init(NULL), STATUS_NULL_POINTER);
    AssertStatus("WindSpeed_Update(data=NULL)",
                 WindSpeed_Update(NULL, 3.2, 10.0, 0U), STATUS_NULL_POINTER);
    AssertStatus("Calc_WindSpeedAt2m(out=NULL)",
                 Calc_WindSpeedAt2m(3.2, 10.0, NULL), STATUS_NULL_POINTER);

    /* u2 must not change (Calc would return error before writing) */
    AssertDouble("u2 unchanged", u2, 0.0, 1e-9);
}

/* TC45: STATUS_INVALID_VALUE, all invalid input paths;
 * speed: negative, NaN, INFINITY;
 * height: below 0.1 m (eq. 47 "degenerates") & above 200 m */
static void test_Calc_WindSpeedAt2m_InvalidValues(void) {
    (void)printf("\n>>> TC45: %s\n", __func__);

    double u2 = 0.0;

    AssertStatus("speed < 0",
                 Calc_WindSpeedAt2m(-1.0,  TEST_WIND_HEIGHT_M, &u2), STATUS_INVALID_VALUE);
    AssertStatus("speed = NaN",
                 Calc_WindSpeedAt2m(NAN, TEST_WIND_HEIGHT_M, &u2), STATUS_INVALID_VALUE);
    AssertStatus("speed = INFINITY",
                 Calc_WindSpeedAt2m(INFINITY,  TEST_WIND_HEIGHT_M, &u2), STATUS_INVALID_VALUE);
    AssertStatus("z < 0.1 m",
                 Calc_WindSpeedAt2m(TEST_WIND_UZ_MS,  TEST_WIND_BAD_HEIGHT_LOW_M,  &u2),
                 STATUS_INVALID_VALUE);
    AssertStatus("z > 200 m",
                 Calc_WindSpeedAt2m(TEST_WIND_UZ_MS,  TEST_WIND_BAD_HEIGHT_HIGH_M, &u2),
                 STATUS_INVALID_VALUE);
}

/* TC46: WindSpeed_Update - height mismatch on repeated call; physically anemometer height is constant;
 * changing height signals a configuration error; after a rejected Update, data must remain untouched */
static void test_WindSpeed_Update_HeightMismatch(void) {
    (void)printf("\n>>> TC46: %s\n", __func__);

    WindSpeedData data;

    AssertStatus("WindSpeed_Init", WindSpeed_Init(&data), STATUS_OK);

    AssertStatus("Update(3.0, z = 10.0) - first",
                 WindSpeed_Update(&data, TEST_WIND_SAMPLE_MS_MID, TEST_WIND_HEIGHT_M, 0U),
                 STATUS_OK);

    /* Same speed but different height: return error */
    AssertStatus("Update(2.0, z = 2.0) - height changed",
                 WindSpeed_Update(&data, TEST_WIND_SAMPLE_MS_LOW, TEST_WIND_HEIGHT_SECOND_M, 1U),
                 STATUS_INVALID_VALUE);

    /* Data must not have changed after rejected Update */
    AssertDouble("u_z_mean unchanged after error",
                 data.u_z_mean_m_s, TEST_WIND_SAMPLE_MS_MID, TOL_WIND_MS);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1U, data.sample_count,
                                     "sample_count must not have increased after error");
}

/* *** TC47-TC53: evapotranspiration (eq. 6, eq. 56) *** */

/* TC47: Calc_ETo, normal path (G = 0, u2 = 2.0); composite scenario from FAO56 examples;
 * ETo = 0.7130 / 0.2579 = 2.764 mm/day (see test-config.h) */
static void test_Calc_ETo_Normal(void) {
    (void)printf("\n>>> TC47: %s\n", __func__);

    double eto = 0.0;

    AssertStatus("Calc_ETo(G=0, u2=2.0)",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_MS,
                          TEST_ETO_ES_KPA, TEST_ETO_EA_KPA, &eto),
                 STATUS_OK);

    AssertDouble("ETo [mm/day]", eto, TEST_ETO_EXPECTED, TOL_ETO);
}

/* TC48: Calc_ETo, G ≠ 0 (accounting for daily soil heat flux);
 * tests the G parameter; Rn - G = 7.6 - 1.0 = 6.6 instead of 7.6; ETo = 2.535 mm/day */
static void test_Calc_ETo_WithSoilHeatFlux(void) {
    (void)printf("\n>>> TC48: %s\n", __func__);

    double eto = 0.0;

    AssertStatus("Calc_ETo(G=1.0)",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_NON_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_MS,
                          TEST_ETO_ES_KPA, TEST_ETO_EA_KPA, &eto),
                 STATUS_OK);

    AssertDouble("ETo with G ≠ 0 [mm/day]", eto, TEST_ETO_G_NONZERO_EXPECTED, TOL_ETO);
}

/* TC49: Calc_ETo, calm/Stille (u2 = 0);
 * aerodynamic term vanishes: ETo = 0.408 * Δ * Rn / (Δ + γ) = 2.115 mm/day;
 * denominator with u2 = 0: Δ + γ * 1 > 0, no degeneration */
static void test_Calc_ETo_CalmWind(void) {
    (void)printf("\n>>> TC49: %s\n", __func__);

    double eto = 0.0;

    AssertStatus("Calc_ETo(u2 = 0, calm)",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_CALM,
                          TEST_ETO_ES_KPA, TEST_ETO_EA_KPA, &eto),
                 STATUS_OK);

    AssertDouble("ETo in calm [mm/day]", eto, TEST_ETO_CALM_EXPECTED, TOL_ETO);
}

/* TC50: Calc_ETo, negative Rn, result clamped to 0;
 * with Rn = -5.0: numerator = -0.295 + 0.264 = -0.031 < 0 -> ETo ≈ -0.120 -> clamp to 0 */
static void test_Calc_ETo_NegativeRn(void) {
    (void)printf("\n>>> TC50: %s\n", __func__);

    double eto = 0.0;

    AssertStatus("Calc_ETo(Rn < 0)",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_NEGATIVE, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_MS,
                          TEST_ETO_ES_KPA, TEST_ETO_EA_KPA, &eto),
                 STATUS_OK);

    AssertDouble("ETo clamped to 0 [mm/day]", eto, TEST_ETO_NEGATIVE_EXPECTED, TOL_ETO);
}

/* TC51: Calc_ETo, NULL pointer & all STATUS_INVALID_VALUE paths */
static void test_Calc_ETo_Errors(void) {
    (void)printf("\n>>> TC51: %s\n", __func__);

    double eto = 0.0;

    AssertStatus("out = NULL",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_MS,
                          TEST_ETO_ES_KPA, TEST_ETO_EA_KPA, NULL),
                 STATUS_NULL_POINTER);

    AssertStatus("delta <= 0",
                 Calc_ETo(0.0, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO, TEST_ETO_GAMMA_KPA_C,
                          TEST_ETO_TMEAN_C, TEST_ETO_U2_MS, TEST_ETO_ES_KPA,
                          TEST_ETO_EA_KPA, &eto),
                 STATUS_INVALID_VALUE);

    AssertStatus("gamma <= 0",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO, 0.0,
                          TEST_ETO_TMEAN_C, TEST_ETO_U2_MS, TEST_ETO_ES_KPA,
                          TEST_ETO_EA_KPA, &eto),
                 STATUS_INVALID_VALUE);

    AssertStatus("u2 < 0",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, -1.0,
                          TEST_ETO_ES_KPA, TEST_ETO_EA_KPA, &eto),
                 STATUS_INVALID_VALUE);

    AssertStatus("ea < 0",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_MS,
                          TEST_ETO_ES_KPA, -1.0, &eto),
                 STATUS_INVALID_VALUE);

    AssertStatus("es <= 0",
                 Calc_ETo(TEST_ETO_DELTA_KPA_C, TEST_ETO_RN_MJ, TEST_ETO_G_ZERO,
                          TEST_ETO_GAMMA_KPA_C, TEST_ETO_TMEAN_C, TEST_ETO_U2_MS,
                          0.0, TEST_ETO_EA_KPA, &eto),
                 STATUS_INVALID_VALUE);
}

/* TC52: Calc_ETc, normal path;
 * ETc = Kc * ETo = 1.15 * 2.764 = 3.179 mm/day */
static void test_Calc_ETc_Normal(void) {
    (void)printf("\n>>> TC52: %s\n", __func__);

    double etc = 0.0;

    AssertStatus("Calc_ETc(2.764, 1.15)",
                 Calc_ETc(TEST_ETC_ETO_MM, TEST_ETC_KC, &etc), STATUS_OK);
    AssertDouble("ETc [mm/day]", etc, TEST_ETC_EXPECTED, TOL_ETC);
}

/* TC53: Calc_ETc, NULL pointer & invalid inputs */
static void test_Calc_ETc_Errors(void) {
    (void)printf("\n>>> TC53: %s\n", __func__);

    double etc = 0.0;

    AssertStatus("out = NULL",  Calc_ETc(2.764, 1.0,  NULL), STATUS_NULL_POINTER);
    AssertStatus("eto < 0",     Calc_ETc(-1.0,  1.0,  &etc), STATUS_INVALID_VALUE);
    AssertStatus("kc = 0",      Calc_ETc(2.764, 0.0,  &etc), STATUS_INVALID_VALUE);
    AssertStatus("kc < 0",      Calc_ETc(2.764, -0.5, &etc), STATUS_INVALID_VALUE);
}

/* *** * * * ****** * * ******* * * *** * *** * * * *** * * ***** * * * **** *** *
 * main() is Unity runner
 * UNITY_BEGIN() opens the session
 * RUN_TEST() registers & executes each test
 * UNITY_END() prints result & returns 0 (all passed) or 1 (errors)
 * * * * ****** * * **** * * ** * * ** **** * *** **** * ***** * *** * ** **** * */

int main(void) {
    UNITY_BEGIN();

    /* Temperature & vapour pressure */
    RUN_TEST(test_AirTemperature_NormalPath_T20);
    RUN_TEST(test_AirTemperature_OutOfRange);
    RUN_TEST(test_AirTemperature_NaN);
    RUN_TEST(test_AirTemperature_Infinity);
    RUN_TEST(test_AirTemperature_MinMaxTracking);

    /* Validation & date conversion */
    RUN_TEST(test_ValidDayOfYear_BoundaryValues);
    RUN_TEST(test_ValidLatitudeRad_BoundaryValues);
    RUN_TEST(test_DayCalc_JFromDate_CalendarDates);

    /* DayCalc_Update: errors */
    RUN_TEST(test_DayCalc_Update_NullPointer);
    RUN_TEST(test_DayCalc_Update_InvalidJ);
    RUN_TEST(test_DayCalc_Update_InvalidLatitude);

    /* Geolocation */
    RUN_TEST(test_Location_DMS_Bangkok);
    RUN_TEST(test_Location_DMS_Rio);

    /* Extraterrestrial radiation */
    RUN_TEST(test_Calc_Ra_FAO56_ex8);
    RUN_TEST(test_Calc_Ra_PolarNight);
    RUN_TEST(test_Calc_Ra_UninitializedDayData);

    /* Sunshine */
    RUN_TEST(test_SunshineLux_FullSunshine);
    RUN_TEST(test_SunshineLux_NoSunshine);
    RUN_TEST(test_SunshineLux_MixedDay);

    /* Solar radiation */
    RUN_TEST(test_SolarRadiation_FAO56_ex10);
    RUN_TEST(test_SolarRadiation_PolarNight);
    RUN_TEST(test_SolarRadiation_NClampedToN);
    RUN_TEST(test_SolarRadiation_InvalidAngstrom);
    RUN_TEST(test_SolarRadiation_UninitializedData);

    /* Date provider & time */
    RUN_TEST(test_DateProvider_BasicPC);
    RUN_TEST(test_DateProvider_NullPointer);
    RUN_TEST(test_SunshineLux_RealTimeEmulation);

    /* Geolocation: initialized */
    RUN_TEST(test_Location_Init_InitializedFlag);

    /* Net radiation */
    RUN_TEST(test_Calc_NetRadiation_FAO56_ex11_12);
    RUN_TEST(test_Calc_NetRadiation_NullPointer);
    RUN_TEST(test_Calc_NetRadiation_UninitializedTemp);
    RUN_TEST(test_Calc_NetRadiation_InvalidEa);

    /* Air humid., atm. press., psychrom., vap. press. */
    RUN_TEST(test_AirHumidity_Update_MinMaxTracking);
    RUN_TEST(test_Calc_PressureFromElevation_SeaLevel);
    RUN_TEST(test_Calc_PressureFromElevation_FAO56_ex2);
    RUN_TEST(test_Calc_AtmosphericParameters_SeaLevel);
    RUN_TEST(test_Calc_AtmosphericParameters_FAO56_ex2);
    RUN_TEST(test_Calc_AtmosphericParameters_NullPointer);
    RUN_TEST(test_Calc_SaturationVapourPressure_FAO56_ex3);
    RUN_TEST(test_Calc_ActualVapourPressure_FAO56_ex5);

    /* Wind speed */
    RUN_TEST(test_Calc_WindSpeedAt2m_FAO56);
    RUN_TEST(test_Calc_WindSpeedAt2m_At2m);
    RUN_TEST(test_WindSpeed_Update_Accumulation);
    RUN_TEST(test_WindSpeed_NullPointer);
    RUN_TEST(test_Calc_WindSpeedAt2m_InvalidValues);
    RUN_TEST(test_WindSpeed_Update_HeightMismatch);

    /* Evapotranspiration */
    RUN_TEST(test_Calc_ETo_Normal);
    RUN_TEST(test_Calc_ETo_WithSoilHeatFlux);
    RUN_TEST(test_Calc_ETo_CalmWind);
    RUN_TEST(test_Calc_ETo_NegativeRn);
    RUN_TEST(test_Calc_ETo_Errors);
    RUN_TEST(test_Calc_ETc_Normal);
    RUN_TEST(test_Calc_ETc_Errors);

    return UNITY_END();
}
