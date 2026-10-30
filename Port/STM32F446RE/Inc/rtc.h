/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "03-validation/033-status/status.h"

/* Can the stored time be believed? Three separate facts are checked, in this
 * order of precedence:
 *   1. the RTC clock - LSE running, RTC clocked from it and actually ticking;
 *   2. the calendar  - set at least once since the last backup-domain reset
 *                      (RTC_ISR.INITS) and decodable into a real date;
 *   3. the trust     - set by RTC_SetEpoch() of this firmware (backup marker
 *                      plus last-set timestamp), configuration unchanged,
 *                      and the instant has not moved backwards. */
typedef enum {
    RTC_STATE_CLOCK_FAULT = 0, /* LSE/RTC not running, or RTC clocked from a source other than LSE */
    RTC_STATE_NOT_SET,         /* Clock runs, calendar still in its reset state (never set / VBAT was lost) */
    RTC_STATE_UNCONFIRMED,     /* Calendar holds something, but it was not set by RTC_SetEpoch() or is implausible */
    RTC_STATE_TRUSTED          /* RTC_ReadEpoch() returns the time */
} RTCState;

/**
 * @brief Configures the RTC hardware and checks that its clock runs.
 *
 * Idempotent; safe after any reset. Starts the LSE (32.768 kHz crystal),
 * selects it as the RTC clock and enables the RTC - each step only if it has
 * not been done yet - then waits for the shadow registers to synchronize and
 * checks that the sub-second counter really changes.
 *
 * Never sets the time or the date, never changes a running calendar, and
 * never resets the backup domain (see RTC_ResetBackupDomain()). Whether the
 * time is trustworthy is reported by RTC_GetState(), not by this function:
 * a clock that runs with an unset calendar is a successful initialization.
 *
 * Blocks for up to about 5 s (LSE start-up) plus 1 s (synchronization) if the
 * crystal does not start; normally it returns within milliseconds. Timeouts
 * are measured with the DWT cycle counter, which this function enables.
 *
 * @retval STATUS_OK             The RTC clock runs.
 * @retval STATUS_UNAVAILABLE    The LSE did not become ready, registers did
 *                               not synchronize, or the counter does not tick.
 * @retval STATUS_INTERNAL_ERROR The backup domain is configured for another
 *                               clock source (RTCSEL) or for an external clock
 *                               (LSEBYP). Only a backup-domain reset can fix it.
 */
Status RTC_Init(void);

/**
 * @brief Evaluates the current state of the RTC and of the stored time.
 *
 * Checks that the sub-second counter advances, then evaluates the calendar.
 * If ticking has stopped, latches a clock fault until RTC_Init() succeeds again.
 * Any non-trusted result clears the trust marker so a calendar rollback cannot
 * silently become trusted later. The liveness check may wait up to 25 ms.
 *
 * @return The state; RTC_STATE_CLOCK_FAULT if RTC_Init() has not succeeded or
 *         the RTC has stopped ticking.
 */
RTCState RTC_GetState(void);

/**
 * @brief Returns a human-readable name for an RTCState value.
 *
 * @param[in] state An RTCState value. Values not matching any defined
 *                  enumerator are returned as "RTC_STATE_UNKNOWN".
 *
 * @return A static string naming @p state.
 */
const char *RTCState_ToString(RTCState state);

/**
 * @brief Reads the current instant: UTC seconds since 1970-01-01T00:00:00Z.
 *
 * Verifies that the sub-second counter advances, then reads a consistent
 * shadow-register snapshot using the STM32F446 erratum workaround. The liveness
 * check may wait up to 25 ms. Time is returned only when the
 * RTC state is RTC_STATE_TRUSTED.
 *
 * @param[out] out_epoch Destination [s]. Must not be NULL; unchanged on failure.
 *
 * @retval STATUS_OK           *out_epoch is valid.
 * @retval STATUS_NULL_POINTER out_epoch was NULL.
 * @retval STATUS_UNAVAILABLE  The state is not RTC_STATE_TRUSTED.
 */
Status RTC_ReadEpoch(uint32_t *out_epoch);

/**
 * @brief Sets the RTC to the given UTC instant and marks it as trusted.
 *
 * The only way to obtain RTC_STATE_TRUSTED. Meant for the service procedure
 * (see commissioning-rtc.md), never to be called at start-up with a constant.
 * Order of operations: marker cleared, calendar written in initialization mode
 * (24-hour format, fixed prescalers), marker written, registers resynchronized,
 * value read back and compared. If anything fails the marker stays cleared,
 * and the calendar is never left in initialization mode.
 *
 * @param[in] epoch_utc UTC seconds since 1970-01-01T00:00:00Z; must lie
 *                      within TIME_PROVIDER_MIN_EPOCH ... TIME_PROVIDER_MAX_EPOCH.
 *
 * @retval STATUS_OK             Set, read back, state is RTC_STATE_TRUSTED.
 * @retval STATUS_INVALID_VALUE  epoch_utc outside the allowed window.
 * @retval STATUS_UNAVAILABLE    RTC_Init() has not succeeded, or the RTC did
 *                               not enter initialization mode / synchronize.
 * @retval STATUS_INTERNAL_ERROR The read-back differs from the value written.
 */
Status RTC_SetEpoch(uint32_t epoch_utc);

/**
 * @brief Explicitly resets the RTC backup domain.
 *
 * DESTRUCTIVE: clears the RTC calendar, clock-source selection and trust
 * marker. Never called during normal initialization. Use only when migrating
 * from a different RTC clock source; call RTC_Init() and set UTC again after
 * the reset. There is intentionally no UART command for this operation.
 *
 * @retval STATUS_OK The reset was performed.
 */
Status RTC_ResetBackupDomain(void);

#ifdef __cplusplus
}
#endif

#endif /* RTC_H */
