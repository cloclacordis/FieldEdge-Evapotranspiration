/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <stddef.h>
#include "geolocation-calc.h"
#include "../../02-providers/022-configurations/deployment-config.h"

/* Constant for degrees to radians conversion */
#define DEG_TO_RAD (3.14159265358979323846 / 180.0)

/* Convert latitude from DMS to decimal degrees */
Status Location_DMS_to_decimal(const double degrees, const double minutes, double* decimal_deg) {
    if (decimal_deg == NULL) {
        return STATUS_NULL_POINTER;
    }

    /* Check minutes range */
    if ((minutes < 0.0) || (minutes >= 60.0)) {
        return STATUS_INVALID_VALUE;
    }

    /* Check degrees range */
    if ((degrees < -90.0) || (degrees > 90.0)) {
        return STATUS_INVALID_VALUE;
    }

    /* The sign of degrees "-" applies to both degrees & minutes */
    const double sign = (degrees < 0.0) ? (-1.0) : (1.0);

    *decimal_deg = sign * (fabs(degrees) + (minutes / 60.0));

    return STATUS_OK;
}

/* Initialize geolocation */
Status Location_Init(LocationData* loc) {
    if (loc == NULL) {
        return STATUS_NULL_POINTER;
    }

    loc->elevation_m = CONFIG_ELEVATION_M;    /* Sea level */

    /* Convert latitude from DMS to decimal degrees */
    Status status = Location_DMS_to_decimal(CONFIG_LATITUDE_DEG, CONFIG_LATITUDE_MIN, &loc->latitude_deg);

    if (status != STATUS_OK) {
        return status;
    }

    /* Convert latitude to radians */
    loc->latitude_rad = loc->latitude_deg * DEG_TO_RAD;

    loc->initialized = true;

    return STATUS_OK;
}
