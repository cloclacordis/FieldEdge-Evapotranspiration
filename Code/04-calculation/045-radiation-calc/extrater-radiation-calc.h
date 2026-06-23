/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef EXTRATER_RADIATION_CALC_H
#define EXTRATER_RADIATION_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "day-in-year-calc.h"
#include "geolocation-calc.h"
#include "../../03-validation/033-status/status.h"

/* Result for daily period */
typedef struct {
    double Ra_daily;       /* Extraterrestrial radiation [MJ m2 day] (eq. 21) */
    bool   initialized;
} RaData;

/* Zero-initialize the structure */
Status RaCalc_Init(RaData* data);

/* Compute Ra for daily period (eq. 21) */
Status Calc_Ra(RaData* out, const DayData* day, const LocationData* loc);

#ifdef __cplusplus
}
#endif

#endif /* EXTRATER_RADIATION_CALC_H */
