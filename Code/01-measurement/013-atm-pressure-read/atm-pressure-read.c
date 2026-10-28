/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdint.h>
#include <stddef.h>
#include "atm-pressure-read.h"
#include "../../02-providers/023-time/time-provider.h"

/* PC mock */
#define SENSOR_MOCK_P_KPA                 (101.3)
#define SENSOR_DEFAULT_P_KPA              (101.3)  /* Standard sea-level pressure */
#define SENSOR_PRESSURE_DEFAULT_TIMESTAMP (0U)

Status SensorPressure_ReadInstant(AtmPressureSample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    uint32_t timestamp = 0U;
    const Status time_status = TimeProvider_ReadTimestamp(&timestamp);

    if (time_status != STATUS_OK) {
        return time_status;
    }

    out_sample->P_kPa     = SENSOR_MOCK_P_KPA;
    out_sample->timestamp = timestamp;
    out_sample->source    = SENSOR_VALUE_MEASURED;

    return STATUS_OK;
}

Status SensorPressure_ReadDefault(AtmPressureSample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->P_kPa     = SENSOR_DEFAULT_P_KPA;
    out_sample->timestamp = SENSOR_PRESSURE_DEFAULT_TIMESTAMP;
    out_sample->source    = SENSOR_VALUE_DEFAULT;

    return STATUS_OK;
}
