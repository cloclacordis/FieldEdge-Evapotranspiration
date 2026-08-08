/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <stddef.h>
#include "atm-pressure-model.h"

Status Calc_PressureFromElevation(const double elevation_m, double *P_kPa) {
    if (P_kPa == NULL) {
        return STATUS_NULL_POINTER;
    }

    /* Range: -500 m...+6000 m; formula (293 - 0.0065z) > 0 at z < 45077 m is always ok */
    if ((elevation_m < -500.0) || (elevation_m > 6000.0)) {
        return STATUS_INVALID_VALUE;
    }

    const double factor = (293.0 - (0.0065 * elevation_m)) / 293.0;
    if (factor <= 0.0) {
        return STATUS_INVALID_VALUE;
    }

    const double P = 101.3 * pow(factor, 5.26);     /* eq. 7 */
    if (!isfinite(P) || (P <= 0.0)) {
        return STATUS_INVALID_VALUE;
    }

    *P_kPa = P;

    return STATUS_OK;
}
