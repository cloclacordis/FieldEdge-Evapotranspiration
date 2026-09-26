/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef ATM_PRESSURE_READ_H
#define ATM_PRESSURE_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant atmospheric pressure reading ** * */
typedef struct {
    double            P_kPa;      /* Atmospheric pressure [kPa] */
    uint32_t          timestamp;  /* Timestamp                  */
    SensorValueSource source;     /* Data source                */
} AtmPressureSample;

/**
 * @brief Reads an instantaneous atmospheric pressure value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_P_KPA) with a live timestamp; fails only on a NULL
 * pointer.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pressure has a three-source priority in RunDailyCycle():
 *       sensor (this function), then Calc_PressureFromElevation()
 *       (FAO-56, eq. 7), then SensorPressure_ReadDefault() as
 *       the last resort. See also atm-pressure-model.c.
 */
Status SensorPressure_ReadInstant(AtmPressureSample *out_sample);

/**
 * @brief Returns the final-fallback atmospheric pressure reading.
 *
 * Fixed standard sea-level value (SENSOR_DEFAULT_P_KPA), zero
 * timestamp, `.source` set to SENSOR_VALUE_DEFAULT. Used only if both
 * SensorPressure_ReadInstant() and Calc_PressureFromElevation() fail.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorPressure_ReadDefault(AtmPressureSample *out_sample);

#ifdef __cplusplus
}
#endif

#endif /* ATM_PRESSURE_READ_H */
