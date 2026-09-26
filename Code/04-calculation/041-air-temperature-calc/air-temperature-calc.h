/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_TEMPERATURE_CALC_H
#define AIR_TEMPERATURE_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Structure for storing computed temperature values [C] */
typedef struct {
    double   T_max_C;
    double   T_min_C;
    double   T_mean_C;
    uint32_t timestamp;
    bool     initialized;
} AirTemperatureData;

/**
 * @brief Initializes air temperature data to a safe zero state.
 *
 * @param[out] data Pointer to the AirTemperatureData structure to
 *                  initialize. Must not be NULL.
 *
 * @post On success, *data is zero-initialized; `.initialized` is false.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status AirTemperature_Init(AirTemperatureData* data);

/**
 * @brief Folds one instantaneous reading into the daily min/max/mean.
 *
 * Validates @p T_inst_C before accepting it. The first accepted
 * reading initializes both min and max to that value; each
 * subsequent one only widens the range. `.T_mean_C` is recomputed as
 * `(T_max_C + T_min_C) / 2` on every accepted call (a range midpoint,
 * not a running arithmetic mean of all samples).
 *
 * @param[in,out] data      Accumulator to update. Must not be NULL.
 * @param[in]     T_inst_C  Instantaneous air temperature [C]. Must
 *                          satisfy ValidTemperatureC().
 * @param[in]     timestamp Acquisition time of T_inst_C.
 *
 * @retval STATUS_OK            Accepted; *data reflects the new reading.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE T_inst_C failed ValidTemperatureC();
 *                              *data unchanged.
 */
Status AirTemperature_Update(AirTemperatureData* data, double T_inst_C, uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* AIR_TEMPERATURE_CALC_H */
