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

/* Zero-initialize the structure */
Status DayCalc_Init(DayData* data);

/* Compute all DayData fields from J and loc */
Status DayCalc_Update(DayData* data, uint16_t J, const LocationData* loc);

/* Helper utility: day of year from calendar date;
 * accounts for leap years; pure function with no side effects;
 * input range is not validated, responsibility lies with the caller */
uint16_t DayCalc_JFromDate(uint8_t day, uint8_t month, uint16_t year);

#ifdef __cplusplus
}
#endif

#endif /* DAY_IN_YEAR_CALC_H */
