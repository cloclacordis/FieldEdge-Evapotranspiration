/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <stdio.h>
#include "daily-cycle.h"

#include "../02-providers/022-configurations/deployment-config.h"
#include "../03-validation/034-math-utils/math-utils.h"

#include "../04-calculation/043-vapour-pressure-calc/vapour-pressure-calc.h"
#include "../04-calculation/044-atmospheric-calc/atm-pressure-model.h"
#include "../04-calculation/047-evapotranspiration-calc/eto-calc.h"

/* Runs the daily measurement and calculation cycle */
Status RunDailyCycle(DailyResults *out, const char **out_failed_step) {
    if ((out == NULL) || (out_failed_step == NULL)) {
        return STATUS_NULL_POINTER;
    }

    *out_failed_step = "OK";

    /* *** Initialize acquisition diagnostics for this run *** */

    /* Diagnostics start "clean": every fallback status defaults to
     * "not taken" ("not reached yet"); PrintTrace() only ever
     * reports what actually happened during this run */
    out->trace.temperature_read_status = STATUS_OK;
    out->trace.humidity_read_status    = STATUS_OK;
    out->trace.pressure_read_status    = STATUS_OK;
    out->trace.pressure_model_status   = STATUS_OK;
    out->trace.wind_read_status        = STATUS_OK;
    out->trace.lux_sample_count        = 0U;

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

    status = SunshineLux_Init(&out->sunshine_data,
        CONFIG_BRIGHT_LUX_THRESHOLD, CONFIG_SAMPLE_PERIOD_SEC);
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
    out->trace.temperature_read_status = status;
    if (status != STATUS_OK) {
        status = SensorTemperature_ReadDefault(&out->t_sample);
        if (status != STATUS_OK) {
            *out_failed_step = "SensorTemperature_ReadDefault";
            return status;
        }
    }

    /* Air humidity */
    status = SensorHumidity_ReadInstant(&out->humidity_sample);
    out->trace.humidity_read_status = status;
    if (status != STATUS_OK) {
        status = SensorHumidity_ReadDefault(&out->humidity_sample);
        if (status != STATUS_OK) {
            *out_failed_step = "SensorHumidity_ReadDefault";
            return status;
        }
    }

    status = AirHumidity_Update(&out->humidity_data,
        out->humidity_sample.RH_pct, out->humidity_sample.timestamp);
    if (status != STATUS_OK) {
        *out_failed_step = "AirHumidity_Update";
        return status;
    }

    /* Atmospheric pressure (priority sources for P) */
    status = SensorPressure_ReadInstant(&out->pressure_sample);
    out->trace.pressure_read_status = status;
    if (status == STATUS_OK) {
        /* Source 1: sensor */
        out->P_source_kPa = out->pressure_sample.P_kPa;
    } else {
        /* Source 2: eq. 7 model, preferred fallback */
        status = Calc_PressureFromElevation(out->location.elevation_m,
            &out->P_source_kPa);

        out->trace.pressure_model_status = status;

        if (status != STATUS_OK) {
            /* Source 3: final fallback level */
            (void)SensorPressure_ReadDefault(&out->pressure_sample);

            out->P_source_kPa = out->pressure_sample.P_kPa;
        }
    }

    /* Wind speed */
    status = SensorWindSpeed_ReadInstant(&out->wind_sample);
    out->trace.wind_read_status = status;
    if (status != STATUS_OK) {
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
    /* PC mock: generate a small sequence of illuminance samples;
     * on MCU, SunshineLux_Update() will be driven by periodic sampling;
     * this loop will be removed and RunDailyCycle() will only finalize
     * the accumulated daily data */
    for (uint32_t i = 0U; i < DAILY_CYCLE_MOCK_LUX_SAMPLE_COUNT; ++i) {
        status = SensorLux_ReadInstant(&out->lux_sample);
        out->trace.lux_samples[i].read_status = status;

        if (status != STATUS_OK) {
            status = SensorLux_ReadDefault(&out->lux_sample);
            if (status != STATUS_OK) {
                *out_failed_step = "SensorLux_ReadDefault";
                return status;
            }
        }

        status = SunshineLux_Update(&out->sunshine_data,
            out->lux_sample.lux, out->lux_sample.source);
        if (status != STATUS_OK) {
            *out_failed_step = "SunshineLux_Update";
            return status;
        }

        out->trace.lux_samples[i].sample = out->lux_sample;
        out->trace.lux_sample_count = i + 1U;
    }

    status = SunshineLux_FinalizeDay(&out->sunshine_data);
    if (status != STATUS_OK) {
        *out_failed_step = "SunshineLux_FinalizeDay";
        return status;
    }

    /* *** Calculation layer *** */

    /* Air temperature */
    status = AirTemperature_Update(&out->temperature_data,
        out->t_sample.instant_c, out->t_sample.timestamp);
    if (status != STATUS_OK) {
        *out_failed_step = "AirTemperature_Update";
        return status;
    }

    /* Saturation vapour pressure */
    status = Calc_SaturationVapourPressure(out->temperature_data.T_mean_C,
        &out->e_tmean);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_SaturationVapourPressure";
        return status;
    }

    status = Calc_MeanSaturationVapourPressure(&out->temperature_data,
        &out->e_s);
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
    status = Calc_AtmosphericParameters(&out->atmos_data,
        out->P_source_kPa);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_AtmosphericParameters";
        return status;
    }

    /* Actual vapour pressure ea (eq. 17) */
    status = Calc_ActualVapourPressure(&out->ea_kpa,
        &out->temperature_data, &out->humidity_data);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_ActualVapourPressure";
        return status;
    }

    /* Wind speed at 2 m height (eq. 47) */
    status = Calc_WindSpeedAt2m(out->wind_data.u_z_mean_m_s,
        out->wind_data.height_m, &out->u2);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_WindSpeedAt2m";
        return status;
    }

    /* Astronomy */
    status = DateProvider_Read(&out->date);
    if (status != STATUS_OK) {
        *out_failed_step = "DateProvider_Read";
        return status;
    }

    out->current_j = DayCalc_JFromDate(out->date.day,
        out->date.month, out->date.year);

    status = DayCalc_Update(&out->day_data, out->current_j,
        &out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "DayCalc_Update";
        return status;
    }

    /* Extraterrestrial radiation */
    status = Calc_Ra(&out->ra_data, &out->day_data,
        &out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_Ra";
        return status;
    }

    /* Solar radiation */
    status = SolarRadiation_Calc(&out->angstrom,
        &out->solar_radiation, &out->ra_data, &out->day_data,
        &out->sunshine_data, &out->location);
    if (status != STATUS_OK) {
        *out_failed_step = "SolarRadiation_Calc";
        return status;
    }

    /* Net radiation */
    status = Calc_NetRadiation(&out->net_radiation,
        &out->temperature_data, &out->solar_radiation, out->ea_kpa);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_NetRadiation";
        return status;
    }

    /* Reference evapotranspiration (eq. 6, Penman-Monteith) */
    status = Calc_ETo(
        out->delta,                         /* Δ [kPa/C]                 */
        out->net_radiation.Rn_daily,        /* Rn [MJ m-2 day-1]         */
        ETO_G_DAILY_MJ_M2_DAY,              /* G = 0 for daily (eq. 42)  */
        out->atmos_data.gamma_kPa_per_C,    /* γ [kPa/C]                 */
        out->temperature_data.T_mean_C,     /* Tmean [C]                 */
        out->u2,                            /* u2 [m/s]                  */
        out->e_s,                           /* es [kPa]                  */
        out->ea_kpa,                        /* ea [kPa]                  */
        &out->eto_mm_day                    /* eto [mm/day]              */
    );

    if (status != STATUS_OK) {
        *out_failed_step = "Calc_ETo";
        return status;
    }

    /* Crop evapotranspiration (eq. 56) */
    status = Calc_ETc(out->eto_mm_day, CONFIG_CROP_KC,
        &out->etc_mm_day);
    if (status != STATUS_OK) {
        *out_failed_step = "Calc_ETc";
        return status;
    }

    return STATUS_OK;
}

