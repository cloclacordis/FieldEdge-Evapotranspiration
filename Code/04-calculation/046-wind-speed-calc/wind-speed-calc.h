/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef WIND_SPEED_CALC_H
#define WIND_SPEED_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* *** * * * **** * ** ***** * * * * * ****** * *** * * **** * *** * *** * *** * * ** * ** * *** *** *
 * Accumulation of daily wind speed readings and conversion to u2 (eq. 47);
 *
 * pattern: structure with accumulation; initialized = false after WindSpeed_Init(),
 * true after first successful WindSpeed_Update(); cf. AirTemperatureData;
 *
 * u2 as a derived quantity is not stored in the structure: computed by a separate
 * Calc_WindSpeedAt2m() call in orchestration - analogous to Calc_SaturationVapourPressure()
 * *** * * * **** * ** ***** * * * * * ****** * *** * * **** * *** * *** * *** * * ** * ** * *** *** */

/* Daily wind speed accumulator  * *** * * * **** * ** ***** * * * * * ****** * *** * * **** * *** * */
typedef struct {
    double    u_z_min_m_s;    /* Minimum daily wind speed [m/s]  * *** * * * **** * ** ***** * * * * */
    double    u_z_max_m_s;    /* Maximum daily wind speed [m/s]  * *** * * * **** * ** ***** * * * * */
    double    u_z_mean_m_s;   /* Mean wind speed at height z [m/s] * *** * * * **** * ** ***** * * * */
    double    u_sum_m_s;      /* Accumulated sample sum (internal) [m/s]* *** * * * **** * ** **** * */
    double    height_m;       /* Measurement height [m]; set in Update() * *** * * * **** * ** ***** */
    uint32_t  sample_count;   /* Number of accepted measurements * *** * * * **** * ** ***** * * * * */
    bool      initialized;    /* true after first valid Update() * *** * * * **** * ** ***** * * * * */
} WindSpeedData;

/* Initialize accumulator; sets initialized = false; data not ready until first Update() *** * *** * */
Status WindSpeed_Init(WindSpeedData *data);

/* Add 1 instant measurement; speed_m_s - instant wind speed [m/s], range [0.0, 100.0] ** * ** *** * */
Status WindSpeed_Update(WindSpeedData *data, double speed_m_s, double height_m, uint32_t timestamp);

/* Convert wind speed to 2 m height (eq. 47): u2 = uz * 4.87 / ln(67.8 * z - 5.42), in m/s *** * * */
Status Calc_WindSpeedAt2m(double u_z, double z, double *out_u2);

#ifdef __cplusplus
}
#endif

#endif /* WIND_SPEED_CALC_H */
