/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <string.h>
#include "psychrometric-calc.h"

Status AtmosphericData_Init(AtmosphericData *data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    memset(data, 0, sizeof(*data));
    data->initialized = true;

    return STATUS_OK;
}

Status Calc_AtmosphericParameters(AtmosphericData *out, const double P_kPa) {
    if (out == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!out->initialized) {
        return STATUS_INVALID_VALUE;
    }

    /* Atm pressure range check: 50-120 kPa (with a large margin) */
    if ((P_kPa < 50.0) || (P_kPa > 120.0) || !isfinite(P_kPa)) {
        return STATUS_INVALID_VALUE;
    }

    /* Eq. 8: γ = c_p * P / (ε * λ) ≈ 0.000665 * P */
    const double gamma = PSYCHROMETRIC_GAMMA_COEFF * P_kPa;

    if (!isfinite(gamma)) {
        return STATUS_INVALID_VALUE;
    }

    out->P_kPa           = P_kPa;
    out->gamma_kPa_per_C = gamma;

    return STATUS_OK;
}
