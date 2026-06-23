/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_HUMIDITY_READ_H
#define AIR_HUMIDITY_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant relative air humidity RH reading */
typedef struct {
    double            RH_pct;     /* Relative air humidity [%] */
    uint32_t          timestamp;  /* Timestamp [s]             */
    SensorValueSource source;     /* Data source               */
} AirHumiditySample;

/* Read instant air humidity value */
Status SensorHumidity_ReadInstant(AirHumiditySample *out_sample);

/* Fallback value when primary source is unavailable */
Status SensorHumidity_ReadDefault(AirHumiditySample *out_sample);

#ifdef __cplusplus
}
#endif

#endif /* AIR_HUMIDITY_READ_H */
