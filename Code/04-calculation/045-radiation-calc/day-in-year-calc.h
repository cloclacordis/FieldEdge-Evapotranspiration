/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef DAY_IN_YEAR_CALC_H
#define DAY_IN_YEAR_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "geolocation-calc.h"
#include "../../03-validation/033-status/status.h"

/* Astronomical derivatives: day of year & location; all angles in radians;
 * all values are computed from J and latitude φ; no measurements required */
typedef struct {
    uint16_t J;            /* Day of year [1...366] ** * ***** * * * *** * */
    double   dr;           /* Inverse relative distance Earth-Sun (eq. 23) */
    double   delta_rad;    /* Solar declination [rad] (eq. 24) * *** * * * */
    double   omega_s_rad;  /* Sunset hour angle [rad] (eq. 25) *** * * *** */
    double   N_hours;      /* Maximum daylight duration [hour] (eq. 34) ** */
    bool     initialized;
} DayData;

/**
 * @brief Initializes day/astronomy data to a safe zero state.
 *
 * @param[out] data Pointer to the DayData structure to initialize.
 *                  Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status DayCalc_Init(DayData* data);

/**
 * @brief Computes all astronomical day quantities for a day of year
 *        and latitude (FAO-56, eq. 23-25, 34).
 *
 * Inverse relative distance Earth-Sun dr (eq. 23), solar declination
 * delta (eq. 24, adapted Cooper's equation), sunset hour angle omega_s
 * (eq. 25), and daylight hours N (eq. 34). At polar latitudes, the eq. 25
 * arccos argument can fall outside [-1, 1]: this is handled, not rejected -
 * argument > 1 (polar night) yields omega_s = 0, N = 0; argument < -1
 * (polar day) yields omega_s = pi, N = 24.
 *
 * @param[in,out] data Structure to update. Must not be NULL.
 * @param[in]     J    Day of year. Must satisfy ValidDayOfYear().
 * @param[in]     loc  Location data. Must not be NULL;
 *                     `.initialized` must be true and
 *                     `.latitude_rad` must satisfy ValidLatitudeRad().
 *
 * @retval STATUS_OK            *data is valid for every latitude in
 *                              the accepted range, including the poles.
 * @retval STATUS_NULL_POINTER  data or loc was NULL.
 * @retval STATUS_INVALID_VALUE loc not initialized, J invalid, or
 *                              loc->latitude_rad invalid.
 */
Status DayCalc_Update(DayData* data, uint16_t J, const LocationData* loc);

/**
 * @brief Converts a calendar date to day-of-year, with leap-year
 *        correction.
 *
 * Pure function, no side effects. Does not validate its inputs - the
 * caller is responsible for passing a plausible date (no Status
 * return is available to report a problem).
 *
 * @param[in] day   Day of month.
 * @param[in] month Month (1-12).
 * @param[in] year  Calendar year, used only to test the leap-year rule.
 *
 * @return Day of year (1-366).
 */
uint16_t DayCalc_JFromDate(uint8_t day, uint8_t month, uint16_t year);

#ifdef __cplusplus
}
#endif

#endif /* DAY_IN_YEAR_CALC_H */
