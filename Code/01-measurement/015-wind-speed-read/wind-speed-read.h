/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef WIND_SPEED_READ_H
#define WIND_SPEED_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant wind speed reading */
typedef struct {
    double            speed_m_s;    /* Instant wind speed [m/s] */
    double            height_m;     /* Measurement height [m]   */
    uint32_t          timestamp;    /* Timestamp [s]            */
    SensorValueSource source;       /* Data source              */
} WindSpeedSample;

/**
 * @brief Reads an instantaneous wind speed value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_WIND_SPEED_MS) at the WMO standard measurement height 10 m
 * (CONFIG_WIND_HEIGHT_WMO_M, deployment-config.h), with a live
 * timestamp; fails only on a NULL pointer.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pairs with SensorWindSpeed_ReadDefault(). The fallback uses a
 *       different measurement height (2 m, not 10 m) - downstream
 *       WindSpeed_Update() rejects a height change between calls, so
 *       this is safe because ReadInstant and ReadDefault are
 *       never both accepted within the same run.
 */
Status SensorWindSpeed_ReadInstant(WindSpeedSample *out_sample);

/**
 * @brief Returns the fallback wind speed reading.
 *
 * Fixed value (SENSOR_DEFAULT_WIND_SPEED_MS) at the FAO-56 standard
 * height 2 m (CONFIG_WIND_HEIGHT_FAO_M, deployment-config.h),
 * zero timestamp, `.source` set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorWindSpeed_ReadDefault(WindSpeedSample *out_sample);

#ifdef __cplusplus
}
#endif

#endif /* WIND_SPEED_READ_H */
