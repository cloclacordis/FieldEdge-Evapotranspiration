/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef DATE_PROVIDER_H
#define DATE_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"

typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
} DateData;

/**
 * @brief Reads the current calendar date from the host system clock.
 *
 * Wraps time() and localtime(). Uses the host's configured local
 * time zone. This is a known open question for the RTC-based
 * replacement in v0.2.x (see README, "Limitations").
 *
 * @param[out] date Destination for the date. Must not be NULL.
 *
 * @retval STATUS_OK            *date is valid.
 * @retval STATUS_NULL_POINTER  date was NULL.
 * @retval STATUS_INVALID_VALUE `time()` or `localtime()`
 *                              failed (unable to obtain the date).
 */
Status DateProvider_Read(DateData* date);

#ifdef __cplusplus
}
#endif

#endif /* DATE_PROVIDER_H */