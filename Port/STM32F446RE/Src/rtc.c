/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

/* *** STM32F446RE RTC driver: LSE 32.768 kHz & CMSIS register access; see stm32f446xx.h *** */

#include <stdbool.h>
#include <stddef.h>

#include "rtc.h"
#include "stm32f4xx.h"
#include "system_stm32f4xx.h"

#include "02-providers/023-time/calendar-time.h"
#include "02-providers/023-time/time-provider.h"

/* RTC clock: 32768 Hz / ((PREDIV_A + 1) * (PREDIV_S + 1)) = 1 Hz **** *** ** **** *** ***** */
#define PREDIV_A            127U
#define PREDIV_S            255U

#define PRER_FIELDS_MASK    (RTC_PRER_PREDIV_A | RTC_PRER_PREDIV_S)
#define PRER_EXPECTED       ((PREDIV_A << RTC_PRER_PREDIV_A_Pos) | PREDIV_S)

/* RTC_WPR write-protection keys ****** *** * * *** * * ******** * ***** * * ***** * **** ** */
#define WPR_KEY_1 0xCAU
#define WPR_KEY_2 0x53U

/* BKP0R is the commit marker; BKP1R stores the timestamp last set by the operator;
 * a timestamp earlier than that value is treated as a calendar rollback ***** **** ** *** * */
#define TRUST_MARKER 0x52544332U

/* Timeouts, ms. Each wait ends: the flag arrives or the timeout expires *** * * ****** * * * * ***** * * * ** */
#define LSE_STARTUP_TIMEOUT_MS 5000U  /* Crystal start-up takes up to seconds (datasheet, LSE characteristics) */
#define INIT_TIMEOUT_MS        1000U  /* INITF: expected after about 2 RTCCLK cycles (61 us) **** *** ***** ** */
#define SYNC_TIMEOUT_MS        1000U  /* RSF: expected after about 2 RTCCLK cycles (61 us) *** *** * ***** *** */
#define LIVENESS_TIMEOUT_MS    25U    /* SSR changes every 1/256 s = 3.9 ms: allow more than 6 periods *** *** */
#define MS_PER_SECOND          1000U

/* Allow the seconds counter to advance during read-back verification **** **** * * * *** ** */
#define READBACK_TOLERANCE_S  2U
#define CALENDAR_READ_RETRIES 3U

/* Widths of the BCD fields as masks (applied after the shift) *** * * * ***** * * * **** ** */
#define MASK_1_BIT  0x1U
#define MASK_2_BITS 0x3U
#define MASK_3_BITS 0x7U
#define MASK_4_BITS 0xFU

#define BCD_RADIX      10U
#define BCD_MAX_DIGIT  9U
#define RTC_YEAR_BASE  2000U /* RTC_DR holds a two-digit year, 00 = 2000 *** * * * ***** **  */

#define SECONDS_PER_DAY      86400U
#define DAYS_PER_WEEK        7U
#define EPOCH_WEEKDAY_SHIFT  3U    /* 1970-01-01 was a Thursday: (0 + 3) % 7 + 1 = 4; RTC: 1 = Mon ... 7 = Sun */

/* Backup-domain control register (RCC_BDCR) *** * * * ****** *** **** * ** * * ***** *** ****** * * * ***** * */
#define BDCR_CLOCK_MASK     (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL   | RCC_BDCR_LSEON | RCC_BDCR_LSERDY)
#define BDCR_CLOCK_EXPECTED (RCC_BDCR_RTCEN | RCC_BDCR_RTCSEL_0 | RCC_BDCR_LSEON | RCC_BDCR_LSERDY)

/* True once RTC_Init() has confirmed that the clock runs; cleared by anything that may stop it *** * * * **** */
static bool s_clock_alive = false;