/* Prints the diagnostics recorded during the cycle */
void PrintTrace(const DailyCycleTrace *trace) {
    if (trace->temperature_read_status != STATUS_OK) {
        (void)fprintf(stderr,
            "No air temperature data, using default value. Reason: %s\n",
            Status_ToString(trace->temperature_read_status));
    }

    if (trace->humidity_read_status != STATUS_OK) {
        (void)fprintf(stderr,
            "No air humidity data, using default value. Reason: %s\n",
            Status_ToString(trace->humidity_read_status));
    }

    if (trace->pressure_read_status != STATUS_OK) {
        (void)fprintf(stderr,
            "Pressure sensor unavailable (%s). Using eq.7 model.\n",
            Status_ToString(trace->pressure_read_status));

        if (trace->pressure_model_status != STATUS_OK) {
            (void)fprintf(stderr,
                "Eq. 7 model unavailable (%s). Using constant.\n",
                Status_ToString(trace->pressure_model_status));
        }
    }

    if (trace->wind_read_status != STATUS_OK) {
        (void)fprintf(stderr,
            "No wind speed data, using default value. Reason: %s\n",
            Status_ToString(trace->wind_read_status));
    }

    for (uint32_t i = 0U; i < trace->lux_sample_count; ++i) {
        const LuxSampleTrace *lux_trace = &trace->lux_samples[i];

        if (lux_trace->read_status != STATUS_OK) {
            (void)fprintf(stderr,
                "No illuminance data, using default value. Reason: %s\n",
                Status_ToString(lux_trace->read_status));
        }

        (void)printf("lux[%02u] = %.0f, source = %s\n",
            (unsigned)i, lux_trace->sample.lux,
            SensorValueSource_ToString(lux_trace->sample.source));
    }
}

