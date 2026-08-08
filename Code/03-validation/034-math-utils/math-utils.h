/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Portable math constants */
#define PI          (3.14159265358979323846)
#define DEG_TO_RAD  (PI / 180.0)
#define RAD_TO_DEG  (180.0 / PI)

static inline double Min(const double a, const double b) {
    return (a < b) ? a : b;
}

static inline double Max(const double a, const double b) {
    return (a > b) ? a : b;
}

#ifdef __cplusplus
}
#endif

#endif /* MATH_UTILS_H */
