/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef ETO_CALC_H
#define ETO_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../03-validation/033-status/status.h"

/* Daily soil heat flux G ≈ 0 (eq. 42): G_mj_m2_day argument in Calc_ETo() for daily calc  */
#define ETO_G_DAILY_MJ_M2_DAY  (0.0)    /* MJ m⁻² day⁻¹ **** * * ***** * * * * ******* * ** */

/* Reference evapotranspiration, or Penman-Monteith equation (eq. 6):
 * if ea  > es (RH > 100%): ea is capped to es (VPD = 0);
 * if ETo < 0  (Rn < 0): result = 0 (ETo ≥ 0) * * **** * ** * **** * * * * * **** * * **** */
Status Calc_ETo(
    double  delta_kpa_c,       /* Slope of saturation vapour pressure curve [kPa/C]; > 0 * */
    double  Rn_mj_m2_day,      /* Net radiation [MJ m⁻² day⁻¹]; may be < 0 * * **** * * * * */
    double  G_mj_m2_day,       /* Daily soil heat flux; ETO_G_DAILY_MJ_M2_DAY * * **** * * */
    double  gamma_kpa_c,       /* Psychrometric constant [kPa/C];   > 0 * * * ** * * *** * */
    double  T_mean_c,          /* Mean daily temperature [C] * * * * **** * **** * * * *** */
    double  u2_m_s,            /* Wind speed at 2 m height [m/s];   ≥ 0 * * * * **** * *** */
    double  es_kpa,            /* Saturation vapour pressure [kPa]; > 0 * * * * **** * * * */
    double  ea_kpa,            /* Actual vapour pressure [kPa];     ≥ 0 * * **** * ** * ** */
    double  *out_eto_mm_day    /* Result: ETo [mm/day]; always      ≥ 0 *** ** ** * ** * * */
);

/* Crop evapotranspiration (eq. 56): ETc = Kc * ETo **** * * **** * ** * * **** * * * **** */
Status Calc_ETc(
    double eto_mm_day,        /* ETo from Calc_ETo() [mm/day]; ≥ 0 ** * ** * * ***** * *** */
    double kc,                /* Crop coefficient, type A from deployment-config.h; > 0 ** */
    double *out_etc_mm_day    /* Result: ETc [mm/day] ** * ** * * ***** * ** * * ***** * * */
);

#ifdef __cplusplus
}
#endif

#endif /* ETO_CALC_H */
