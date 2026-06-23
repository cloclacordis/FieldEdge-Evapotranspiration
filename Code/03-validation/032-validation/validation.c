/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include "validation.h"

#define VALIDATION_PI (3.14159265358979323846)

bool ValidTemperatureC(const double value) {
    return isfinite(value)
        && (value >= -100.0)
        && (value <= 100.0);
}

bool ValidLatitudeRad(const double phi) {
    return isfinite(phi)
        && (phi >= -(VALIDATION_PI / 2.0))
        && (phi <=  (VALIDATION_PI / 2.0));
}

bool ValidDayOfYear(const uint16_t J) {
    return (J >= 1U) && (J <= 366U);
}
