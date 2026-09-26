/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_TEMPERATURE_READ_H
#define AIR_TEMPERATURE_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant air temperature reading [C] */
typedef struct {
    double            instant_c;
    uint32_t          timestamp;
    SensorValueSource source;
} TemperatureSample;

/**
 * @brief Reads an instantaneous air temperature value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_INSTANT_C) with a live timestamp; fails only on a
 * NULL pointer. A real sensor driver will fail for other reasons too.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pairs with SensorTemperature_ReadDefault(), the fallback used
 *       by RunDailyCycle() when this call does not return STATUS_OK.
 */
Status SensorTemperature_ReadInstant(TemperatureSample* out_sample);

/**
 * @brief Returns the fallback air temperature reading.
 *
 * Fixed value (SENSOR_DEFAULT_INSTANT_C), zero timestamp, `.source`
 * set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorTemperature_ReadDefault(TemperatureSample* out_sample);

#ifdef __cplusplus
}
#endif

#endif /* AIR_TEMPERATURE_READ_H */
