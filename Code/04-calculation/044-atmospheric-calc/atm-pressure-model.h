/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef ATM_PRESSURE_MODEL_H
#define ATM_PRESSURE_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../../03-validation/033-status/status.h"

/**
 * @brief Estimates atmospheric pressure from elevation (FAO-56, eq. 7).
 *
 * P = 101.3 * [(293 - 0.0065 * z) / 293]^5.26.
 *
 * Used as the second-priority fallback (after the sensor, before the
 * fixed constant) when the pressure sensor is unavailable - see
 * data-flow-specification.md.
 *
 * @param[in]  elevation_m Station elevation above sea level [m].
 *                         Must be in [-500, 6000].
 * @param[out] P_kPa       Destination for the result [kPa]. Must
 *                         not be NULL.
 *
 * @retval STATUS_OK            *P_kPa is valid.
 * @retval STATUS_NULL_POINTER  P_kPa was NULL.
 * @retval STATUS_INVALID_VALUE elevation_m out of range, or the
 *                              result was non-finite or non-positive.
 *
 * @note "The effect is, however, small and in the calculation
 *       procedures, the average value for a location is sufficient"
 *       (FAO-56, p. 31).
 */
Status Calc_PressureFromElevation(double elevation_m, double *P_kPa);

#ifdef __cplusplus
}
#endif

#endif /* ATM_PRESSURE_MODEL_H */
