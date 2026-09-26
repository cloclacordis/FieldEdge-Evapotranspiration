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

/**
 * @brief Initializes extraterrestrial radiation data to a safe zero state.
 *
 * @param[out] data Pointer to the RaData structure to initialize.
 *                  Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status RaCalc_Init(RaData* data);

/**
 * @brief Computes daily extraterrestrial radiation (FAO-56, eq. 21).
 *
 * Ra = (24*60 / pi) * Gsc * dr * [omega_s*sin(phi)*sin(delta) +
 *      cos(phi)*cos(delta)*sin(omega_s)]
 *
 * During polar night (omega_s = 0, from DayCalc_Update()), both terms
 * vanish and Ra = 0 - no special case needed here.
 *
 * @param[out] out Destination for the result. Must not be NULL.
 * @param[in]  day Day/astronomy data. Must not be NULL;
 *                 `.initialized` must be true.
 * @param[in]  loc Location data. Must not be NULL; `.initialized`
 *                 must be true.
 *
 * @retval STATUS_OK            out->Ra_daily is valid.
 * @retval STATUS_NULL_POINTER  out, day, or loc was NULL.
 * @retval STATUS_INVALID_VALUE day or loc not initialized.
 */
Status Calc_Ra(RaData* out, const DayData* day, const LocationData* loc);

#ifdef __cplusplus
}
#endif

#endif /* EXTRATER_RADIATION_CALC_H */
