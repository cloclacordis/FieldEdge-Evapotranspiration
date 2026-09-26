/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef VALIDATION_H
#define VALIDATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Checks whether a value is within the accepted
 *        air-temperature range.
 *
 * @param[in] value Candidate temperature [C].
 *
 * @return true iff isfinite(value) && -100.0 <= value <= 100.0.
 *
 * @note The range is a temporary protective corridor, not a
 *       climate-derived bound.
 */
bool ValidTemperatureC(double value);

/**
 * @brief Checks whether a value is a valid relative humidity.
 *
 * @param[in] value Candidate relative humidity [%].
 *
 * @return true iff isfinite(value) && 0.0 <= value <= 100.0.
 */
bool ValidHumidityPercent(double value);

/**
 * @brief Checks whether a value is a valid latitude, in radians.
 *
 * @param[in] phi Candidate latitude [rad].
 *
 * @return true iff isfinite(phi) && -pi/2 <= phi <= +pi/2. The full
 *         range, including the poles, is intentionally accepted.
 */
bool ValidLatitudeRad(double phi);

/**
 * @brief Checks whether a value is a valid day-of-year value.
 *
 * @param[in] J Candidate day of year.
 *
 * @return true iff 1 <= J <= 366. 366 is always accepted.
 *         Whether the year associated with J is a leap year
 *         is outside the scope of this function.
 */
bool ValidDayOfYear(uint16_t J);

#ifdef __cplusplus
}
#endif

#endif /* VALIDATION_H */
