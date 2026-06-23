/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <stddef.h>
#include "vapour-pressure-calc.h"
#include "../../03-validation/032-validation/validation.h"

/* Constants for e(T) and slope of SVP curve calculation *** *** * * * *** * * * *** */
#define TETENS_CONST_A    (0.6108)    /* Tetens equation constant for e(T) per FAO56 */
#define TETENS_CONST_B    (17.27)     /* Tetens equation constant for e(T) per FAO56 */
#define TETENS_CONST_C    (237.3)     /* Tetens equation constant for e(T) per FAO56 */
#define SVP_CS_CONST_D    (4098.0)    /* Constant for slope of SVP curve equation ** */

/* Internal (non-public) helper function: Magnus-Tetens equation (eq. 11) */
static double Calc_TetensSaturationPressure(const double temperature_c) {
    const double exp_term = (TETENS_CONST_B * temperature_c) / (temperature_c + TETENS_CONST_C);
    return TETENS_CONST_A * exp(exp_term);
}

/* Saturation vapour pressure e(T) at arbitrary T (eq. 11) */
Status Calc_SaturationVapourPressure(const double T_c, double *e_sat_kPa) {
    if (e_sat_kPa == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!isfinite(T_c)) {
        return STATUS_INVALID_VALUE;
    }

    *e_sat_kPa = Calc_TetensSaturationPressure(T_c);

    return STATUS_OK;
}

/* Mean saturation vapour pressure (eq. 12), e_s = (e(T_max) + e(T_min)) / 2 */
Status Calc_MeanSaturationVapourPressure(const AirTemperatureData* Tdata, double* out_kPa) {
    if ((Tdata == NULL) || (out_kPa == NULL)) {
        return STATUS_NULL_POINTER;
    }

    if ((Tdata->initialized == false) || (!ValidTemperatureC(Tdata->T_max_C)) || (!ValidTemperatureC(Tdata->T_min_C))) {
        return STATUS_INVALID_VALUE;
    }

    const double e_Tmax = Calc_TetensSaturationPressure(Tdata->T_max_C);
    const double e_Tmin = Calc_TetensSaturationPressure(Tdata->T_min_C);
    *out_kPa = (e_Tmax + e_Tmin) / 2.0;

    return STATUS_OK;
}

/* Delta = slope of saturation vapour pressure curve, uses T_mean (eq. 13) */
Status Calc_SlopeDelta(const AirTemperatureData* Tdata, double* out_kPa_per_C) {
    if ((Tdata == NULL) || (out_kPa_per_C == NULL)) {
        return STATUS_NULL_POINTER;
    }

    if ((Tdata->initialized == false) || (!ValidTemperatureC(Tdata->T_mean_C))) {
        return STATUS_INVALID_VALUE;
    }

    const double e_Tmean = Calc_TetensSaturationPressure(Tdata->T_mean_C);
    const double denom = (Tdata->T_mean_C + TETENS_CONST_C) * (Tdata->T_mean_C + TETENS_CONST_C);

    if (denom == 0.0) {
        return STATUS_INVALID_VALUE;
    }

    *out_kPa_per_C = (SVP_CS_CONST_D * e_Tmean) / denom;

    return STATUS_OK;
}

/* Actual vapour pressure from accumulated daily RH (eq. 17) */
Status Calc_ActualVapourPressure(double *ea_kPa, const AirTemperatureData *temp, const AirHumidityData *humidity) {
    if ((ea_kPa == NULL) || (temp == NULL) || (humidity == NULL)) {
        return STATUS_NULL_POINTER;
    }

    if (!temp->initialized || !humidity->initialized) {
        return STATUS_INVALID_VALUE;
    }

    double e_min = 0.0;
    double e_max = 0.0;

    Status s = Calc_SaturationVapourPressure(temp->T_min_C, &e_min);
    if (s != STATUS_OK) {
		return s;
	}

    s = Calc_SaturationVapourPressure(temp->T_max_C, &e_max);
    if (s != STATUS_OK) {
		return s;
	}

    /* ea = [e(Tmin) * RHmax/100 + e(Tmax) * RHmin/100] / 2 */
    const double ea = (e_min * (humidity->RH_max / 100.0) + e_max * (humidity->RH_min / 100.0)) / 2.0;

    if (!isfinite(ea) || (ea < 0.0)) {
        return STATUS_INVALID_VALUE;
    }

    *ea_kPa = ea;

    return STATUS_OK;
}
