/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <stddef.h>
#include "solar-radiation-calc.h"

Status AngstromValues_Default(AngstromValues* ang) {
    if (ang == NULL) {
        return STATUS_NULL_POINTER;
    }

    ang->a_s = DEFAULT_ANGSTROM_VALUE_A_S;
    ang->b_s = DEFAULT_ANGSTROM_VALUE_B_S;

    return STATUS_OK;
}

Status SolarRadiation_Init(SolarRadiationData* data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    data->Rs_daily    = 0.0;
    data->Rso_daily   = 0.0;
    data->initialized = true;

    return STATUS_OK;
}

Status SolarRadiation_Calc(const AngstromValues* ang, SolarRadiationData* out, const RaData* ra, const DayData* day,
    const SunshineLuxData* sunshine, const LocationData* loc) {

    if ((ang == NULL) || (out == NULL) || (ra == NULL) || (day == NULL) || (sunshine == NULL) || (loc == NULL)) {
        return STATUS_NULL_POINTER;
    }

    if (!out->initialized || !ra->initialized || !day->initialized || !sunshine->initialized || !loc->initialized) {
        return STATUS_INVALID_VALUE;
    }

    /* Check Angström-Prescott coefficients; expected: as >= 0, bs >= 0, as + bs <= 1 */
    if ((ang->a_s < 0.0) || (ang->b_s < 0.0) || ((ang->a_s + ang->b_s) > 1.0)) {
        return STATUS_INVALID_VALUE;
    }

    if (ra->Ra_daily < 0.0) {
        return STATUS_INVALID_VALUE;
    }

    if (day->N_hours < 0.0) {
        return STATUS_INVALID_VALUE;
    }

    if (sunshine->n_hours < 0.0) {
        return STATUS_INVALID_VALUE;
    }

    /* Polar night: Ra = 0, N = 0, Rs = 0 */
    if (day->N_hours  == 0.0) {
        out->Rs_daily  = 0.0;
        out->Rso_daily = 0.0;

        return STATUS_OK;
    }

    /* Actual sunshine duration n cannot exceed maximum possible N */
    const double n = (sunshine->n_hours <= day->N_hours) ? sunshine->n_hours : day->N_hours;

    /* Relative sunshine duration n/N */
    const double n_over_N = n / day->N_hours;

    /* FAO56 eq. 35: Rs = (as + bs * n/N) * Ra */
    const double Rs  = (ang->a_s + (ang->b_s * n_over_N)) * ra->Ra_daily;

    /* FAO56 eq. 37: Rso = (0.75 + 2e-5 * z) * Ra */
    const double Rso = (CLEAR_SKY_BASE_COEFFICIENT + (2e-5 * loc->elevation_m)) * ra->Ra_daily;

    /* NaN/inf protection */
    if (!isfinite(Rs) || !isfinite(Rso)) {
        return STATUS_INVALID_VALUE;
    }

    /* Additional check */
    if ((Rs < 0.0) || (Rso < 0.0)) {
        return STATUS_INVALID_VALUE;
    }

    /* Store computed values */
    out->Rs_daily  = Rs;
    out->Rso_daily = Rso;

    return STATUS_OK;
}
