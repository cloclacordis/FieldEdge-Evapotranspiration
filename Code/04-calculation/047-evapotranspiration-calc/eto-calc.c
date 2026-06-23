/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stddef.h>
#include "eto-calc.h"

/* Constants for FAO56 eq. 6 (1998: 24) **** * * * * * * * ********* * *** */
#define C_RAD   (0.408)    /* Radiation conversion coefficient to ET mm ** */
#define C_AERO  (900.0)    /* Daily aerodynamic constant * * * * ***** *** */
#define C_TK    (273.0)    /* C to K conversion (FAO56 uses 273) ** * ** * */
#define C_WIND  (0.34)     /* Aerodynamic resistance coefficient for grass */

Status Calc_ETo(const double  delta_kpa_c, const double  Rn_mj_m2_day, const double  G_mj_m2_day, const double  gamma_kpa_c,
    const double  T_mean_c, const double  u2_m_s, const double  es_kpa, const double  ea_kpa, double *out_eto_mm_day) {
    if (out_eto_mm_day == NULL) {
        return STATUS_NULL_POINTER;
    }

    /* Physical constraints: Δ and γ must be positive, u2 non-negative, es positive, ea non-negative */
    if (delta_kpa_c <= 0.0 || gamma_kpa_c <= 0.0 || u2_m_s < 0.0 || es_kpa <= 0.0 || ea_kpa < 0.0) {
        return STATUS_INVALID_VALUE;
    }

    /* Clamp ea: RH cannot exceed 100% (otherwise it's a measurement error) */
    double ea_eff = (ea_kpa > es_kpa) ? es_kpa : ea_kpa;

    /* Eq. 6 numerator: radiation term + aerodynamic term */
    double num_rad  = C_RAD * delta_kpa_c * (Rn_mj_m2_day - G_mj_m2_day);
    double num_aero = gamma_kpa_c * (C_AERO / (T_mean_c + C_TK)) * u2_m_s * (es_kpa - ea_eff);

    /* Eq. 6 denominator; with delta > 0, gamma > 0, u2 >= 0 denominator is always > 0 */
    double den = delta_kpa_c + gamma_kpa_c * (1.0 + C_WIND * u2_m_s);

    /* Eq. 6 result; clamp to 0 when Rn < 0 (winter, polar night) */
    double eto = (num_rad + num_aero) / den;
    *out_eto_mm_day = (eto > 0.0) ? eto : 0.0;

    return STATUS_OK;
}

Status Calc_ETc(const double eto_mm_day, const double kc, double *out_etc_mm_day) {
    if (out_etc_mm_day == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (eto_mm_day < 0.0 || kc <= 0.0) {
        return STATUS_INVALID_VALUE;
    }

    *out_etc_mm_day = kc * eto_mm_day;  /* Eq. 56 */

    return STATUS_OK;
}
