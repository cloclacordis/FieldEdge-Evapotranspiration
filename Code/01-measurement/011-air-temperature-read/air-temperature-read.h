/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_TEMPERATURE_READ_H
#define AIR_TEMPERATURE_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant air temperature reading [C] */
typedef struct {
    double            instant_c;
    uint32_t          timestamp;
    SensorValueSource source;
} TemperatureSample;

/* Emulate instant air temperature reading */
Status SensorTemperature_ReadInstant(TemperatureSample* out_sample);

/* Default (fallback) value: if measurement is unavailable or data is corrupted */
Status SensorTemperature_ReadDefault(TemperatureSample* out_sample);

#ifdef __cplusplus
}
#endif

#endif /* AIR_TEMPERATURE_READ_H */
