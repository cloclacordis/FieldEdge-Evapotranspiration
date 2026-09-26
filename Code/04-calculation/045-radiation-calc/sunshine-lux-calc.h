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
 * - if no samples at all, FinalizeDay() returns STATUS_INVALID_VALUE *** * * ** **** *** ** ** */
typedef struct {
    double            threshold_lux;        /* Binarization threshold [lux] *** * ** **** *** * */
    uint32_t          sample_period_sec;    /* Polling interval [sec] ** * * * ** ***** * * *** */
    uint32_t          bright_samples;       /* Counter of bright samples ***** * * * *** * **** */
    uint32_t          total_samples;        /* Counter of all samples (for inspection) ** ** ** */
    double            n_hours;              /* Computed n [h] after finalization ** * * *** *** */
    bool              has_any_samples;      /* Whether at least one sample was received ** * ** */
    bool              has_default_samples;  /* Whether at least one DEFAULT sample was received */
    SensorValueSource source;               /* Data quality for the day * ** *** ***** ** ** ** */
    bool              initialized;          /* Structure initialized ***** * * **** * ** * ** * */
} SunshineLuxData;

/**
 * @brief Initializes the sunshine-duration accumulator for a new
 *        deployment (calibration parameters + first day).
 *
 * @param[out] data              Structure to initialize. Must not
 *                               be NULL.
 * @param[in]  threshold_lux     Illuminance threshold above which a
 *                               sample counts as "bright". Must be > 0.
 * @param[in]  sample_period_sec Sampling interval [s]. Must be > 0.
 *
 * @post On success, `.threshold_lux` and `.sample_period_sec` are set
 *       and daily counters are zeroed, same as after
 *       SunshineLux_ResetDay().
 *
 * @retval STATUS_OK            Initialization succeeded.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE threshold_lux <= 0 or sample_period_sec == 0.
 *
 * @note   On the illuminance threshold, see illuminance-proxy.md.
 */
Status SunshineLux_Init(SunshineLuxData* data, double threshold_lux, uint32_t sample_period_sec);

/**
 * @brief Folds one instantaneous illuminance sample into the day's
 *        bright/total counters.
 *
 * Increments `.total_samples`, and `.bright_samples` if
 * `lux >= threshold_lux`. If @p source is SENSOR_VALUE_DEFAULT for
 * even one sample in the day, the whole day's final `.source`
 * (set by SunshineLux_FinalizeDay()) becomes SENSOR_VALUE_DEFAULT -
 * a single fallback sample taints the day as a diagnostic signal,
 * even though the numeric result still uses that sample's value.
 *
 * @param[in,out] data   Accumulator to update; must already be
 *                       initialized. Must not be NULL.
 * @param[in]     lux    Instantaneous illuminance [lux].
 * @param[in]     source Must be SENSOR_VALUE_MEASURED or
 *                       SENSOR_VALUE_DEFAULT.
 *
 * @retval STATUS_OK            Sample accepted.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE data not initialized, or source was
 *                              neither SENSOR_VALUE_MEASURED nor
 *                              SENSOR_VALUE_DEFAULT.
 */
Status SunshineLux_Update(SunshineLuxData* data, double lux, SensorValueSource source);

/**
 * @brief Converts the day's accumulated bright-sample count to
 *        sunshine duration in hours, and finalizes the day's source tag.
 *
 * n = bright_samples * sample_period_sec / 3600. Must be called once,
 * after the day's SunshineLux_Update() calls and before
 * SolarRadiation_Calc() reads `.n_hours`.
 *
 * @param[in,out] data Accumulator to finalize; must already be
 *                     initialized and have received at least one
 *                     Update(). Must not be NULL.
 *
 * @post On success, `.n_hours` holds the result and `.source` is
 *       SENSOR_VALUE_DEFAULT if any sample in the day was a fallback,
 *       else SENSOR_VALUE_MEASURED.
 *
 * @retval STATUS_OK            *data is finalized.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE data not initialized, no samples were
 *                              received this day, or the computed n
 *                              exceeded 24 hours (a sampling
 *                              configuration error) - in the latter
 *                              two cases `.n_hours` is left unchanged,
 *                              not reset to 0.
 */
Status SunshineLux_FinalizeDay(SunshineLuxData* data);

/**
 * @brief Resets the day's bright/total counters for a new day.
 *
 * Calibration parameters (`.threshold_lux`, `.sample_period_sec`) are
 * preserved. On the MCU (v0.2.x), called from an RTC midnight interrupt.
 *
 * @param[in,out] data Accumulator to reset; must already be
 *                     initialized. Must not be NULL.
 *
 * @retval STATUS_OK            Daily counters reset.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE data not initialized.
 */
Status SunshineLux_ResetDay(SunshineLuxData* data);

#ifdef __cplusplus
}
#endif

#endif /* SUNSHINE_LUX_CALC_H */
