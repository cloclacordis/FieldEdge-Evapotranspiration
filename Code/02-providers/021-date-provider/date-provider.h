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
 * @brief Reads the current calendar date for the configured station.
 *
 * The UTC timestamp comes from TimeProvider_ReadTimestamp(). The station date
 * is derived using CONFIG_STATION_UTC_OFFSET_MIN; the host computer's time zone
 * and daylight-saving rules do not affect the result.
 *
 * @param[out] date Destination for the date. Must not be NULL.
 *
 * @retval STATUS_OK            *date is valid.
 * @retval STATUS_NULL_POINTER  date was NULL.
 * @retval STATUS_UNAVAILABLE   A trusted timestamp is not available.
 * @retval STATUS_INVALID_VALUE The timestamp/offset could not be converted.
 */
Status DateProvider_Read(DateData *date);

#ifdef __cplusplus
}
#endif

#endif /* DATE_PROVIDER_H */
