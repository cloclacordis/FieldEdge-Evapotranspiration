/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef ATM_PRESSURE_MODEL_H
#define ATM_PRESSURE_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../03-validation/033-status/status.h"

/* **** * * * **** * ** * *** * * * * *** * * * ****** * * * *** * *** * * * * * *
 * Model-based atmospheric pressure calculation from elevation above sea level.
 * FAO56 eq. 7: P = 101.3 * [(293 - 0.0065z) / 293]^5.26
 *
 * Used as fallback when barometric sensor is unavailable.
 * FAO56: "The effect is, however, small and in the calculation procedures,
 *         the average value for a location is sufficient."
 * ** * * * * * *** * ** * ** ***   ** ** * * * ** *** * * *** * **** * ** * *** */

/* Eq. 7: atmospheric pressure as a function of elevation [m] */
Status Calc_PressureFromElevation(double elevation_m, double *P_kPa);

#ifdef __cplusplus
}
#endif

#endif /* ATM_PRESSURE_MODEL_H */
