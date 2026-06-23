/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef SUNSHINE_LUX_CALC_H
#define SUNSHINE_LUX_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "../../03-validation/033-status/status.h"
#include "../../03-validation/031-value-source/value-source.h"

/* Structure for daily accumulator; accumulator logic:
 * - receives instant values via SunshineLux_Update();
 * - stores accumulated state over the day;
 * - finalizes with SunshineLux_FinalizeDay() at end of day;
 * - resets with SunshineLux_ResetDay() at start of new day.
 *
 * Data quality for the day is determined based on the entire series:
 * - if all samples are MEASURED, then source = SENSOR_VALUE_MEASURED;
 * - if at least one sample is DEFAULT, then source = SENSOR_VALUE_DEFAULT;
 * - if no samples at all, FinalizeDay() returns STATUS_INVALID_VALUE *** * * ** **** **** ** ** */
typedef struct {
    double            threshold_lux;        /* Binarization threshold [lux] **** * ** **** *** * */
    uint32_t          sample_period_sec;    /* Polling interval [sec] ** * * * ** ****** * * *** */
    uint32_t          bright_samples;       /* Counter of bright samples ***** * * * **** * **** */
    uint32_t          total_samples;        /* Counter of all samples (for inspection) ** *** ** */
    double            n_hours;              /* Computed n [h] after finalization ** * * *** **** */
    bool              has_any_samples;      /* Whether at least one sample was received *** * ** */
    bool              has_default_samples;  /* Whether at least one DEFAULT sample was received  */
    SensorValueSource source;               /* Data quality for the day * ** *** ***** ** *** ** */
    bool              initialized;          /* Structure initialized ***** * * ***** * ** * ** * */
} SunshineLuxData;

/* Init the computation module structure: threshold_lux > 0 & sample_period_sec > 0 are required */
Status SunshineLux_Init(SunshineLuxData* data, double threshold_lux, uint32_t sample_period_sec);

/* Accept a single instant sensor reading:
 * if lux >= threshold_lux - increments bright_samples counter;
 * if at least 1 sample source is SENSOR_VALUE_DEFAULT - data->source -> SENSOR_VALUE_DEFAULT;
 * called every sample_period_sec seconds (on MCU - from timer/ISR) *** **** * **** * * * **** * */
Status SunshineLux_Update(SunshineLuxData* data, double lux, SensorValueSource source);

/* Compute n_hours from accumulated samples: n = (bright_samples * sample_period_sec) / 3600;
 * called once at end of day before passing n to Calc_Rs;
 * final source for the day is computed here *** * **** * * * ** *** * **** * * * *** * * **** * */
Status SunshineLux_FinalizeDay(SunshineLuxData* data);

/* Reset daily counters for a new day; calibration parameters (threshold, period) are preserved;
 * called at midnight (on MCU - from RTC interrupt) * *** * **** * * * *** * * **** * *** * **** */
Status SunshineLux_ResetDay(SunshineLuxData* data);

#ifdef __cplusplus
}
#endif

#endif /* SUNSHINE_LUX_CALC_H */
