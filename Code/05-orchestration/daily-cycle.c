/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdio.h>
#include "daily-cycle.h"

#include "../02-providers/022-configurations/deployment-config.h"
#include "../03-validation/034-math-utils/math-utils.h"

#include "../04-calculation/043-vapour-pressure-calc/vapour-pressure-calc.h"
#include "../04-calculation/044-atmospheric-calc/atm-pressure-model.h"
#include "../04-calculation/047-evapotranspiration-calc/eto-calc.h"

Status RunDailyCycle(DailyResults *out, const char **out_failed_step) {
    if ((out == NULL) || (out_failed_step == NULL)) {
        return STATUS_NULL_POINTER;
    }

    *out_failed_step = "OK";

    /* *** Initialization (with formal status check) *** */
    Status status = AirTemperature_Init(&out->temperature_data);
    if (status != STATUS_OK) {
        *out_failed_step = "AirTemperature_Init";
        return status;
    }

    status = AirHumidity_Init(&out->humidity_data);
    if (status != STATUS_OK) {
        *out_failed_step = "AirHumidity_Init";
        return status;
    }

    status = AtmosphericData_Init(&out->atmos_data);
    if (status != STATUS_OK) {
        *out_failed_step = "AtmosphericData_Init";
        return status;
    }

    status = WindSpeed_Init(&out->wind_data);
    if (status != STATUS_OK) {
        *out_failed_step = "WindSpeed_Init";
        return status;
    }

    status = Location_Init(&out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "Location_Init";
        return status;
    }

    status = DayCalc_Init(&out->day_data);
    if (status != STATUS_OK) {
        *out_failed_step = "DayCalc_Init";
        return status;
    }

    status = RaCalc_Init(&out->ra_data);
    if (status != STATUS_OK) {
        *out_failed_step = "RaCalc_Init";
        return status;
    }

    status = AngstromValues_Default(&out->angstrom);
    if (status != STATUS_OK) {
        *out_failed_step = "AngstromValues_Default";
        return status;
    }

    status = SolarRadiation_Init(&out->solar_radiation);
    if (status != STATUS_OK) {
        *out_failed_step = "SolarRadiation_Init";
        return status;
    }

    status = NetRadiation_Init(&out->net_radiation);
    if (status != STATUS_OK) {
        *out_failed_step = "NetRadiation_Init";
        return status;
    }

    status = SunshineLux_Init(&out->sunshine_data, CONFIG_BRIGHT_LUX_THRESHOLD,
        CONFIG_SAMPLE_PERIOD_SEC);
    if (status != STATUS_OK) {
        *out_failed_step = "SunshineLux_Init";
        return status;
    }

    status = SunshineLux_ResetDay(&out->sunshine_data);
    if (status != STATUS_OK) {
        *out_failed_step = "SunshineLux_ResetDay";
        return status;
    }

    /* *** Measurement layer *** */

    /* Air temperature */
    status = SensorTemperature_ReadInstant(&out->t_sample);
    if (status != STATUS_OK) {
        (void)fprintf(stderr,
                      "No air temperature data, using default value. "
                      "Reason: %s\n", Status_ToString(status));

        status = SensorTemperature_ReadDefault(&out->t_sample);
        if (status != STATUS_OK) {
            *out_failed_step = "SensorTemperature_ReadDefault";
            return status;
        }
    }

    /* Air humidity */
    status = SensorHumidity_ReadInstant(&out->humidity_sample);
    if (status != STATUS_OK) {
        (void)fprintf(stderr,
                      "No air humidity data, using default value. Reason: %s\n",
                      Status_ToString(status));

        status = SensorHumidity_ReadDefault(&out->humidity_sample);
        if (status != STATUS_OK) {
            *out_failed_step = "SensorHumidity_ReadDefault";
            return status;
        }
    }

    status = AirHumidity_Update(&out->humidity_data, out->humidity_sample.RH_pct, out->humidity_sample.timestamp);
    if (status != STATUS_OK) {
        *out_failed_step = "AirHumidity_Update";
        return status;
    }

    /* Atmospheric pressure (priority sources for P) */
    status = SensorPressure_ReadInstant(&out->pressure_sample);
    if (status == STATUS_OK) {
        /* Source 1: sensor */
        out->P_source_kPa = out->pressure_sample.P_kPa;
    } else {
        /* Source 2: eq. 7 model, preferred fallback */
        (void)fprintf(stderr,
                      "Pressure sensor unavailable (%s). Using eq.7 model.\n",
                      Status_ToString(status));
        status = Calc_PressureFromElevation(out->location.elevation_m, &out->P_source_kPa);
        if (status != STATUS_OK) {
            /* Source 3: final fallback level */
            (void)fprintf(stderr,
                          "Eq. 7 model unavailable (%s). Using constant.\n",
                          Status_ToString(status));
            (void)SensorPressure_ReadDefault(&out->pressure_sample);

            out->P_source_kPa = out->pressure_sample.P_kPa;
        }
    }

    /* Wind speed */
    status = SensorWindSpeed_ReadInstant(&out->wind_sample);
    if (status != STATUS_OK) {
        (void)fprintf(stderr,
                      "No wind speed data, using default value. "
                      "Reason: %s\n", Status_ToString(status));
        status = SensorWindSpeed_ReadDefault(&out->wind_sample);
        if (status != STATUS_OK) {
            *out_failed_step = "SensorWindSpeed_ReadDefault";
            return status;
        }
    }

    status = WindSpeed_Update(&out->wind_data, out->wind_sample.speed_m_s,
        out->wind_sample.height_m, out->wind_sample.timestamp);
    if (status != STATUS_OK) {
        *out_failed_step = "WindSpeed_Update";
        return status;
    }

    /* Illuminance */
    /* At the PC version we read a sequence of mock values; on MCU the same call
     * through the same read contract will be used, but SensorLux_ReadInstant() will become driver-level */
    for (uint32_t i = 0U; i < 12U; ++i) {
        status = SensorLux_ReadInstant(&out->lux_sample);
        if (status != STATUS_OK) {
            (void)fprintf(stderr,
                          "No illuminance data, using default value. "
                          "Reason: %s\n", Status_ToString(status));

            status = SensorLux_ReadDefault(&out->lux_sample);
            if (status != STATUS_OK) {
                *out_failed_step = "SensorLux_ReadDefault";
                return status;
            }
        }

        status = SunshineLux_Update(&out->sunshine_data, out->lux_sample.lux, out->lux_sample.source);
        if (status != STATUS_OK) {
            *out_failed_step = "SunshineLux_Update";
            return status;
        }

        (void)printf("lux[%02u] = %.0f, source = %s\n",
                     (unsigned)i, out->lux_sample.lux, SensorValueSource_ToString(out->lux_sample.source));
    }

    status = SunshineLux_FinalizeDay(&out->sunshine_data);
    if (status != STATUS_OK) {
        *out_failed_step = "SunshineLux_FinalizeDay";
        return status;
    }

    /* *** Calculation layer *** */

    /* Air temperature */
    status = AirTemperature_Update(&out->temperature_data, out->t_sample.instant_c, out->t_sample.timestamp);
    if (status != STATUS_OK) {
        *out_failed_step = "AirTemperature_Update";
        return status;
    }

    /* Saturation vapour pressure */
    status = Calc_SaturationVapourPressure(out->temperature_data.T_mean_C, &out->e_tmean);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_SaturationVapourPressure";
        return status;
    }

    status = Calc_MeanSaturationVapourPressure(&out->temperature_data, &out->e_s);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_MeanSaturationVapourPressure";
        return status;
    }

    status = Calc_SlopeDelta(&out->temperature_data, &out->delta);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_SlopeDelta";
        return status;
    }

    /* Psychrometric constant from P (eq. 8) */
    status = Calc_AtmosphericParameters(&out->atmos_data, out->P_source_kPa);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_AtmosphericParameters";
        return status;
    }

    /* Actual vapour pressure ea (eq. 17) */
    status = Calc_ActualVapourPressure(&out->ea_kpa, &out->temperature_data, &out->humidity_data);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_ActualVapourPressure";
        return status;
    }

    /* Wind speed at 2 m height (eq. 47) */
    status = Calc_WindSpeedAt2m(out->wind_data.u_z_mean_m_s, out->wind_data.height_m, &out->u2);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_WindSpeedAt2m";
        return status;
    }

    /* Astronomy */
    status = DateProvider_Read(&out->date);  /* Get current day of year */
    if (status != STATUS_OK) {
        *out_failed_step = "DateProvider_Read";
        return status;
    }

    out->current_j = DayCalc_JFromDate(out->date.day, out->date.month, out->date.year);

    status = DayCalc_Update(&out->day_data, out->current_j, &out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "DayCalc_Update";
        return status;
    }

    /* Extraterrestrial radiation */
    status = Calc_Ra(&out->ra_data, &out->day_data, &out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_Ra";
        return status;
    }

    /* Solar radiation */
    status = SolarRadiation_Calc(&out->angstrom, &out->solar_radiation,
        &out->ra_data, &out->day_data, &out->sunshine_data, &out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "SolarRadiation_Calc";
        return status;
    }

    /* Net radiation */
    status = Calc_NetRadiation(&out->net_radiation, &out->temperature_data,
        &out->solar_radiation, out->ea_kpa);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_NetRadiation";
        return status;
    }

    /* Reference evapotranspiration (eq. 6, Penman-Monteith) */
    status = Calc_ETo(
        out->delta,                               /* Δ [kPa/C]                 */
        out->net_radiation.Rn_daily,              /* Rn [MJ m-2 day-1]         */
        ETO_G_DAILY_MJ_M2_DAY,                    /* G = 0 for daily (eq. 42)  */
        out->atmos_data.gamma_kPa_per_C,          /* γ [kPa/C]                 */
        out->temperature_data.T_mean_C,           /* Tmean [C]                 */
        out->u2,                                  /* u2 [m/s]                  */
        out->e_s,                                 /* es [kPa]                  */
        out->ea_kpa,                              /* ea [kPa]                  */
        &out->eto_mm_day                          /* eto [mm/day]              */
    );

    if (status != STATUS_OK) {
        *out_failed_step = "Calc_ETo";
        return status;
    }

    /* Crop evapotranspiration (eq. 56) */
    status = Calc_ETc(out->eto_mm_day, CONFIG_CROP_KC, &out->etc_mm_day);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_ETc";
        return status;
    }

    return STATUS_OK;
}

