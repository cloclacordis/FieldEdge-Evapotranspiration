/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef GEOLOCATION_CALC_H
#define GEOLOCATION_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Geographic constants of deployment configuration *** ** * */
typedef struct {
    double latitude_deg;   /* Latitude in decimal degrees ** */
    double latitude_rad;   /* Latitude in radians (computed) */
    double elevation_m;    /* Elevation above sea level [m]  */
    bool   initialized;
} LocationData;

/* Convert latitude from degrees-minutes format to decimal degrees;
 * the sign of degrees determines the hemisphere: negative -> southern hemisphere;
 * requirements:
 * - minutes:   0 <= minutes < 60
 * - degrees: -90 <= degrees <= 90 **** * ***** ** **** ******* ***** * *** * *** */
Status Location_DMS_to_decimal(double degrees, double minutes, double* decimal_deg);

/* Initialize geolocation */
Status Location_Init(LocationData* loc);

#ifdef __cplusplus
}
#endif

#endif /* GEOLOCATION_CALC_H */
