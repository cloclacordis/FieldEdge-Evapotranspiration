/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef VALIDATION_H
#define VALIDATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* Temporary protective corridor for temperature values */
bool ValidTemperatureC(double value);

/* Relative humidity as measured; true iff isfinite(value) && 0 <= value <= 100 */
bool ValidHumidityPercent(double value);

/* Range covers the entire Earth; poles are intentionally included *** * ** *** * * *** *** *** */
bool ValidLatitudeRad(double phi);  /* true iff isfinite(phi) && phi >= -(π/2) && phi <= +(π/2) */

/* Leap year: 366 is allowed; year validation is outside the scope of this function */
bool ValidDayOfYear(uint16_t J);    /* true iff J >= 1 && J <= 366 *** * ** *** * * */

#ifdef __cplusplus
}
#endif

#endif /* VALIDATION_H */
