/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef ATM_PRESSURE_READ_H
#define ATM_PRESSURE_READ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Data structure for instant atmospheric pressure reading.
 * The pressure fallback hierarchy is defined in orchestration:
 * 1. SensorPressure_ReadInstant() -> actual sensor data (PC mock)
 * 2. Calc_PressureFromElevation() -> see atm-pressure-model.c
 * 3. SensorPressure_ReadDefault() -> the last recourse * ** ** */
typedef struct {
    double            P_kPa;      /* Atmospheric pressure [kPa] */
    uint32_t          timestamp;  /* Timestamp                  */
    SensorValueSource source;     /* Data source                */
} AtmPressureSample;

/* Read instant atmospheric pressure value */
Status SensorPressure_ReadInstant(AtmPressureSample *out_sample);

/* Fallback: standard sea-level pressure */
Status SensorPressure_ReadDefault(AtmPressureSample *out_sample);

#ifdef __cplusplus
}
#endif

#endif /* ATM_PRESSURE_READ_H */
