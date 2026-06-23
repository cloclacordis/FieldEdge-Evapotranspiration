/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_TEMPERATURE_CALC_H
#define AIR_TEMPERATURE_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Structure for storing computed temperature values */
typedef struct {
    double   T_max_C;
    double   T_min_C;
    double   T_mean_C;
    uint32_t timestamp;     /* Time of last update   */
    bool     initialized;   /* Initialization flag   */
} AirTemperatureData;

/* Explicit structure initialization */
Status AirTemperature_Init(AirTemperatureData* data);

/* Update state with a single measurement. The function handles:
 * input validation; initialization on first valid value; min & max update; mean calculation */
Status AirTemperature_Update(AirTemperatureData* data, double T_inst_C, uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* AIR_TEMPERATURE_CALC_H */
