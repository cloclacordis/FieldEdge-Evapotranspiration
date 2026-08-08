/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <string.h>
#include "air-temperature-calc.h"
#include "../../03-validation/032-validation/validation.h"

Status AirTemperature_Init(AirTemperatureData* data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    memset(data, 0, sizeof(*data));

    return STATUS_OK;
}

Status AirTemperature_Update(AirTemperatureData* data, const double T_inst_C, const uint32_t timestamp) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!ValidTemperatureC(T_inst_C)) {
        return STATUS_INVALID_VALUE;
    }

    data->timestamp = timestamp;

    /* First valid input */
    if (data->initialized == false) {
        data->T_min_C = T_inst_C;
        data->T_max_C = T_inst_C;
        data->initialized = true;
    } else {
        if (T_inst_C > data->T_max_C) {
            data->T_max_C = T_inst_C;
        }
        if (T_inst_C < data->T_min_C) {
            data->T_min_C = T_inst_C;
        }
    }

    /* Mean air temperature [C] */
    data->T_mean_C = (data->T_max_C + data->T_min_C) / 2.0;

    return STATUS_OK;
}