/** @internal @brief Starts the DWT cycle counter used for all timeouts (idempotent). */
static void EnableCycleCounter(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/** @internal @brief True if more than timeout_ms elapsed since start (cycle counter value).
 *  Wrap-around safe for timeouts shorter than 2^32 cycles. */
static bool TimerExpired(const uint32_t start, const uint32_t timeout_ms) {
    const uint32_t cycles_per_ms = SystemCoreClock / MS_PER_SECOND;

    if (cycles_per_ms == 0U) {
        return true;
    }

    const uint32_t timeout_cycles = timeout_ms * cycles_per_ms;

    return (uint32_t)(DWT->CYCCNT - start) >= timeout_cycles;
}

/** @internal @brief Waits until (*reg & mask) is non-zero (state == true) or zero (state == false).
 *  @retval true  The condition was met.
 *  @retval false The timeout expired first. */
static bool WaitForFlag(const volatile uint32_t *reg, const uint32_t mask,
    const bool state, const uint32_t timeout_ms) {
    const uint32_t start = DWT->CYCCNT;

    do {
        if (((*reg & mask) != 0U) == state) {
            return true;
        }
    } while (!TimerExpired(start, timeout_ms));

    return ((*reg & mask) != 0U) == state;
}

/** @internal @brief Opens write access to the backup domain (PWR_CR.DBP). */
static void BackupAccessEnable(void) {
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR; /* Read-back: the clock needs a moment to reach PWR */
    PWR->CR |= PWR_CR_DBP;
}

/** @internal @brief Closes write access to the backup domain again. */
static void BackupAccessDisable(void) {
    PWR->CR &= ~PWR_CR_DBP;
}

/** @internal @brief Clears the trust marker without changing the calendar. */
static void InvalidateTrustMarker(void) {
    BackupAccessEnable();
    RTC->BKP0R = 0U;
    BackupAccessDisable();
}

/** @internal @brief Removes the RTC register write protection (RTC_WPR key sequence). */
static void RtcWriteEnable(void) {
    RTC->WPR = WPR_KEY_1;
    RTC->WPR = WPR_KEY_2;
}

/** @internal @brief Restores the RTC register write protection. */
static void RtcWriteDisable(void) {
    RTC->WPR = RTC_WPR_KEY;
}

/** @internal @brief Clears RSF and waits until the shadow registers are refreshed from the calendar.
 *  Write access to the RTC must be enabled by the caller.
 *  @retval true  Synchronized.
 *  @retval false RSF did not come back (for example, the clock is stopped). */
static bool SynchronizeShadowRegisters(void) {
    RTC->ISR &= ~RTC_ISR_RSF;

    return WaitForFlag(&RTC->ISR, RTC_ISR_RSF, true, SYNC_TIMEOUT_MS);
}

/** @internal @brief Checks that the sub-second counter (RTC_SSR) changes, i.e. LSE and RTC really tick.
 *  Needs the shadow registers synchronized. A stopped crystal is not visible
 *  in RCC_BDCR (LSERDY stays set), so this is the only test of "alive". */
static bool ClockIsTicking(void) {
    /* With BYPSHAD = 0, reading SSR locks the shadow registers until DR is read;
     * read DR after every sample; otherwise repeated SSR reads can return the
     * same snapshot and falsely report a stopped RTC */
    uint32_t previous = RTC->SSR;
    (void)RTC->DR;

    const uint32_t start = DWT->CYCCNT;

    do {
        const uint32_t current = RTC->SSR;
        (void)RTC->DR; /* Release the shadow-register lock for the next sample */

        if (current != previous) {
            return true;
        }

        previous = current;
    } while (!TimerExpired(start, LIVENESS_TIMEOUT_MS));

    return false;
}

/** @internal @brief Packs a value 0 ... 99 into BCD fields at the given bit positions. */
static uint32_t EncodeBcd(const uint32_t value, const uint32_t tens_shift, const uint32_t units_shift) {
    return ((value / BCD_RADIX) << tens_shift) | ((value % BCD_RADIX) << units_shift);
}

/** @internal @brief Unpacks two BCD digits into a value.
 *  @retval true  Both digits are 0 ... 9, *out is valid.
 *  @retval false A digit is not a decimal digit (register content is garbage). */
static bool DecodeBcd(const uint32_t tens, const uint32_t units, uint32_t *out) {
    if ((tens > BCD_MAX_DIGIT) || (units > BCD_MAX_DIGIT)) {
        return false;
    }

    *out = (tens * BCD_RADIX) + units;

    return true;
}

/** @internal @brief Extracts one register field. */
static uint32_t Field(const uint32_t reg, const uint32_t shift, const uint32_t mask) {
    return (reg >> shift) & mask;
}

/** @internal @brief Reads a consistent calendar snapshot and converts it to UTC seconds.
 *  STM32F446xC/xE Errata sheet 0298 Rev 8, section 2.10.2: with BYPSHAD = 0, verify SSR
 *  did not change across the SSR/TR/DR read sequence. Retry if the shadow registers
 *  were refreshed during the read. The caller keeps BYPSHAD cleared.
 *  @retval true  *out_epoch is valid.
 *  @retval false The snapshot stayed inconsistent or contains an invalid date/time. */
static bool ReadCalendar(uint32_t *out_epoch) {
    for (uint32_t attempt = 0U; attempt < CALENDAR_READ_RETRIES; ++attempt) {
        const uint32_t ssr_before = RTC->SSR;
        const uint32_t tr = RTC->TR;
        const uint32_t dr = RTC->DR;
        const uint32_t ssr_after = RTC->SSR;
        (void)RTC->DR; /* Release the shadow-register lock */

        if (ssr_before != ssr_after) {
            continue;
        }

        if ((tr & RTC_TR_PM) != 0U) {
            return false; /* 12-hour format: not ours */
        }

        uint32_t year   = 0U;
        uint32_t month  = 0U;
        uint32_t day    = 0U;
        uint32_t hour   = 0U;
        uint32_t minute = 0U;
        uint32_t second = 0U;

        if (!DecodeBcd(Field(dr, RTC_DR_YT_Pos,  MASK_4_BITS),  Field(dr, RTC_DR_YU_Pos,  MASK_4_BITS), &year)   ||
            !DecodeBcd(Field(dr, RTC_DR_MT_Pos,  MASK_1_BIT),   Field(dr, RTC_DR_MU_Pos,  MASK_4_BITS), &month)  ||
            !DecodeBcd(Field(dr, RTC_DR_DT_Pos,  MASK_2_BITS),  Field(dr, RTC_DR_DU_Pos,  MASK_4_BITS), &day)    ||
            !DecodeBcd(Field(tr, RTC_TR_HT_Pos,  MASK_2_BITS),  Field(tr, RTC_TR_HU_Pos,  MASK_4_BITS), &hour)   ||
            !DecodeBcd(Field(tr, RTC_TR_MNT_Pos, MASK_3_BITS),  Field(tr, RTC_TR_MNU_Pos, MASK_4_BITS), &minute) ||
            !DecodeBcd(Field(tr, RTC_TR_ST_Pos,  MASK_3_BITS),  Field(tr, RTC_TR_SU_Pos,  MASK_4_BITS), &second)) {
            return false;
        }

        CalendarTime t;
        t.year   = (uint16_t)(RTC_YEAR_BASE + year);
        t.month  = (uint8_t)month;
        t.day    = (uint8_t)day;
        t.hour   = (uint8_t)hour;
        t.minute = (uint8_t)minute;
        t.second = (uint8_t)second;

        return CalendarTime_ToEpoch(&t, out_epoch) == STATUS_OK;
    }

    return false;
}

/** @internal @brief Evaluates clock, calendar and trust from the registers (no waiting).
 *  @param[out] out_epoch Receives the time in the state RTC_STATE_TRUSTED; may be NULL. */
static RTCState EvaluateState(uint32_t *out_epoch) {
    /* 1. Clock: RTC clocked from LSE and enabled, LSE on and ready */
    if ((RCC->BDCR & BDCR_CLOCK_MASK) != BDCR_CLOCK_EXPECTED) {
        return RTC_STATE_CLOCK_FAULT;
    }

    /* 2. Calendar: year field != 0 since the last backup-domain reset */
    if ((RTC->ISR & RTC_ISR_INITS) == 0U) {
        return RTC_STATE_NOT_SET;
    }

    /* 3. Trust: set by the procedure, configuration is the one we wrote, value is real and plausible */
    if (((RTC->PRER & PRER_FIELDS_MASK) != PRER_EXPECTED) ||
        ((RTC->CR & (RTC_CR_FMT | RTC_CR_BYPSHAD)) != 0U) ||
        (RTC->BKP0R != TRUST_MARKER)) {
        return RTC_STATE_UNCONFIRMED;
    }

    uint32_t epoch = 0U;

    if (!ReadCalendar(&epoch) ||
        (epoch < TIME_PROVIDER_MIN_EPOCH) || (epoch > TIME_PROVIDER_MAX_EPOCH) ||
        (RTC->BKP1R < TIME_PROVIDER_MIN_EPOCH) ||
        (RTC->BKP1R > TIME_PROVIDER_MAX_EPOCH) ||
        (epoch < RTC->BKP1R)) {
        return RTC_STATE_UNCONFIRMED;
    }

    if (out_epoch != NULL) {
        *out_epoch = epoch;
    }

    return RTC_STATE_TRUSTED;
}

Status RTC_Init(void) {
    EnableCycleCounter();
    s_clock_alive = false;

    BackupAccessEnable();

    /* Settings that only a backup-domain reset can undo: do not touch, report */
    const uint32_t clock_source = RCC->BDCR & RCC_BDCR_RTCSEL;

    if (((clock_source != 0U) && (clock_source != RCC_BDCR_RTCSEL_0)) ||
        ((RCC->BDCR & RCC_BDCR_LSEBYP) != 0U)) {
        RTC->BKP0R = 0U;
        BackupAccessDisable();
        return STATUS_INTERNAL_ERROR;
    }

    /* Each step only if not done yet: after a reset with a running RTC nothing is written */
    RCC->BDCR |= RCC_BDCR_LSEON;

    if (!WaitForFlag(&RCC->BDCR, RCC_BDCR_LSERDY, true, LSE_STARTUP_TIMEOUT_MS)) {
        RTC->BKP0R = 0U;
        BackupAccessDisable();
        return STATUS_UNAVAILABLE;
    }

    if (clock_source == 0U) {
        RCC->BDCR |= RCC_BDCR_RTCSEL_0; /* Selectable once, until the next backup-domain reset */
    }

    RCC->BDCR |= RCC_BDCR_RTCEN;

    RtcWriteEnable();

    /* Recover from a reset that interrupted a calendar write in INIT mode */
    if ((RTC->ISR &   RTC_ISR_INIT) != 0U) {
        RTC->ISR  &= ~RTC_ISR_INIT;
    }

    if ((RTC->CR &   RTC_CR_BYPSHAD) != 0U) {
        RTC->CR  &= ~RTC_CR_BYPSHAD; /* Read through the shadow registers; the calendar is untouched */
    }

    const bool synchronized = SynchronizeShadowRegisters();

    RtcWriteDisable();
    BackupAccessDisable();

    if (!synchronized) {
        InvalidateTrustMarker();
        return STATUS_UNAVAILABLE;
    }

    if (!ClockIsTicking()) {
        InvalidateTrustMarker();
        return STATUS_UNAVAILABLE;
    }

    s_clock_alive = true;

    return STATUS_OK;
}

RTCState RTC_GetState(void) {
    if (!s_clock_alive) {
        return RTC_STATE_CLOCK_FAULT;
    }

    /* Fail closed if the crystal stops after initialization */
    if (!ClockIsTicking()) {
        s_clock_alive = false;
        InvalidateTrustMarker();
        return RTC_STATE_CLOCK_FAULT;
    }

    const RTCState state = EvaluateState(NULL);

    /* Once trust has been disproved, do not allow it to recover merely because
     * a rolled-back calendar later catches up with the old set timestamp */
    if (state != RTC_STATE_TRUSTED) {
        InvalidateTrustMarker();
    }

    return state;
}

const char *RTCState_ToString(const RTCState state) {
    switch (state) {
        case RTC_STATE_CLOCK_FAULT: return "RTC_STATE_CLOCK_FAULT";
        case RTC_STATE_NOT_SET:     return "RTC_STATE_NOT_SET";
        case RTC_STATE_UNCONFIRMED: return "RTC_STATE_UNCONFIRMED";
        case RTC_STATE_TRUSTED:     return "RTC_STATE_TRUSTED";
        default:                    return "RTC_STATE_UNKNOWN";
    }
}

Status RTC_ReadEpoch(uint32_t *out_epoch) {
    if (out_epoch == NULL) {
        return STATUS_NULL_POINTER;
    }

    uint32_t epoch = 0U;

    if (!s_clock_alive) {
        return STATUS_UNAVAILABLE;
    }

    /* Never return a timestamp from a calendar that has stopped ticking */
    if (!ClockIsTicking()) {
        s_clock_alive = false;
        InvalidateTrustMarker();
        return STATUS_UNAVAILABLE;
    }

    if (EvaluateState(&epoch) != RTC_STATE_TRUSTED) {
        InvalidateTrustMarker();
        return STATUS_UNAVAILABLE;
    }

    *out_epoch = epoch;

    return STATUS_OK;
}

/** @internal @brief Writes prescalers, format and calendar in initialization mode.
 *  Write access to the RTC must be enabled by the caller. Initialization mode
 *  is always left again: with INIT set the calendar does not count.
 *  @retval STATUS_OK          Written; the calendar restarts counting.
 *  @retval STATUS_UNAVAILABLE INITF did not appear. */
static Status WriteCalendar(const uint32_t tr, const uint32_t dr) {
    Status status = STATUS_OK;

    RTC->ISR |= RTC_ISR_INIT;

    if (!WaitForFlag(&RTC->ISR, RTC_ISR_INITF, true, INIT_TIMEOUT_MS)) {
        status = STATUS_UNAVAILABLE;
    } else {
        RTC->PRER =  PREDIV_S;  /* Two separate write accesses, S then A (RM0390) */
        RTC->PRER =  PREDIV_S | (PREDIV_A << RTC_PRER_PREDIV_A_Pos);
        RTC->CR  &= ~(RTC_CR_FMT | RTC_CR_BYPSHAD);
        RTC->TR   =  tr;
        RTC->DR   =  dr;
    }

    RTC->ISR &= ~RTC_ISR_INIT;

    return status;
}

/** @internal @brief The calendar-writing part of RTC_SetEpoch(), with write access already enabled */
static Status SetCalendarUnlocked(const uint32_t epoch_utc, const uint32_t tr, const uint32_t dr) {
    RTC->BKP0R = 0U; /* From now on the time is untrusted, until proven otherwise */

    Status status = WriteCalendar(tr, dr);

    if ((status == STATUS_OK) && !SynchronizeShadowRegisters()) {
        status   = STATUS_UNAVAILABLE;
    }

    if (status == STATUS_OK) {
        RTC->BKP1R = epoch_utc;
        RTC->BKP0R = TRUST_MARKER; /* Write the commit marker last */

        uint32_t readback = 0U;

        if ((EvaluateState(&readback) != RTC_STATE_TRUSTED) ||
            (readback < epoch_utc) || ((readback - epoch_utc) > READBACK_TOLERANCE_S)) {
            RTC->BKP0R = 0U;
            status = STATUS_INTERNAL_ERROR;
        }
    }

    return status;
}

Status RTC_SetEpoch(const uint32_t epoch_utc) {
    if ((epoch_utc < TIME_PROVIDER_MIN_EPOCH) || (epoch_utc > TIME_PROVIDER_MAX_EPOCH)) {
        return STATUS_INVALID_VALUE;
    }

    if (!s_clock_alive) {
        return STATUS_UNAVAILABLE;
    }

    CalendarTime t;
    const Status conversion = CalendarTime_FromEpoch(epoch_utc, 0, &t);

    if (conversion != STATUS_OK) {
        return conversion;
    }

    const uint32_t weekday = (((epoch_utc / SECONDS_PER_DAY) + EPOCH_WEEKDAY_SHIFT) % DAYS_PER_WEEK) + 1U;

    const uint32_t tr = EncodeBcd(t.hour, RTC_TR_HT_Pos, RTC_TR_HU_Pos)
        | EncodeBcd(t.minute, RTC_TR_MNT_Pos, RTC_TR_MNU_Pos)
        | EncodeBcd(t.second, RTC_TR_ST_Pos,  RTC_TR_SU_Pos);

    const uint32_t dr = EncodeBcd((uint32_t)t.year - RTC_YEAR_BASE, RTC_DR_YT_Pos, RTC_DR_YU_Pos)
        | EncodeBcd(t.month, RTC_DR_MT_Pos, RTC_DR_MU_Pos)
        | EncodeBcd(t.day,   RTC_DR_DT_Pos, RTC_DR_DU_Pos)
        | (weekday << RTC_DR_WDU_Pos);

    BackupAccessEnable();
    RtcWriteEnable();

    const Status status = SetCalendarUnlocked(epoch_utc, tr, dr);

    RtcWriteDisable();
    BackupAccessDisable();

    return status;
}

Status RTC_ResetBackupDomain(void) {
    BackupAccessEnable();

    RCC->BDCR |=  RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    BackupAccessDisable();
    s_clock_alive = false;

    return STATUS_OK;
}
