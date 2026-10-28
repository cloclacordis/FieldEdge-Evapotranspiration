/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef TIME_PROVIDER_H
#define TIME_PROVIDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "../../03-validation/033-status/status.h"

/* The RTC year field is two digits; RTC_ISR.INITS is set only when the year field is nonzero;
 * since year 00 represents 2000, the supported application range begins in 2001 *** *** *** */
#define TIME_PROVIDER_MIN_EPOCH UINT32_C(978307200)   /* 2001-01-01T00:00:00Z *** ** **** ** */
#define TIME_PROVIDER_MAX_EPOCH UINT32_C(4102444799)  /* 2099-12-31T23:59:59Z * ** **** **** */

/**
 * @brief Reads the current UTC instant as seconds since 1970-01-01T00:00:00Z.
 *
 * This is the shared time-source interface used by measurement and calendar providers
 * The implementation is platform-specific. Application code does not depend on
 * RTC registers or the host's time-zone settings.
 *
 * @param[out] out_timestamp    Destination [s]; must not be NULL and is unchanged
 *                              when the operation fails.
 *
 * @retval STATUS_OK            A timestamp in the supported application range.
 * @retval STATUS_NULL_POINTER  out_timestamp was NULL.
 * @retval STATUS_UNAVAILABLE   The clock is unset, untrusted, or out of range.
 */
Status TimeProvider_ReadTimestamp(uint32_t *out_timestamp);

#ifdef __cplusplus
}
#endif

#endif /* TIME_PROVIDER_H */
