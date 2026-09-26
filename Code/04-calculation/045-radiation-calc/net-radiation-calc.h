/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef NET_RADIATION_CALC_H
#define NET_RADIATION_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "solar-radiation-calc.h"
#include "../../03-validation/033-status/status.h"
#include "../041-air-temperature-calc/air-temperature-calc.h"

/* *** * * * ***** * * * **** * * **** * * * **** * ***** ** *** * * *** 
 * Reference crop albedo (eq. 38);
 * type B: mathematical model constant;
 * α = 0.23 - reference value for hypothetical grass cover
 * * * * * * **** *** ** *** * * ***** ** * ** **** * *** * * * * **** */
#define GRASS_ALBEDO (0.23)

/* * * * * * **** *** ** *** * * **** * * * **** * *** * **** * * **** *
 * Stefan-Boltzmann constant (eq. 39); σ = 4.903 * 10⁻⁹ MJ K⁻⁴ m⁻² day⁻¹;
 * derived from SI value (5.67 * 10⁻⁸ W m⁻² K⁻⁴) converted to MJ/day
 * *** * * * ***** * * * **** *** ** **** * * * **** * ***** * * * * ***/
#define STEFAN_BOLTZMANN (4.903e-9)

/* *** * * * ***** * * * **** * * **** * ** * **** * ***** * *** * * ***
 * C -> K conversion (FAO-56 uses 273.16; see p. 52, eq. 39
 * * * * * * **** *** ** *** * ** **** * * *** **** * *** * * * * **** */
#define CELSIUS_TO_KELVIN (273.16)

/* Daily net radiation values */
typedef struct {
    double Rns_daily;      /* Net shortwave radiation [MJ m-2 day-1] * */
    double Rnl_daily;      /* Net longwave radiation  [MJ m-2 day-1] * */
    double Rn_daily;       /* Net radiation           [MJ m-2 day-1] * */
    bool   initialized;    /* Structure initialized *** * * *** * * ** */
} NetRadiationData;

/**
 * @brief Initializes net radiation data to a safe zero state.
 *
 * Unlike the "Layer state" Init functions, also sets
 * `.initialized = true` immediately.
 *
 * @param[out] data Pointer to the NetRadiationData structure to
 *                  initialize. Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status NetRadiation_Init(NetRadiationData *data);

/**
 * @brief Computes daily net radiation (FAO-56, eq. 38-40).
 *
 * Eq. 38: Rns = (1 - α) * Rs, α = 0.23 for hypothetical grass cover.
 * Eq. 39: Rnl = sigma * [(Tmax,K^4 + Tmin,K^4) / 2]
 *         * (0.34 - 0.14 * sqrt(ea)) * (1.35 * Rs/Rso - 0.35).
 * Eq. 40: Rn  = Rns - Rnl.
 *
 * T is converted to Kelvin with FAO-56's eq. 39 constant, 273.16
 * (not 273.15). Rs/Rso is clamped to <= 1.0 and computed as 0
 * (not a division by zero) when Rso = 0 (polar night).
 * The cloudiness factor `1.35 * Rs/Rso - 0.35` is clamped to >= 0,
 * since a very overcast sky would otherwise make it negative,
 * which is not physically meaningful for Rnl.
 *
 * @param[out] out    Destination for the result; must already be
 *                    initialized. Must not be NULL.
 * @param[in]  temp   Accumulated daily temperature data. Must not be
 *                    NULL; `.initialized` true.
 * @param[in]  solar  Solar radiation data. Must not be NULL;
 *                    `.initialized` true, Rs_daily/Rso_daily >= 0.
 * @param[in]  ea_kPa Actual vapour pressure [kPa]. Must be finite
 *                    and >= 0.
 *
 * @retval STATUS_OK            out->Rns_daily, out->Rnl_daily,
 *                              out->Rn_daily are valid.
 * @retval STATUS_NULL_POINTER  out, temp, or solar was NULL.
 * @retval STATUS_INVALID_VALUE Any structure not initialized, ea_kPa
 *                              out of range, Rs_daily/Rso_daily
 *                              negative, or a result was non-finite.
 */
Status Calc_NetRadiation(NetRadiationData *out,
    const AirTemperatureData *temp, const SolarRadiationData *solar, double ea_kPa);

#ifdef __cplusplus
}
#endif

#endif /* NET_RADIATION_CALC_H */
