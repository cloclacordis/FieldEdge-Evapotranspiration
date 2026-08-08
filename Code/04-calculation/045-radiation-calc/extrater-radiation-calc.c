/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <string.h>
#include "extrater-radiation-calc.h"
#include "../../03-validation/034-math-utils/math-utils.h"

#define SOLAR_CONSTANT_GSC    (0.0820)    /* Solar constant G_sc [MJ m2 min] *** * * */
#define RA_COEFFICIENT        (1440.0)    /* (24 * 60) / π * Gsc = 1440 / π * 0.0820 */

/* Ra data initialization */
Status RaCalc_Init(RaData* data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    memset(data, 0, sizeof(*data));

    return STATUS_OK;
}

/* *** Extraterrestrial radiation Ra calculation per FAO56 formula ***  */
Status Calc_Ra(RaData* out, const DayData* day, const LocationData* loc) {
    if ((out == NULL) || (day == NULL) || (loc == NULL)) {
        return STATUS_NULL_POINTER;
    }

    if (!day->initialized || !loc->initialized) {
        return STATUS_INVALID_VALUE;
    }

    const double phi     = loc->latitude_rad;
    const double delta   = day->delta_rad;
    const double omega_s = day->omega_s_rad;   /* During polar night omega_s = 0: both terms = 0, Ra = 0 */
    const double dr      = day->dr;

    /* Ra = (24 * 60 / π) * Gsc * dr * [ωs * sin(φ) * sin(δ) + cos(φ) * cos(δ) * sin(ωs)] *** ** * *** * */
    const double coeff  = (RA_COEFFICIENT / PI) * SOLAR_CONSTANT_GSC * dr;
    const double term_a = omega_s * sin(phi) * sin(delta);         /* Night/day declination contribution */
    const double term_b = cos(phi) * cos(delta) * sin(omega_s);    /* Sunset hour angle contribution * * */

    out->Ra_daily     = coeff * (term_a + term_b);  /* Eq. 21 */
    out->initialized  = true;

    return STATUS_OK;
}
