/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdint.h>
#include <stddef.h>
#include "air-temperature-read.h"
#include "../../02-providers/023-time/time-provider.h"

#define SENSOR_MOCK_INSTANT_C             (20.0)  /* Emulate air temperature measurement */
#define SENSOR_DEFAULT_INSTANT_C          (20.0)  /* Use default value *** * * **** * ** */
#define SENSOR_AIR_TEMP_DEFAULT_TIMESTAMP (0U)

Status SensorTemperature_ReadInstant(TemperatureSample* out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    uint32_t timestamp = 0U;
    const Status time_status = TimeProvider_ReadTimestamp(&timestamp);

    if (time_status != STATUS_OK) {
        return time_status;
    }

    out_sample->instant_c = SENSOR_MOCK_INSTANT_C;
    out_sample->timestamp = timestamp;
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
