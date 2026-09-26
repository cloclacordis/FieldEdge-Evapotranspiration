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

/**
 * @brief Sets Angstrom-Prescott coefficients to their FAO-56 default
 *        values (a_s = 0.25, b_s = 0.50).
 *
 * @param[out] ang Destination structure. Must not be NULL.
 *
 * @retval STATUS_OK           *ang is valid.
 * @retval STATUS_NULL_POINTER ang was NULL.
 */
Status AngstromValues_Default(AngstromValues* ang);

/**
 * @brief Initializes solar radiation data to a safe zero state.
 *
 * Unlike the "Layer state" Init functions, also sets
 * `.initialized = true` immediately.
 *
 * @param[out] data Pointer to the SolarRadiationData structure to
 *                  initialize. Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status SolarRadiation_Init(SolarRadiationData* data);

/**
 * @brief Computes daily solar and clear-sky radiation (FAO-56, eq. 35, 37).
 *
 * Eq. 35: Rs  = (a_s + b_s * n/N) * Ra.
 * Eq. 37: Rso = (0.75 + 2e-5 * z) * Ra.
 *
 * n/N is clamped to [0, 1] (actual sunshine duration cannot exceed
 * the possible daylight duration). During polar night (N = 0),
 * returns STATUS_OK early with Rs = Rso = 0, bypassing
 * the eq. 35/37 arithmetic entirely.
 *
 * @param[in]  ang      Angstrom-Prescott coefficients. Must not be
 *                      NULL; a_s >= 0, b_s >= 0, a_s + b_s <= 1.
 * @param[out] out      Destination for the result; must already be
 *                      initialized. Must not be NULL.
 * @param[in]  ra       Extraterrestrial radiation. Must not be NULL;
 *                      `.initialized` true, `.Ra_daily` >= 0.
 * @param[in]  day      Day/astronomy data. Must not be NULL;
 *                      `.initialized` true, `.N_hours` >= 0.
 * @param[in]  sunshine Accumulated sunshine data. Must not be NULL;
 *                      `.initialized` true, `.n_hours` >= 0.
 * @param[in]  loc      Location data (for elevation z). Must not be
 *                      NULL; `.initialized` true.
 *
 * @retval STATUS_OK            out->Rs_daily and out->Rso_daily are
 *                              valid (including the polar-night
 *                              zero case).
 * @retval STATUS_NULL_POINTER  Any parameter was NULL.
 * @retval STATUS_INVALID_VALUE Any structure not initialized, ang out
 *                              of range, any of Ra_daily / N_hours /
 *                              n_hours negative, or the computed
 *                              Rs/Rso was non-finite or negative.
 */
Status SolarRadiation_Calc(const AngstromValues* ang, SolarRadiationData* out,
    const RaData* ra, const DayData* day, const SunshineLuxData* sunshine, const LocationData* loc);

#ifdef __cplusplus
}
#endif

#endif /* SOLAR_RADIATION_CALC_H */
