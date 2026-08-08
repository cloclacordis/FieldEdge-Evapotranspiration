/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <string.h>
#include "air-humidity-calc.h"
#include "../../03-validation/032-validation/validation.h"

Status AirHumidity_Init(AirHumidityData *data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    memset(data, 0, sizeof(*data));

    return STATUS_OK;
}

Status AirHumidity_Update(AirHumidityData *data, const double RH_pct, const uint32_t timestamp) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!ValidHumidityPercent(RH_pct)) {
        return STATUS_INVALID_VALUE;
    }

    data->timestamp = timestamp;

    /* 1st valid input initializes min & max */
    if (!data->initialized) {
        data->RH_max      = RH_pct;
        data->RH_min      = RH_pct;
        data->initialized = true;
    } else {
        if (RH_pct > data->RH_max) {
            data->RH_max = RH_pct;
        }
        if (RH_pct < data->RH_min) {
            data->RH_min = RH_pct;
        }
    }

    data->RH_mean = (data->RH_max + data->RH_min) / 2.0;

    return STATUS_OK;
}
