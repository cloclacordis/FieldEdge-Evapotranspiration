/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef WIND_SPEED_READ_H
#define WIND_SPEED_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant wind speed reading */
typedef struct {
    double            speed_m_s;    /* Instant wind speed [m/s] */
    double            height_m;     /* Measurement height [m]   */
    uint32_t          timestamp;    /* Timestamp [s]            */
    SensorValueSource source;       /* Data source              */
} WindSpeedSample;

/* Read instant wind speed value */
Status SensorWindSpeed_ReadInstant(WindSpeedSample *out_sample);

/* Fallback value */
Status SensorWindSpeed_ReadDefault(WindSpeedSample *out_sample);

#ifdef __cplusplus
}
#endif

#endif /* WIND_SPEED_READ_H */
