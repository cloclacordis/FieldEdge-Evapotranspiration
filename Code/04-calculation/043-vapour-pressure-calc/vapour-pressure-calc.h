/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef VAPOUR_PRESSURE_CALC_H
#define VAPOUR_PRESSURE_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../041-air-temperature-calc/air-temperature-calc.h"
#include "../042-air-humidity-calc/air-humidity-calc.h"
#include "../../03-validation/033-status/status.h"

/* Saturation vapour pressure e(T) at arbitrary T (eq. 11) */
Status Calc_SaturationVapourPressure(double T_c, double *e_sat_kPa);

/* Mean saturation vapour pressure (eq. 12), e_s = (e(T_max) + e(T_min)) / 2 */
Status Calc_MeanSaturationVapourPressure(const AirTemperatureData* Tdata, double* out_kPa);

/* Delta: slope of saturation vapour pressure curve, uses T_mean */
Status Calc_SlopeDelta(const AirTemperatureData* Tdata, double* out_kPa_per_C);

/* Actual vapour pressure from accumulated daily RH (eq. 17): ea = [e(Tmin) * RHmax/100 + e(Tmax) * RHmin/100] / 2 */
Status Calc_ActualVapourPressure(double *ea_kPa, const AirTemperatureData *temp, const AirHumidityData *humidity);

#ifdef __cplusplus
}
#endif

#endif /* VAPOUR_PRESSURE_CALC_H */
