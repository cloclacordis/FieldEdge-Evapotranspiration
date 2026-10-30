/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdint.h>
#include <stddef.h>
#include "wind-speed-read.h"
#include "../../02-providers/023-time/time-provider.h"
#include "../../02-providers/022-configurations/deployment-config.h"

/* PC mock: speed 3.2 m/s (ex. 14), measurement height 10 m */
#define SENSOR_MOCK_WIND_SPEED_MS     (3.2)

/* Fallback values: speed 2.4 m/s (ex. 14), measurement height 2 m */
#define SENSOR_DEFAULT_WIND_SPEED_MS  (2.4)
#define SENSOR_WIND_DEFAULT_TIMESTAMP (0U)

Status SensorWindSpeed_ReadInstant(WindSpeedSample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    uint32_t timestamp = 0U;
    const Status time_status = TimeProvider_ReadTimestamp(&timestamp);

    if (time_status != STATUS_OK) {
        return time_status;
    }

    out_sample->speed_m_s = SENSOR_MOCK_WIND_SPEED_MS;
    out_sample->height_m  = CONFIG_WIND_HEIGHT_WMO_M;   /* WMO standard: 10 m (standard meteorological stations) */
    out_sample->timestamp = timestamp;
    out_sample->source    = SENSOR_VALUE_MEASURED;

    return STATUS_OK;
}

Status SensorWindSpeed_ReadDefault(WindSpeedSample *out_sample) {
    if (out_sample == NULL) {
        return STATUS_NULL_POINTER;
    }

    out_sample->speed_m_s = SENSOR_DEFAULT_WIND_SPEED_MS;
    out_sample->height_m  = CONFIG_WIND_HEIGHT_FAO_M;   /* FAO56 standard: 2 m (agrometeorological stations) ** **/
    out_sample->timestamp = SENSOR_WIND_DEFAULT_TIMESTAMP;
    out_sample->source    = SENSOR_VALUE_DEFAULT;

    return STATUS_OK;
}
