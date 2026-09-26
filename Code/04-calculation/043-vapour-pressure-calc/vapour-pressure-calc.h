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

/**
 * @brief Computes the saturation vapour pressure e(T) at an arbitrary
 *        temperature using the Magnus-Tetens formula (FAO-56, eq. 11).
 *
 * e(T) = 0.6108 * exp((17.27 * T) / (T + 237.3)).
 *
 * @param[in]  T_c       Air temperature [C]. Must be finite;
 *                       physically valid outside a narrow range,
 *                       no additional bound is enforced here.
 * @param[out] e_sat_kPa Destination for the result [kPa]. Must not
 *                       be NULL.
 *
 * @retval STATUS_OK            *e_sat_kPa is valid.
 * @retval STATUS_NULL_POINTER  e_sat_kPa was NULL.
 * @retval STATUS_INVALID_VALUE T_c was NaN or infinite.
 */
Status Calc_SaturationVapourPressure(double T_c, double *e_sat_kPa);

/**
 * @brief Computes mean saturation vapour pressure (FAO-56, eq. 12).
 *
 * es = (e(Tmax) + e(Tmin)) / 2, each e(T) per eq. 11.
 *
 * @param[in]  Tdata   Accumulated daily temperature data. Must not
 *                     be NULL; `.initialized` must be true and
 *                     `.T_max_C`/`.T_min_C` must satisfy
 *                     ValidTemperatureC().
 * @param[out] out_kPa Destination for the result [kPa]. Must not be
 *                     NULL.
 *
 * @retval STATUS_OK            *out_kPa is valid.
 * @retval STATUS_NULL_POINTER  Tdata or out_kPa was NULL.
 * @retval STATUS_INVALID_VALUE Tdata not initialized, or its stored
 *                              T_max_C/T_min_C is out of range.
 */
Status Calc_MeanSaturationVapourPressure(const AirTemperatureData* Tdata, double* out_kPa);

/**
 * @brief Computes the slope of the saturation vapour pressure curve
 *        using mean air temperature (FAO-56, eq. 13).
 *
 * delta = (4098 * e(Tmean)) / (Tmean + 237.3)^2, e(T) per eq. 11.
 *
 * @param[in]  Tdata         Accumulated daily temperature data. Must
 *                           not be NULL; `.initialized` must be
 *                           true and `.T_mean_C` must satisfy
 *                           ValidTemperatureC().
 * @param[out] out_kPa_per_C Destination for the result [kPa/C]. Must
 *                           not be NULL.
 *
 * @retval STATUS_OK            *out_kPa_per_C is valid.
 * @retval STATUS_NULL_POINTER  Tdata or out_kPa_per_C was NULL.
 * @retval STATUS_INVALID_VALUE Tdata not initialized, its T_mean_C is
 *                              out of range, or the denominator is
 *                              numerically degenerate (T_mean_C at or
 *                              near -237.3 C).
 */
Status Calc_SlopeDelta(const AirTemperatureData* Tdata, double* out_kPa_per_C);

/**
 * @brief Computes actual vapour pressure from accumulated relative
 *        humidity and temperature (FAO-56, eq. 17).
 *
 * ea = [e(Tmin)*RHmax/100 + e(Tmax)*RHmin/100] / 2, e(T) per eq. 11.
 *
 * @param[out] ea_kPa   Destination for the result [kPa]. Must not
 *                      be NULL.
 * @param[in]  temp     Accumulated air temperature data. Must not
 *                      be NULL; `.initialized` must be true.
 * @param[in]  humidity Accumulated relative humidity data. Must not
 *                      be NULL; `.initialized` must be true.
 *
 * @retval STATUS_OK            *ea_kPa is valid.
 * @retval STATUS_NULL_POINTER  ea_kPa, temp, or humidity was NULL.
 * @retval STATUS_INVALID_VALUE temp or humidity not initialized; or
 *                              the result was non-finite or negative;
 *                              also propagated verbatim from an
 *                              internal Calc_SaturationVapourPressure()
 *                              call, in principle unreachable given
 *                              already-validated temp data.
 */
Status Calc_ActualVapourPressure(double *ea_kPa, const AirTemperatureData *temp, const AirHumidityData *humidity);

#ifdef __cplusplus
}
#endif

#endif /* VAPOUR_PRESSURE_CALC_H */