void PrintReport(const DailyResults *results) {
    /* *** Output *** */
    #define COL_W 38
    #define SEP " = "

    /* Data sources */
    (void)printf("\n=== Data sources ===\n");
    (void)printf("%-*s%s%s\n", COL_W, "Air temperature",
                 SEP, SensorValueSource_ToString(results->t_sample.source));

    (void)printf("%-*s%s%s\n", COL_W, "Illuminance (daily data)",
                 SEP, SensorValueSource_ToString(results->sunshine_data.source));

    /* Air temperature & saturation vapour pressure */
    (void)printf("\n=== Air temperature and saturation vapour pressure ===\n");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmin", SEP, results->temperature_data.T_min_C, "C");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmax", SEP, results->temperature_data.T_max_C, "C");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmean", SEP, results->temperature_data.T_mean_C, "C");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "e(Tmean)", SEP, results->e_tmean, "kPa");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "es", SEP, results->e_s, "kPa");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "delta", SEP, results->delta, "kPa/C");

    /* Atmospheric parameters */
    (void)printf("\n=== Atmospheric parameters ===\n");
    (void)printf("%-*s%s%12.2f kPa (source: %s)\n", COL_W, "P", SEP, results->atmos_data.P_kPa,
                 (results->pressure_sample.source == SENSOR_VALUE_MEASURED) ? "sensor" : "model/constant");

    (void)printf("%-*s%s%12.5f %-6s\n", COL_W, "gamma", SEP, results->atmos_data.gamma_kPa_per_C, "kPa/C");
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "RHmax", SEP, results->humidity_data.RH_max, "%");
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "RHmin", SEP, results->humidity_data.RH_min, "%");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "ea", SEP, results->ea_kpa, "kPa");

    /* Wind speed */
    (void)printf("\n=== Wind speed ===\n");
    (void)printf("%-*s%s%s\n", COL_W, "Source", SEP, SensorValueSource_ToString(results->wind_sample.source));
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "Anemometer height (z)", SEP, results->wind_data.height_m, "m");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "uzmean", SEP, results->wind_data.u_z_mean_m_s, "m/s");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "u2 (eq. 47)", SEP, results->u2, "m/s");

    /* Astronomy */
    (void)printf("\n=== Astronomy, at J = %u, phi = %.4f rad = %.2f deg ===\n",
                 results->day_data.J, results->location.latitude_rad, results->location.latitude_rad * RAD_TO_DEG);

    (void)printf("%-*s%s%12u\n", COL_W, "Current day of year (J)", SEP, results->current_j);
    (void)printf("%-*s%s%12.4f\n", COL_W, "Inverse relative distance", SEP, results->day_data.dr);

    (void)printf("%-*s%s%12.4f rad (%6.2f deg)\n", COL_W, "Solar declination",
                 SEP, results->day_data.delta_rad, results->day_data.delta_rad * RAD_TO_DEG);

    (void)printf("%-*s%s%12.4f rad\n", COL_W, "Sunset hour angle", SEP, results->day_data.omega_s_rad);
    (void)printf("%-*s%s%12.2f h\n", COL_W, "Daylight hours (N)", SEP, results->day_data.N_hours);

    /* Extraterrestrial radiation & equivalent evaporation */
    (void)printf("\n=== Extraterrestrial radiation and equivalent evaporation ===\n");
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Extraterrestrial radiation (Ra)",
                 SEP, results->ra_data.Ra_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Equivalent evaporation (from Ra_daily)",
                 SEP, results->ra_data.Ra_daily * C_RAD, "mm/day");

    /* Solar & clear-sky radiation */
    (void)printf("\n=== Solar and clear-sky radiation ===\n");
    (void)printf("%-*s%s%12.2f\n", COL_W, "Angstrom a_s", SEP, results->angstrom.a_s);
    (void)printf("%-*s%s%12.2f\n", COL_W, "Angstrom b_s", SEP, results->angstrom.b_s);

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Solar radiation (Rs)",
                 SEP, results->solar_radiation.Rs_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Clear-sky radiation (Rso)",
                 SEP, results->solar_radiation.Rso_daily, "MJ m-2 day-1");

    /* Net radiation */
    (void)printf("\n=== Net radiation ===\n");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "ea (actual vapour pressure)", SEP, results->ea_kpa, "kPa");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net shortwave radiation (Rns)",
                 SEP, results->net_radiation.Rns_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net longwave radiation (Rnl)",
                 SEP, results->net_radiation.Rnl_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net radiation (Rn)",
                 SEP, results->net_radiation.Rn_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Equivalent evaporation (from Rn_daily)",
                 SEP, results->net_radiation.Rn_daily * C_RAD, "mm/day");

    /* Sunshine duration */
    (void)printf("\n=== Sunshine duration ===\n");
    (void)printf("%-*s%s%12.0f %-6s\n", COL_W, "Binarization threshold",
                 SEP, CONFIG_BRIGHT_LUX_THRESHOLD, "lux");

    (void)printf("%-*s%s%12u %-6s\n", COL_W, "Sampling interval",
                 SEP, (unsigned)CONFIG_SAMPLE_PERIOD_SEC, "s");

    (void)printf("%-*s%s%12u\n", COL_W, "Total samples", SEP, results->sunshine_data.total_samples);
    (void)printf("%-*s%s%12u\n", COL_W, "Bright samples", SEP, results->sunshine_data.bright_samples);
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Sunshine duration (n)", SEP, results->sunshine_data.n_hours, "h");

    /* Evapotranspiration */
    (void)printf("\n=== Evapotranspiration ===\n");
    (void)printf("%-*s%s%12.3f %-6s\n", COL_W, "ETo (eq. 6, Penman-Monteith)", SEP, results->eto_mm_day, "mm/day");
    (void)printf("%-*s%s%12.2f\n", COL_W, "Kc (crop coefficient)", SEP, CONFIG_CROP_KC);
    (void)printf("%-*s%s%12.3f %-6s\n", COL_W, "ETc (eq. 56, Kc * ETo)", SEP, results->etc_mm_day, "mm/day");

    #undef COL_W
    #undef SEP
}
