/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

// #include <time.h>
#include <stddef.h>
#include "air-temperature-read.h"
#include "../../02-providers/021-date-provider/time-provider.h"

#define SENSOR_MOCK_INSTANT_C             (20.0)  /* Emulate air temperature measurement */
#define SENSOR_DEFAULT_INSTANT_C          (20.0)  /* Use default value *** * * **** * ** */
#define SENSOR_AIR_TEMP_DEFAULT_TIMESTAMP (0U)

Status SensorTemperature_ReadInstant(TemperatureSample* out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->instant_c = SENSOR_MOCK_INSTANT_C;

    // out_sample->timestamp = (uint32_t)time(NULL);

    /* Smoke-test timestamp */
    Status status = TimeProvider_Read(&out_sample->timestamp);
    if (status != STATUS_OK) {
        return status;
    }

    out_sample->source    = SENSOR_VALUE_MEASURED;

    return STATUS_OK;
}

Status SensorTemperature_ReadDefault(TemperatureSample* out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->instant_c = SENSOR_DEFAULT_INSTANT_C;
    out_sample->timestamp = SENSOR_AIR_TEMP_DEFAULT_TIMESTAMP;
    out_sample->source    = SENSOR_VALUE_DEFAULT;

    return STATUS_OK;
}
