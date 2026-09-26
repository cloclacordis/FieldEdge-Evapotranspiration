/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_HUMIDITY_CALC_H
#define AIR_HUMIDITY_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Accumulated daily relative humidity values [%] */
typedef struct {
    double   RH_max;
    double   RH_min;
    double   RH_mean;
    uint32_t timestamp;
    bool     initialized;
} AirHumidityData;

/**
 * @brief Initializes air humidity data to a safe zero state.
 *
 * @param[out] data Pointer to the AirHumidityData structure to
 *                  initialize. Must not be NULL.
 *
 * @post On success, *data is zero-initialized; `.initialized` is false.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status AirHumidity_Init(AirHumidityData *data);

/**
 * @brief Folds one instantaneous reading into the daily min/max/mean.
 *
 * Same accumulation pattern as AirTemperature_Update(): first accepted
 * reading initializes min and max; `.RH_mean` is the range midpoint,
 * recomputed on every accepted call.
 *
 * @param[in,out] data      Accumulator to update. Must not be NULL.
 * @param[in]     RH_pct    Instantaneous relative humidity [%]. Must
 *                          satisfy ValidHumidityPercent().
 * @param[in]     timestamp Acquisition time of RH_pct.
 *
 * @retval STATUS_OK            Accepted; *data reflects the new reading.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE RH_pct failed ValidHumidityPercent();
 *                              *data unchanged.
 */
Status AirHumidity_Update(AirHumidityData *data, double RH_pct, uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* AIR_HUMIDITY_CALC_H */
