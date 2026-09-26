/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef SUNSHINE_LUX_READ_H
#define SUNSHINE_LUX_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant illuminance value */
typedef struct {
    double            lux;          /* Illuminance [lux] */
    uint32_t          timestamp;    /* Timestamp         */
    SensorValueSource source;       /* Data source       */
} SunshineLuxSample;

/**
 * @brief Reads an instantaneous illuminance value from the sensor.
 *
 * PC-mock implementation: always returns a fixed representative
 * illuminance value (SENSOR_MOCK_INSTANT_LUX = 55000 lux as "clear sky")
 * with a live timestamp; fails only on a NULL pointer.
 *
 * @warning lux is a photometric quantity (human-eye-weighted), not
 *          the radiometric direct-beam irradiance (W/m^2). No single
 *          lux-to-W/m^2 conversion factor exists (~21-131 lm/W in the
 *          literature). Furthermore, this global-illuminance reading
 *          includes diffuse skylight, which the WMO reference excludes.
 *          See issues-v01x.md (item 4) and illuminance-proxy.md.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Called once per loop iteration by RunDailyCycle()
 *       (DAILY_CYCLE_MOCK_LUX_SAMPLE_COUNT times), not once per run.
 *       See verified-call-graph.md.
 *       Illuminance is used as a proxy; the "clear sky" threshold
 *       is provisional and requires further validation.
 */
Status SensorLux_ReadInstant(SunshineLuxSample* out_sample);

/**
 * @brief Returns the fallback illuminance reading.
 *
 * Fixed value (SENSOR_DEFAULT_INSTANT_LUX = 0, i.e. "no data"),
 * zero timestamp, `.source` set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorLux_ReadDefault(SunshineLuxSample* out_sample);

#ifdef __cplusplus
}
#endif

#endif /* SUNSHINE_LUX_READ_H */
