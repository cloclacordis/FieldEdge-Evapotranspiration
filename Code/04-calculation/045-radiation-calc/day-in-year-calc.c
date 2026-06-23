/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <stddef.h>
#include "day-in-year-calc.h"
#include "../../03-validation/032-validation/validation.h"

#define DAY_CALC_PI            (3.14159265358979323846)
#define DAY_CALC_TWO_PI_365    (0.01721420632103996)    /* 2pi / 365 ** * ** ** **** * * */

/* FAO56 constants */
#define DR_AMPLITUDE           (0.033)   /* Eq. 23: Earth orbit eccentricity coefficient */
#define SOLAR_DECLIN_AMPLITUDE (0.409)   /* Eq. 24: Earth's axial tilt [rad] *** * *** * */
#define SOLAR_DECLIN_PHASE     (1.39)    /* Eq. 24: Phase shift in Cooper equation [rad] */

Status DayCalc_Init(DayData* data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }
    
    data->J            = 0U;
    data->dr           = 0.0;
    data->delta_rad    = 0.0;
    data->omega_s_rad  = 0.0;
    data->N_hours      = 0.0;
    data->initialized  = false;

    return STATUS_OK;
}

Status DayCalc_Update(DayData* data, const uint16_t J, const LocationData* loc) {
    if ((data == NULL) || (loc == NULL)) {
        return STATUS_NULL_POINTER;
    }

    if (!loc->initialized) {
        return STATUS_INVALID_VALUE;
    }

    if (!ValidDayOfYear(J)) {
        return STATUS_INVALID_VALUE;
    }

    if (!ValidLatitudeRad(loc->latitude_rad)) {
        return STATUS_INVALID_VALUE;
    }

    const double angle_rad = DAY_CALC_TWO_PI_365 * (double)J;

    /* Inverse relative distance Earth-Sun (eq. 23) */
    const double dr = 1.0 + DR_AMPLITUDE * cos(angle_rad);

    /* Solar declination [rad] (eq. 24) */
    const double delta = SOLAR_DECLIN_AMPLITUDE * sin(angle_rad - SOLAR_DECLIN_PHASE);

    /* Sunset hour angle [rad] (eq. 25); arccos argument may fall outside [-1, 1] at polar latitudes:
     * arg > +1 -> polar night -> ωs = 0, N = 0, Ra = 0; arg < -1 -> polar day -> ωs = π, N = 24 h */
    const double arg = -tan(loc->latitude_rad) * tan(delta);
    double omega_s;
    
    if (arg > 1.0) {
        omega_s = 0.0;           /* Polar night */
    } else if (arg < -1.0) {
        omega_s = DAY_CALC_PI;   /* Polar day * */
    } else {
        omega_s = acos(arg);
    }

    /* Maximum daylight duration [hour] (eq. 34) */
    const double N = (24.0 / DAY_CALC_PI) * omega_s;

    data->J            = J;
    data->dr           = dr;
    data->delta_rad    = delta;
    data->omega_s_rad  = omega_s;
    data->N_hours      = N;
    data->initialized  = true;

    return STATUS_OK;
}

uint16_t DayCalc_JFromDate(const uint8_t day, const uint8_t month, const uint16_t year) {
    /* Sum days of elapsed months + current day; standard method with leap year correction */
    static const uint16_t days_before_month[13] = {
        0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    /* Leap year: divisible by 4, but not by 100, or divisible by 400 */
    const bool is_leap = ((year % 4U == 0U) && (year % 100U != 0U)) || (year % 400U == 0U);

    uint16_t J = days_before_month[month] + (uint16_t)day;

    if (is_leap && (month > 2U)) {
        J += 1U;
    }

    return J;
}
