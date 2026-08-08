/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <string.h>
#include "net-radiation-calc.h"
#include "../../03-validation/034-math-utils/math-utils.h"

Status NetRadiation_Init(NetRadiationData *data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }
    
    memset(data, 0, sizeof(*data));
    data->initialized = true;
    
    return STATUS_OK;
}

Status Calc_NetRadiation(NetRadiationData *out,
    const AirTemperatureData *temp, const SolarRadiationData *solar, const double ea_kPa) {

    /* NULL checks */
    if ((out == NULL) || (temp == NULL) || (solar == NULL)) {
        return STATUS_NULL_POINTER;
    }

    /* Input structure initialization checks */
    if (!out->initialized || !temp->initialized || !solar->initialized) {
        return STATUS_INVALID_VALUE;
    }

    /* ea validation */
    if ((ea_kPa < 0.0) || !isfinite(ea_kPa)) {
        return STATUS_INVALID_VALUE;
    }

    /* Radiation values validation */
    if ((solar->Rs_daily < 0.0) || (solar->Rso_daily < 0.0)) {
        return STATUS_INVALID_VALUE;
    }

    /* *** * * * ***** * * ***** * * * ****** * ****** * * * *** *** * *** *
     * eq. 38: Rns = (1 - α) * Rs;
     * α = GRASS_ALBEDO = 0.23 - for hypothetical reference crop
     * * * ** **** * * ***** * * * ** ****** * * * * * * ***** * * * *** * */
    const double Rns = (1.0 - GRASS_ALBEDO) * solar->Rs_daily;

    /* * * ** **** * * ***** * * * ** ****** * * * * * * ***** * * * *** * *
     * eq. 39: Rnl = σ * [(Tmax,K⁴ + Tmin,K⁴) / 2] * humidity * cloudiness
     * *** * * * ***** * * ***** * * * ****** * ****** * * * *** *** * *** */

    /* Convert to Kelvin: FAO56 uses 273.16 */
    const double Tmax_K = temp->T_max_C + CELSIUS_TO_KELVIN;
    const double Tmin_K = temp->T_min_C + CELSIUS_TO_KELVIN;

    /* T⁴ via multiplication, avoiding pow() */
    const double Tmax_K2 = Tmax_K * Tmax_K;
    const double Tmin_K2 = Tmin_K * Tmin_K;
    
    const double sigma_T4_avg = STEFAN_BOLTZMANN * (((Tmax_K2 * Tmax_K2) + (Tmin_K2 * Tmin_K2)) / 2.0);

    /* Humidity correction: (0.34 - 0.14 * √ea); at high humidity:
     * ea↑ -> √ea↑ -> factor↓ -> Rnl↓ (moisture absorbs heat) ** */
    const double humidity_factor = 0.34 - (0.14 * sqrt(ea_kPa));

    /* Rs/Rso ratio - measure of cloudiness;
     * upper bound 1.0 (FAO56 requirement, protects against sensor noise);
     * when Rso = 0 (polar night) and Rs = 0 -> use 0, division avoided */
    double Rs_over_Rso;

    if (solar->Rso_daily <= 0.0) {
        Rs_over_Rso = 0.0;
    } else {
        Rs_over_Rso = Min(solar->Rs_daily / solar->Rso_daily, 1.0);
    }

    /* Cloudiness correction: 1.35 * Rs/Rso - 0.35;
     * under clear sky (Rs/Rso -> 1): factor ->   1.0 -> Rnl maximum;
     * under overcast  (Rs/Rso -> 0): factor -> -0.35 -> bounded to 0;
     * a negative factor would mean Rnl < 0, unrealistic for daily time step */
    const double cloudiness_factor = Max((1.35 * Rs_over_Rso) - 0.35, 0.0);

    const double Rnl = sigma_T4_avg * humidity_factor * cloudiness_factor;

    /* *** * * * ***** * * ***** * * * ****** * ****** * * * *** *** * *** *
     * eq. 40: Rn = Rns - Rnl
     * * * ** **** * * ***** * * * ** ****** * * * * * * ***** * * * *** * */
    const double Rn = Rns - Rnl;

    /* NaN/inf protection */
    if (!isfinite(Rns) || !isfinite(Rnl) || !isfinite(Rn)) {
        return STATUS_INVALID_VALUE;
    }

    out->Rns_daily = Rns;
    out->Rnl_daily = Rnl;
    out->Rn_daily  = Rn;

    return STATUS_OK;
}
