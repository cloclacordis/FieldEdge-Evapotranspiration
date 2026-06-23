/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef PSYCHROMETRIC_CALC_H
#define PSYCHROMETRIC_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "../../03-validation/033-status/status.h"

/* Coefficient for the simplified form (eq. 8): γ = 0.000665 * P */
#define PSYCHROMETRIC_GAMMA_COEFF (0.000665)

/* Atm parameters struct; P_kPa source hierarchy: sensor -> model (eq. 7) -> constant */
typedef struct {
    double P_kPa;            /* Atmospheric pressure [kPa] * * * * ****** * ***** * * */
    double gamma_kPa_per_C;  /* Psychrometric constant [kPa/C] *** * **** * * *** *** */
    bool   initialized;
} AtmosphericData;

/* Structure initialization */
Status AtmosphericData_Init(AtmosphericData *data);

/* Calculate γ from P (eq. 8): γ = 0.000665 * P;
 * takes P_kPa as a double and does not know the source of P;
 * stores P_kPa in the structure so that P & γ are kept as a pair */
Status Calc_AtmosphericParameters(AtmosphericData *out, double P_kPa);

#ifdef __cplusplus
}
#endif

#endif /* PSYCHROMETRIC_CALC_H */
