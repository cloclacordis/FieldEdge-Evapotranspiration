/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <time.h>
#include <stddef.h>
#include "sunshine-lux-read.h"

/* Mock values for "clear sky" conditions */
#define SENSOR_MOCK_INSTANT_LUX      (55000.0)     /* Direct sunlight   */
#define SENSOR_DEFAULT_INSTANT_LUX   (0.0)         /* No data available */
#define SENSOR_LUX_DEFAULT_TIMESTAMP (0U)

Status SensorLux_ReadInstant(SunshineLuxSample* out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->lux       = SENSOR_MOCK_INSTANT_LUX;
    out_sample->timestamp = (uint32_t)time(NULL);
    out_sample->source    = SENSOR_VALUE_MEASURED;

    return STATUS_OK;
}

Status SensorLux_ReadDefault(SunshineLuxSample* out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->lux       = SENSOR_DEFAULT_INSTANT_LUX;
    out_sample->timestamp = SENSOR_LUX_DEFAULT_TIMESTAMP;
    out_sample->source    = SENSOR_VALUE_DEFAULT;

    return STATUS_OK;
}
