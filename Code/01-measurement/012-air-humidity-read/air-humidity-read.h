/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_HUMIDITY_READ_H
#define AIR_HUMIDITY_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant relative air humidity RH reading */
typedef struct {
    double            RH_pct;     /* Relative air humidity [%] */
    uint32_t          timestamp;  /* Timestamp [s]             */
    SensorValueSource source;     /* Data source               */
} AirHumiditySample;

/**
 * @brief Reads an instantaneous relative humidity value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_INSTANT_RH_PCT) with a live timestamp; fails only on
 * a NULL pointer.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pairs with SensorHumidity_ReadDefault().
 */
Status SensorHumidity_ReadInstant(AirHumiditySample *out_sample);

/**
 * @brief Returns the fallback relative humidity reading.
 *
 * Fixed value (SENSOR_DEFAULT_INSTANT_RH_PCT), zero timestamp,
 * `.source` set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorHumidity_ReadDefault(AirHumiditySample *out_sample);

#ifdef __cplusplus
}
#endif

#endif /* AIR_HUMIDITY_READ_H */
