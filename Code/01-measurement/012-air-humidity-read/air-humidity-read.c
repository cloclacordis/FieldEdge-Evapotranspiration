/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

// #include <time.h>
#include <stddef.h>
#include "air-humidity-read.h"
#include "../../02-providers/021-date-provider/time-provider.h"

/* PC mock */
#define SENSOR_MOCK_INSTANT_RH_PCT        (70.0)
#define SENSOR_DEFAULT_INSTANT_RH_PCT     (70.0)
#define SENSOR_HUMIDITY_DEFAULT_TIMESTAMP (0U)

Status SensorHumidity_ReadInstant(AirHumiditySample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->RH_pct    = SENSOR_MOCK_INSTANT_RH_PCT;

    // out_sample->timestamp = (uint32_t)time(NULL);

    /* Smoke-test timestamp */
    Status status = TimeProvider_Read(&out_sample->timestamp);
    if (status != STATUS_OK) {
        return status;
    }

    out_sample->source    = SENSOR_VALUE_MEASURED;

    return STATUS_OK;
}

Status SensorHumidity_ReadDefault(AirHumiditySample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->RH_pct    = SENSOR_DEFAULT_INSTANT_RH_PCT;
    out_sample->timestamp = SENSOR_HUMIDITY_DEFAULT_TIMESTAMP;
    out_sample->source    = SENSOR_VALUE_DEFAULT;

    return STATUS_OK;
}
