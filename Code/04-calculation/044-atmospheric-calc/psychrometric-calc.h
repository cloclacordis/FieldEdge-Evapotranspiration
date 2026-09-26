/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef PSYCHROMETRIC_CALC_H
#define PSYCHROMETRIC_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Coefficient for the simplified form (eq. 8): γ = 0.000665 * P *** * **** * * ** ** */
#define PSYCHROMETRIC_GAMMA_COEFF (0.000665) /* 0.665 * 10^-3 (or 0.665e-3) *** * *** */

/* Atm parameters struct; P_kPa source hierarchy: sensor -> model (eq. 7) -> constant */
typedef struct {
    double P_kPa;            /* Atmospheric pressure [kPa] * * * * ****** * ***** * * */
    double gamma_kPa_per_C;  /* Psychrometric constant [kPa/C] *** * **** * * *** *** */
    bool   initialized;
} AtmosphericData;

/**
 * @brief Initializes atmospheric parameter data to a safe zero state.
 *
 * Unlike the "Layer state" Init functions, this one also sets
 * `.initialized = true` immediately - the structure is usable right
 * after Init(), before any Calc_AtmosphericParameters() call.
 *
 * @param[out] data Pointer to the AtmosphericData structure to
 *                  initialize. Must not be NULL.
 *
 * @post On success, `.P_kPa` and `.gamma_kPa_per_C` are zero and
 *       `.initialized` is true.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status AtmosphericData_Init(AtmosphericData *data);

/**
 * @brief Computes psychrometric constant from pressure (FAO-56, eq. 8).
 *
 * gamma = 0.000665 * P. Takes P_kPa as a plain double and has no
 * knowledge of where it came from (sensor, elevation model, or
 * constant) - see data-flow-specification.md ("Derived intermediates").
 * P_kPa is stored alongside gamma_kPa_per_C purely so the pair stays
 * together for reporting.
 *
 * @param[in,out] out   Structure to update; must already be
 *                      initialized (see AtmosphericData_Init()).
 *                      Must not be NULL.
 * @param[in]     P_kPa Resolved atmospheric pressure [kPa]. Must be
 *                      finite and in [50, 120].
 *
 * @retval STATUS_OK            out->P_kPa and out->gamma_kPa_per_C
 *                              are valid.
 * @retval STATUS_NULL_POINTER  out was NULL.
 * @retval STATUS_INVALID_VALUE out was not initialized, P_kPa was out
 *                              of range or non-finite, or the computed
 *                              gamma was non-finite.
 */
Status Calc_AtmosphericParameters(AtmosphericData *out, double P_kPa);

#ifdef __cplusplus
}
#endif

#endif /* PSYCHROMETRIC_CALC_H */
