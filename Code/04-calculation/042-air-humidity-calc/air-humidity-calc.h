/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef AIR_HUMIDITY_CALC_H
#define AIR_HUMIDITY_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Accumulated daily relative humidity values * **** *** */
typedef struct {
    double   RH_max;      /* Maximum daily humidity [%]  */
    double   RH_min;      /* Minimum daily humidity [%]  */
    double   RH_mean;     /* Mean daily humidity    [%]  */
    uint32_t timestamp;   /* Time of last update * ** ** */
    bool     initialized;
} AirHumidityData;

/* Structure initialization, STATUS_OK: zeroed, initialized = false */
Status AirHumidity_Init(AirHumidityData *data);

/* Update daily min/max/mean with a single instant measurement.
 * RH% [0, 100], first valid input initializes min & max * ** *** * */
Status AirHumidity_Update(AirHumidityData *data, double RH_pct, uint32_t timestamp);

#ifdef __cplusplus
}
#endif

#endif /* AIR_HUMIDITY_CALC_H */
