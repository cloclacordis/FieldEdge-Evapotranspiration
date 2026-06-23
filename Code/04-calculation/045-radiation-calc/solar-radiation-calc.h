/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef SOLAR_RADIATION_CALC_H
#define SOLAR_RADIATION_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "geolocation-calc.h"
#include "day-in-year-calc.h"
#include "sunshine-lux-calc.h"
#include "extrater-radiation-calc.h"
#include "../../03-validation/033-status/status.h"

/* Default Angström-Prescott coefficients  */
#define DEFAULT_ANGSTROM_VALUE_A_S    (0.25)
#define DEFAULT_ANGSTROM_VALUE_B_S    (0.50)

/* Clear-sky radiation coefficient for Rso */
#define CLEAR_SKY_BASE_COEFFICIENT    (0.75)

/* Angström-Prescott coefficient configuration; these values are related
 * not to the daily computation state, but to model calibration parameters */
typedef struct {
    double a_s;            /* Angström coefficient */
    double b_s;            /* Prescott coefficient */
} AngstromValues;

/* Computed daily solar radiation */
typedef struct {
    double Rs_daily;       /* Solar radiation Rs  [MJ m-2 day-1] */
    double Rso_daily;      /* Clear-sky radiation [MJ m-2 day-1] */
    bool   initialized;    /* Structure initialized ** * *** *** */
} SolarRadiationData;

/* Initialize the Rs module structure */
Status SolarRadiation_Init(SolarRadiationData* data);

/* Initialize Angström-Prescott coefficients with default values */
Status AngstromValues_Default(AngstromValues* ang);

/* Calculate solar radiation:
 * Rs  = (a_s  + b_s  * n/N) * Ra (eq. 35);
 * Rso = (0.75 + 2e-5 *   z) * Ra (eq. 37).
 *
 * - n/N is bounded to [0, 1];
 * - during polar night (N = 0) returns Rs = 0 & Rso = 0;
 * - n > N is automatically capped to N;
 * - elevation z is taken from LocationData *** * * * **** * * * */
Status SolarRadiation_Calc(const AngstromValues* ang, SolarRadiationData* out,
    const RaData* ra, const DayData* day, const SunshineLuxData* sunshine, const LocationData* loc);

#ifdef __cplusplus
}
#endif

#endif /* SOLAR_RADIATION_CALC_H */
