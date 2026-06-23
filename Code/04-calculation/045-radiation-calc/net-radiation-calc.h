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
 * C -> K conversion (FAO56 uses 273.16; see 1998: 52, eq. 39
 * * * * * * **** *** ** *** * ** **** * * *** **** * *** * * * * **** */
#define CELSIUS_TO_KELVIN (273.16)

/* Daily net radiation values */
typedef struct {
    double Rns_daily;      /* Net shortwave radiation [MJ m-2 day-1] * */
    double Rnl_daily;      /* Net longwave radiation  [MJ m-2 day-1] * */
    double Rn_daily;       /* Net radiation           [MJ m-2 day-1] * */
    bool   initialized;    /* Structure initialized *** * * *** * * ** */
} NetRadiationData;

/* Initialize structure */
Status NetRadiation_Init(NetRadiationData *data);

/* Compute net radiation:
 * - eq. 38: Rns = (1 - α) * Rs;
 * - eq. 39: Rnl = σ * [(Tmax,K⁴ + Tmin,K⁴) / 2] *
 *                 (0.34 - 0.14√ea) * (1.35 * Rs / Rso - 0.35);
 * - eq. 40: Rn  = Rns - Rnl.
 *
 * Edge cases:
 * - Rs/Rso is capped at 1.0 (FAO56 requirement);
 * - when Rso = 0 (polar night): Rs/Rso = 0, division by zero is avoided;
 * - cloudiness factor is bounded below by 0, because under very overcast skies
 *   (Rs/Rso < 0.26) the factor would become negative; thus Rnl < 0 physically incorrect */
Status Calc_NetRadiation(NetRadiationData *out,
    const AirTemperatureData *temp, const SolarRadiationData *solar, double ea_kPa);

#ifdef __cplusplus
}
#endif

#endif /* NET_RADIATION_CALC_H */
