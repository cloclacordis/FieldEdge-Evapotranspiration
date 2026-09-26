/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef ETO_CALC_H
#define ETO_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../03-validation/033-status/status.h"

#define C_RAD  (0.408)    /* Radiation conversion coefficient to ET mm *** ** * **** * ** */
/* Daily soil heat flux G ≈ 0 (eq. 42): G_mj_m2_day argument in Calc_ETo() for daily calc */
#define ETO_G_DAILY_MJ_M2_DAY  (0.0)    /* MJ m-2 day-1 **** * * ***** * * * * ***** * ** */

/**
 * @brief Computes reference evapotranspiration by the FAO-56
 *        Penman-Monteith method (eq. 6).
 *
 * ETo = [0.408*delta*(Rn-G) + gamma*(900/(T+273))*u2*(es-ea)]
 *       / [delta + gamma*(1+0.34*u2)].
 *
 * All eight inputs are leaf values, already computed by earlier
 * RunDailyCycle() steps - this function does not read any sensor or
 * acquisition data itself; see data-flow-specification.md ("Final
 * outputs"). If @p ea_kpa exceeds @p es_kpa (RH would be over
 * 100%, a measurement artifact), ea is capped to es before use. The
 * result is clamped to >= 0 (a negative Rn, e.g. in winter, can
 * otherwise drive the numerator negative).
 *
 * @param[in]  delta_kpa_c    Slope of the saturation vapour pressure
 *                            curve [kPa/C] (eq. 13). Must be finite
 *                            and > 0.
 * @param[in]  Rn_mj_m2_day   Net radiation [MJ/m2/day] (eq. 40). Must
 *                            be finite; may be negative.
 * @param[in]  G_mj_m2_day    Daily soil heat flux [MJ/m2/day]; pass
 *                            ETO_G_DAILY_MJ_M2_DAY (0, per eq. 42) for
 *                            a daily time step. Must be finite.
 * @param[in]  gamma_kpa_c    Psychrometric constant [kPa/C] (eq. 8).
 *                            Must be finite and > 0.
 * @param[in]  T_mean_c       Mean daily air temperature [C]. Must be
 *                            finite.
 * @param[in]  u2_m_s         Wind speed at 2 m height [m/s] (eq. 47).
 *                            Must be finite and >= 0.
 * @param[in]  es_kpa         Saturation vapour pressure [kPa] (eq. 12).
 *                            Must be finite and > 0.
 * @param[in]  ea_kpa         Actual vapour pressure [kPa] (eq. 17).
 *                            Must be finite and >= 0.
 * @param[out] out_eto_mm_day Destination for the result [mm/day].
 *                            Always >= 0. Must not be NULL.
 *
 * @retval STATUS_OK            *out_eto_mm_day is valid.
 * @retval STATUS_NULL_POINTER  out_eto_mm_day was NULL.
 * @retval STATUS_INVALID_VALUE Any input was non-finite, or delta,
 *                              gamma, u2, or es was outside the
 *                              ranges listed above.
 */
Status Calc_ETo(
    double  delta_kpa_c,      /* Slope of saturation vapour pressure curve [kPa/C]; > 0 * */
    double  Rn_mj_m2_day,     /* Net radiation [MJ m-2 day-1]; may be < 0 * * *** * * * * */
    double  G_mj_m2_day,      /* Daily soil heat flux; ETO_G_DAILY_MJ_M2_DAY * * **** * * */
    double  gamma_kpa_c,      /* Psychrometric constant [kPa/C];   > 0 * * * ** * * *** * */
    double  T_mean_c,         /* Mean daily temperature [C] * * * * **** * **** * * * *** */
    double  u2_m_s,           /* Wind speed at 2 m height [m/s];   ≥ 0 * * * * **** * *** */
    double  es_kpa,           /* Saturation vapour pressure [kPa]; > 0 * * * * **** * * * */
    double  ea_kpa,           /* Actual vapour pressure [kPa];     ≥ 0 * * **** * ** * ** */
    double  *out_eto_mm_day   /* Result: ETo [mm/day]; always      ≥ 0 *** ** ** * ** * * */
);

/**
 * @brief Computes crop evapotranspiration (FAO-56, eq. 56).
 *
 * ETc = Kc * ETo.
 *
 * @param[in]  eto_mm_day     Reference evapotranspiration, from
 *                            Calc_ETo() [mm/day]. Must be finite
 *                            and >= 0.
 * @param[in]  kc             Crop coefficient (Type A, from
 *                            deployment-config.h). Must be finite
 *                            and > 0.
 * @param[out] out_etc_mm_day Destination for the result [mm/day].
 *                            Must not be NULL.
 *
 * @retval STATUS_OK            *out_etc_mm_day is valid.
 * @retval STATUS_NULL_POINTER  out_etc_mm_day was NULL.
 * @retval STATUS_INVALID_VALUE eto_mm_day or kc was non-finite or
 *                              out of range.
 */
Status Calc_ETc(
    double eto_mm_day,       /* ETo from Calc_ETo() [mm/day]; ≥ 0 ** * ** * * ***** * *** */
    double kc,               /* Crop coefficient, type A from deployment-config.h; > 0 ** */
    double *out_etc_mm_day   /* Result: ETc [mm/day] ** * ** * * ***** * ** * * ***** * * */
);

#ifdef __cplusplus
}
#endif

#endif /* ETO_CALC_H */
