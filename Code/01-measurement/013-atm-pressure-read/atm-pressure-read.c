/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <time.h>
#include <stddef.h>
#include "atm-pressure-read.h"

/* Mock: sea-level pressure */
#define SENSOR_MOCK_P_KPA                 (101.3)
#define SENSOR_DEFAULT_P_KPA              (101.3)
#define SENSOR_PRESSURE_DEFAULT_TIMESTAMP (0U)

Status SensorPressure_ReadInstant(AtmPressureSample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->P_kPa     = SENSOR_MOCK_P_KPA;
    out_sample->timestamp = (uint32_t)time(NULL);
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