/* Formatting helpers for PrintReport() */
static const int  COL_W  = 38;      /* Label column width  */
static const char *SEP   = " = ";
static const int  VAL_W  = 12;      /* Numeric field width */

static void PrintSectionHeader(const char *title) {
    (void)printf("\n=== %s ===\n", title);
}

static void PrintLabeledDouble(const char *label, const double value,
    const int precision, const char *unit, const int unit_width) {
    if (unit != NULL) {
        (void)printf("%-*s%s%*.*f %-*s\n",
            COL_W, label, SEP, VAL_W, precision, value, unit_width, unit);
    } else {
        (void)printf("%-*s%s%*.*f\n",
            COL_W, label, SEP, VAL_W, precision, value);
    }
}

static void PrintLabeledUint(const char *label, const unsigned int value,
    const char *unit, const int unit_width) {
    if (unit != NULL) {
        (void)printf("%-*s%s%*u %-*s\n",
            COL_W, label, SEP, VAL_W, value, unit_width, unit);
    } else {
        (void)printf("%-*s%s%*u\n",
            COL_W, label, SEP, VAL_W, value);
    }
}

/* Prints the full daily cycle report */
void PrintReport(const DailyResults *results) {
    PrintTrace(&results->trace);

    /* *** Output *** */

    /* Data sources */
    PrintSectionHeader("Data sources");
    (void)printf("%-*s%s%s\n", COL_W, "Air temperature",
        SEP, SensorValueSource_ToString(results->t_sample.source));
    (void)printf("%-*s%s%s\n", COL_W, "Illuminance (daily data)",
        SEP, SensorValueSource_ToString(results->sunshine_data.source));

    /* Air temperature & saturation vapour pressure */
    PrintSectionHeader("Air temperature and saturation vapour pressure");
    PrintLabeledDouble("Tmin", results->temperature_data.T_min_C, 2, "C", 6);
    PrintLabeledDouble("Tmax", results->temperature_data.T_max_C, 2, "C", 6);
    PrintLabeledDouble("Tmean", results->temperature_data.T_mean_C, 2, "C", 6);
    PrintLabeledDouble("e(Tmean)", results->e_tmean, 4, "kPa", 6);
    PrintLabeledDouble("es", results->e_s, 4, "kPa", 6);
    PrintLabeledDouble("delta", results->delta, 4, "kPa/C", 6);

    /* Atmospheric parameters */
    PrintSectionHeader("Atmospheric parameters");
    (void)printf("%-*s%s%12.2f kPa (source: %s)\n", COL_W, "P", SEP, results->atmos_data.P_kPa,
        (results->pressure_sample.source == SENSOR_VALUE_MEASURED) ? "sensor" : "model/constant");

    PrintLabeledDouble("gamma", results->atmos_data.gamma_kPa_per_C, 5, "kPa/C", 6);
    PrintLabeledDouble("RHmax", results->humidity_data.RH_max, 1, "%", 6);
    PrintLabeledDouble("RHmin", results->humidity_data.RH_min, 1, "%", 6);
    PrintLabeledDouble("ea", results->ea_kpa, 4, "kPa", 6);

    /* Wind speed */
    PrintSectionHeader("Wind speed");
    (void)printf("%-*s%s%s\n", COL_W, "Source", SEP, SensorValueSource_ToString(results->wind_sample.source));
    PrintLabeledDouble("Anemometer height (z)", results->wind_data.height_m, 1, "m", 6);
    PrintLabeledDouble("uzmean", results->wind_data.u_z_mean_m_s, 2, "m/s", 6);
    PrintLabeledDouble("u2 (eq. 47)", results->u2, 2, "m/s", 6);

    /* Astronomy */
    (void)printf("\n=== Astronomy, at J = %u, phi = %.4f rad = %.2f deg ===\n",
        results->day_data.J, results->location.latitude_rad, results->location.latitude_rad * RAD_TO_DEG);

    PrintLabeledUint("Current day of year (J)", results->current_j, NULL, 0);
    PrintLabeledDouble("Inverse relative distance", results->day_data.dr, 4, NULL, 0);

    (void)printf("%-*s%s%12.4f rad (%6.2f deg)\n", COL_W, "Solar declination",
        SEP, results->day_data.delta_rad, results->day_data.delta_rad * RAD_TO_DEG);

    PrintLabeledDouble("Sunset hour angle", results->day_data.omega_s_rad, 4, "rad", 3);
    PrintLabeledDouble("Daylight hours (N)", results->day_data.N_hours, 2, "h", 1);

    /* Extraterrestrial radiation & equivalent evaporation */
    PrintSectionHeader("Extraterrestrial radiation and equivalent evaporation");
    PrintLabeledDouble("Extraterrestrial radiation (Ra)", results->ra_data.Ra_daily, 2, "MJ m-2 day-1", 14);
    PrintLabeledDouble("Equivalent evaporation (from Ra_daily)", results->ra_data.Ra_daily * C_RAD, 2, "mm/day", 14);

    /* Solar & clear-sky radiation */
    PrintSectionHeader("Solar and clear-sky radiation");
    PrintLabeledDouble("Angstrom a_s", results->angstrom.a_s, 2, NULL, 0);
    PrintLabeledDouble("Angstrom b_s", results->angstrom.b_s, 2, NULL, 0);
    PrintLabeledDouble("Solar radiation (Rs)", results->solar_radiation.Rs_daily, 2, "MJ m-2 day-1", 14);
    PrintLabeledDouble("Clear-sky radiation (Rso)", results->solar_radiation.Rso_daily, 2, "MJ m-2 day-1", 14);

    /* Net radiation */
    PrintSectionHeader("Net radiation");
    PrintLabeledDouble("ea (actual vapour pressure)", results->ea_kpa, 2, "kPa", 6);
    PrintLabeledDouble("Net shortwave radiation (Rns)", results->net_radiation.Rns_daily, 2, "MJ m-2 day-1", 14);
    PrintLabeledDouble("Net longwave radiation (Rnl)", results->net_radiation.Rnl_daily, 2, "MJ m-2 day-1", 14);
    PrintLabeledDouble("Net radiation (Rn)", results->net_radiation.Rn_daily, 2, "MJ m-2 day-1", 14);
    PrintLabeledDouble("Equivalent evaporation (from Rn_daily)", results->net_radiation.Rn_daily * C_RAD, 2, "mm/day", 14);

    /* Sunshine duration */
    PrintSectionHeader("Sunshine duration");
    PrintLabeledDouble("Binarization threshold", CONFIG_BRIGHT_LUX_THRESHOLD, 0, "lux", 6);
    PrintLabeledUint("Sampling interval", (unsigned)CONFIG_SAMPLE_PERIOD_SEC, "s", 6);
    PrintLabeledUint("Total samples", results->sunshine_data.total_samples, NULL, 0);
    PrintLabeledUint("Bright samples", results->sunshine_data.bright_samples, NULL, 0);
    PrintLabeledDouble("Sunshine duration (n)", results->sunshine_data.n_hours, 2, "h", 6);

    /* Evapotranspiration */
    PrintSectionHeader("Evapotranspiration");
    PrintLabeledDouble("ETo (eq. 6, Penman-Monteith)", results->eto_mm_day, 3, "mm/day", 6);
    PrintLabeledDouble("Kc (crop coefficient)", CONFIG_CROP_KC, 2, NULL, 0);
    PrintLabeledDouble("ETc (eq. 56, Kc * ETo)", results->etc_mm_day, 3, "mm/day", 6);
}
