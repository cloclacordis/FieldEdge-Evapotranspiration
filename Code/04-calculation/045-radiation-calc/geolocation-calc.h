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

/**
 * @brief Converts a latitude from degrees-minutes to decimal degrees.
 *
 * The sign of @p degrees determines the hemisphere and is applied to
 * the combined 'degrees + minutes' magnitude (a negative @p minutes is
 * rejected, not treated as a second sign).
 *
 * @param[in]  degrees     Degrees component. Must be in [-90, 90].
 * @param[in]  minutes     Minutes component. Must be in [0, 60).
 * @param[out] decimal_deg Destination for the result [decimal
 *                         degrees]. Must not be NULL.
 *
 * @retval STATUS_OK            *decimal_deg is valid.
 * @retval STATUS_NULL_POINTER  decimal_deg was NULL.
 * @retval STATUS_INVALID_VALUE degrees or minutes out of range.
 *
 * @note   The sign of @p degrees determines the hemisphere:
 *         negative -> southern hemisphere.
 */
Status Location_DMS_to_decimal(double degrees, double minutes, double* decimal_deg);

/**
 * @brief Initializes location data from deployment configuration.
 *
 * Reads CONFIG_ELEVATION_M, CONFIG_LATITUDE_DEG, and
 * CONFIG_LATITUDE_MIN (deployment-config.h); converts
 * latitude via Location_DMS_to_decimal() and to radians.
 *
 * @param[out] loc Pointer to the LocationData structure to
 *                 initialize. Must not be NULL.
 *
 * @post On success, `.latitude_deg`, `.latitude_rad`, `.elevation_m`
 *       are set from configuration and `.initialized` is true.
 *
 * @retval STATUS_OK            Initialization succeeded.
 * @retval STATUS_NULL_POINTER  loc was NULL.
 * @retval STATUS_INVALID_VALUE Propagated verbatim from
 *                              Location_DMS_to_decimal(); in practice
 *                              unreachable with the current constants.
 */
Status Location_Init(LocationData* loc);

#ifdef __cplusplus
}
#endif

#endif /* GEOLOCATION_CALC_H */
