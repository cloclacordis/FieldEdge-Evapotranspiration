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

/* *** * * * **** * * *** * * * * * **** * *** * * **** * *** * ** * * * * ** * ** * *** *** *
 * Accumulation of daily wind speed readings and conversion to u2 (eq. 47);
 *
 * pattern: structure with accumulation; initialized = false after WindSpeed_Init(),
 * true after first successful WindSpeed_Update(); cf. AirTemperatureData;
 *
 * u2 as a derived quantity is not stored in the structure: computed by a separate
 * Calc_WindSpeedAt2m() call in orchestration - analogous to Calc_SaturationVapourPressure()
 * *** * * * ** * * *** * * * * * **** * *** * * *** * *** * *** * *** * * ** * ** * *** *** */

/* Daily wind speed accumulator  * *** * * * **** * ** ***** * * * * * ****** * ** * * *** * */
typedef struct {
    double    u_z_min_m_s;    /* Minimum daily wind speed [m/s]  * *** * * * ** **** * * * * */
    double    u_z_max_m_s;    /* Maximum daily wind speed [m/s]  * ** * * * ** ***** * * * * */
    double    u_z_mean_m_s;   /* Mean wind speed at height z [m/s] * *** * * * **** * ** *** */
    double    u_sum_m_s;      /* Accumulated sample sum (internal) [m/s]* *** * * * **** * * */
    double    height_m;       /* Measurement height [m]; set in Update() * *** ** * **** * * */
    uint32_t  sample_count;   /* Number of accepted measurements * *** * * *** ***** * * * * */
    bool      initialized;    /* true after first valid Update() * *** * * * ** **** * * * * */
} WindSpeedData;

/**
 * @brief Initializes wind speed data to a safe zero state.
 *
 * @param[out] data Pointer to the WindSpeedData structure to
 *                  initialize. Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status WindSpeed_Init(WindSpeedData *data);

/**
 * @brief Folds one instantaneous reading into the daily min/max/mean.
 *
 * Unlike the temperature/humidity accumulators, this one tracks a
 * true running arithmetic mean (`.u_z_mean_m_s = sum/count`), not a
 * range midpoint. The measurement height is fixed by the first
 * accepted sample; every later sample in the same run must match it
 * within `WIND_HEIGHT_TOL_M`, or the call is rejected - heights
 * cannot be mixed within one accumulation.
 *
 * @param[in,out] data          Accumulator to update. Must not be NULL.
 * @param[in]     speed_m_s     Instantaneous wind speed [m/s]. Must be
 *                              finite and in [0.0, 100.0].
 * @param[in]     height_m      Measurement height [m]. Must be finite
 *                              and in [0.1, 200.0]; must match the
 *                              height fixed by the first accepted
 *                              sample, if any.
 * @param[in]     timestamp     Currently unused (reserved for future
 *                              time-ordered sample handling).
 *
 * @retval STATUS_OK            Accepted; *data reflects the new reading.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE speed_m_s or height_m out of range, or
 *                              height_m differs from the fixed height
 *                              by more than WIND_HEIGHT_TOL_M;
 *                              *data unchanged.
 */
Status WindSpeed_Update(WindSpeedData *data, double speed_m_s, double height_m, uint32_t timestamp);

/**
 * @brief Converts a wind speed measured at height z to the FAO-56
 *        standard 2 m height (eq. 47).
 *
 * u2 = u_z * (4.87 / ln(67.8 * z - 5.42)).
 *
 * @param[in]  u_z    Wind speed at height @p z [m/s]. Must be finite
 *                    and in [0.0, 100.0].
 * @param[in]  z      Measurement height [m]. Must be finite and in
 *                    [0.1, 200.0]; must also keep `67.8 * z - 5.42 > 0`
 *                    (true for any z within that range).
 * @param[out] out_u2 Destination for the result [m/s]. Must not be
 *                    NULL.
 *
 * @retval STATUS_OK            *out_u2 is valid.
 * @retval STATUS_NULL_POINTER  out_u2 was NULL.
 * @retval STATUS_INVALID_VALUE u_z or z out of range, or the result
 *                              was non-finite.
 */
Status Calc_WindSpeedAt2m(double u_z, double z, double *out_u2);

#ifdef __cplusplus
}
#endif

#endif /* WIND_SPEED_CALC_H */
