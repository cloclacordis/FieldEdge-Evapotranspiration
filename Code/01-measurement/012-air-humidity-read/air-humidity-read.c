/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdint.h>
#include <stddef.h>
#include "air-humidity-read.h"
#include "../../02-providers/023-time/time-provider.h"

/* PC mock */
#define SENSOR_MOCK_INSTANT_RH_PCT        (70.0)
#define SENSOR_DEFAULT_INSTANT_RH_PCT     (70.0)
#define SENSOR_HUMIDITY_DEFAULT_TIMESTAMP (0U)

Status SensorHumidity_ReadInstant(AirHumiditySample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    uint32_t timestamp = 0U;
    const Status time_status = TimeProvider_ReadTimestamp(&timestamp);

    if (time_status != STATUS_OK) {
        return time_status;
    }

    out_sample->RH_pct    = SENSOR_MOCK_INSTANT_RH_PCT;
    out_sample->timestamp = timestamp;
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
