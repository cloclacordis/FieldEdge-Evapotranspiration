/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef SUNSHINE_LUX_READ_H
#define SUNSHINE_LUX_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Instant illuminance value */
typedef struct {
    double            lux;          /* Illuminance [lux] */
    uint32_t          timestamp;    /* Timestamp         */
    SensorValueSource source;       /* Data source       */
} SunshineLuxSample;

/* Read instant illuminance value */
Status SensorLux_ReadInstant(SunshineLuxSample* out_sample);

/* Fallback value when primary source is unavailable */
Status SensorLux_ReadDefault(SunshineLuxSample* out_sample);

#ifdef __cplusplus
}
#endif

#endif /* SUNSHINE_LUX_READ_H */
