/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <string.h>
#include "wind-speed-calc.h"

/* Constants per Prandtl logarithmic profile (eq. 47) * * * * * ****** * * * *** * */
#define C_EQ47_NUM          (4.87)
#define C_EQ47_MULT         (67.8)
#define C_EQ47_SUB          (5.42)

/* Valid argument ranges */
#define WIND_SPEED_MIN_MS   (0.0)      /* m/s, calm/Stille, 0.0 physically valid   */
#define WIND_SPEED_MAX_MS   (100.0)    /* m/s, practical upper limit *** * * * *** */
#define WIND_HEIGHT_MIN_M   (0.1)      /* m,   lower bound, eq. 47 protection ** * */
#define WIND_HEIGHT_MAX_M   (200.0)    /* m,   upper limit for meteostation height */

/* Tolerance for height comparison between consecutive Update() calls **** * * *** */
#define WIND_HEIGHT_TOL_M   (0.001)    /* m *** * * **** * * * * ****** * * * * ** */

/* *** Internal validators *** */

/* isfinite() filters out NaN and +-Inf before range comparison */
static bool IsValidSpeed(const double u) {
    return isfinite(u) && (u >= WIND_SPEED_MIN_MS) && (u <= WIND_SPEED_MAX_MS);
}

static bool IsValidHeight(const double z) {
    return isfinite(z) && (z >= WIND_HEIGHT_MIN_M) && (z <= WIND_HEIGHT_MAX_M);
}

/* *** Function implementations *** */

Status WindSpeed_Init(WindSpeedData *data) {
    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    memset(data, 0, sizeof(*data));

    return STATUS_OK;
}

Status WindSpeed_Update(WindSpeedData *data, const double speed_m_s, const double height_m, const uint32_t timestamp) {
    (void)timestamp; /* TODO: reserved (time-ordered sorting) */

    if (data == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!IsValidSpeed(speed_m_s) || !IsValidHeight(height_m)) {
        return STATUS_INVALID_VALUE;
    }

    if (data->initialized) {
        /* From the 2nd call onward, anemometer height must be constant */
        double diff = height_m - data->height_m;
        if ((diff > WIND_HEIGHT_TOL_M) || (diff < -WIND_HEIGHT_TOL_M)) {
            return STATUS_INVALID_VALUE;
        }
    } else {
        /* First valid sample: fix height, initialize min/max */
        data->height_m    = height_m;
        data->u_z_min_m_s = speed_m_s;
        data->u_z_max_m_s = speed_m_s;
        data->initialized = true;
    }

    /* Update min */
    if (speed_m_s < data->u_z_min_m_s) {
        data->u_z_min_m_s = speed_m_s;
    }

    /* Update max */
    if (speed_m_s > data->u_z_max_m_s) {
        data->u_z_max_m_s = speed_m_s;
    }

    /* Accumulation for arithmetic mean of daily series */
    data->u_sum_m_s += speed_m_s;
    data->sample_count++;
    data->u_z_mean_m_s = data->u_sum_m_s / (double)data->sample_count;

    return STATUS_OK;
}

Status Calc_WindSpeedAt2m(const double u_z, const double z, double *out_u2) {
    if (out_u2 == NULL) {
        return STATUS_NULL_POINTER;
    }

    if (!IsValidSpeed(u_z) || !IsValidHeight(z)) {
        return STATUS_INVALID_VALUE;
    }

    double log_arg = C_EQ47_MULT * z - C_EQ47_SUB;
    *out_u2 = u_z * (C_EQ47_NUM / log(log_arg)); /* Eq. 47 */

    return STATUS_OK;
}
