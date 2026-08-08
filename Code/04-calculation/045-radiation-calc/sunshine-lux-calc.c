/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stddef.h>
#include "sunshine-lux-calc.h"

/* Helper function; binary decision: sample is bright or not;
 * threshold comparison logic can be modified in one place (e.g., for calibration or adding hysteresis) */
static bool SunshineLux_IsBright(const SunshineLuxData* data, const double lux) {
    return lux >= data->threshold_lux;
}

Status SunshineLux_Init(SunshineLuxData* data, const double threshold_lux, const uint32_t sample_period_sec) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if ((threshold_lux <= 0.0) || (sample_period_sec == 0U)) {
        return STATUS_INVALID_VALUE;
    }

    data->threshold_lux       = threshold_lux;
    data->sample_period_sec   = sample_period_sec;
    data->bright_samples      = 0U;
    data->total_samples       = 0U;
    data->n_hours             = 0.0;
    data->has_any_samples     = false;
    data->has_default_samples = false;
    data->source              = SENSOR_VALUE_DEFAULT;
    data->initialized         = true;

    return STATUS_OK;
}

Status SunshineLux_Update(SunshineLuxData* data, const double lux, const SensorValueSource source) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!data->initialized) {
        return STATUS_INVALID_VALUE;
    }

    if ((source != SENSOR_VALUE_MEASURED) && (source != SENSOR_VALUE_DEFAULT)) {
        return STATUS_INVALID_VALUE;
    }

    data->total_samples++;
    data->has_any_samples = true;

    if (SunshineLux_IsBright(data, lux)) {
        data->bright_samples++;
    }

    /* If at least one sample for the day was DEFAULT, the day's result is stored as DEFAULT until next ResetDay();
     * used as a note: "Sensor had failures, daily data may be incomplete" */
    if (source == SENSOR_VALUE_DEFAULT) {
        data->has_default_samples = true;
    }

    return STATUS_OK;
}

Status SunshineLux_FinalizeDay(SunshineLuxData* data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!data->initialized) {
        return STATUS_INVALID_VALUE;
    }

    /* Nothing to finalize without at least one sample */
    if (!data->has_any_samples) {
        return STATUS_INVALID_VALUE;
    }

    /* Convert accumulated bright samples to sunshine duration in hours;
     * physical constraint: n <= 24 h; if bright_samples * sample_period_sec > 86400 sec,
     * there is a configuration error: period too long or counter not reset at midnight *** * */
    const double n = ((double)data->bright_samples * (double)data->sample_period_sec) / 3600.0;

    /* Protective check: physically n should not exceed 24 hours */
    if (n > 24.0) {
        return STATUS_INVALID_VALUE; /* Don't overwrite n_hours, leave as 0.0 from Init/Reset */
    }

    data->n_hours = n;
    data->source = data->has_default_samples ? SENSOR_VALUE_DEFAULT : SENSOR_VALUE_MEASURED;

    return STATUS_OK;
}

Status SunshineLux_ResetDay(SunshineLuxData* data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!data->initialized) {
        return STATUS_INVALID_VALUE;
    }

    data->bright_samples       = 0U;
    data->total_samples        = 0U;
    data->n_hours              = 0.0;
    data->has_any_samples      = false;
    data->has_default_samples  = false;
    data->source               = SENSOR_VALUE_DEFAULT;

    return STATUS_OK;
}
