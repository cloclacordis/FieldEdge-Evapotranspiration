/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#include <math.h>
#include <stdio.h>
#include <stdint.h>

#include "../01-measurement/011-air-temperature-read/air-temperature-read.h"
#include "../01-measurement/012-air-humidity-read/air-humidity-read.h"
#include "../01-measurement/013-atm-pressure-read/atm-pressure-read.h"
#include "../01-measurement/014-sunshine-lux-read/sunshine-lux-read.h"
#include "../01-measurement/015-wind-speed-read/wind-speed-read.h"

#include "../02-providers/021-date-provider/date-provider.h"
#include "../02-providers/022-configurations/deployment-config.h"

#include "../03-validation/031-value-source/value-source.h"
#include "../03-validation/033-status/status.h"

#include "../04-calculation/041-air-temperature-calc/air-temperature-calc.h"
#include "../04-calculation/042-air-humidity-calc/air-humidity-calc.h"
#include "../04-calculation/043-vapour-pressure-calc/vapour-pressure-calc.h"
#include "../04-calculation/044-atmospheric-calc/atm-pressure-model.h"
#include "../04-calculation/044-atmospheric-calc/psychrometric-calc.h"

#include "../04-calculation/045-radiation-calc/geolocation-calc.h"
#include "../04-calculation/045-radiation-calc/day-in-year-calc.h"
#include "../04-calculation/045-radiation-calc/sunshine-lux-calc.h"

#include "../04-calculation/045-radiation-calc/extrater-radiation-calc.h"
#include "../04-calculation/045-radiation-calc/solar-radiation-calc.h"
#include "../04-calculation/045-radiation-calc/net-radiation-calc.h"

#include "../04-calculation/046-wind-speed-calc/wind-speed-calc.h"

#include "../04-calculation/047-evapotranspiration-calc/eto-calc.h"

#define PI (3.14159265358979323846)

static int PrintStatusAndReturn(const char* prefix, const Status status) {
    (void)fprintf(stderr, "%s%s\n", prefix, Status_ToString(status));
    return 1;
}

int main(void) {
    /* *** Local variable declarations *** */
    TemperatureSample   t_sample;
    AirTemperatureData  temperature_data;
    AirHumiditySample   humidity_sample;
    AirHumidityData     humidity_data;
    AtmPressureSample   pressure_sample;
    AtmosphericData     atmos_data;
    WindSpeedSample     wind_sample;
    WindSpeedData       wind_data;
    SunshineLuxSample   lux_sample;
    SunshineLuxData     sunshine_data;
    LocationData        location;
    DayData             day_data;
    DateData            date;
    RaData              ra_data;
    AngstromValues      angstrom;
    SolarRadiationData  solar_radiation;
    NetRadiationData    net_radiation;

    uint16_t            current_j    = 0U;
    double              e_tmean      = 0.0;
    double              e_s          = 0.0;
    double              delta        = 0.0;
    double              ea_kpa       = 0.0;
    double              P_source_kPa = 0.0;
    double              u2           = 0.0;
    double              eto_mm_day   = 0.0;
    double              etc_mm_day   = 0.0;

    /* *** Initialization (with formal status check) *** */
    Status status = AirTemperature_Init(&temperature_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("AirTemperatureData initialization error: ", status);
    }

    status = AirHumidity_Init(&humidity_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("AirHumidityData initialization error: ", status);
    }

    status = AtmosphericData_Init(&atmos_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("AtmosphericData initialization error: ", status);
    }

    status = WindSpeed_Init(&wind_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("WindSpeedData initialization error: ", status);
    }

    status = Location_Init(&location);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("LocationData initialization error: ", status);
    }

    status = DayCalc_Init(&day_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("DayData initialization error: ", status);
    }

    status = RaCalc_Init(&ra_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("RaData initialization error: ", status);
    }

    status = AngstromValues_Default(&angstrom);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("Angstrom-Prescott coefficients initialization error: ", status);
    }

    status = SolarRadiation_Init(&solar_radiation);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("SolarRadiationData initialization error: ", status);
    }

    status = NetRadiation_Init(&net_radiation);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("NetRadiationData initialization error: ", status);
    }

    status = SunshineLux_Init(&sunshine_data, CONFIG_BRIGHT_LUX_THRESHOLD, CONFIG_SAMPLE_PERIOD_SEC);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("SunshineLuxData initialization error: ", status);
    }

    status = SunshineLux_ResetDay(&sunshine_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("SunshineLuxData daily accumulator reset error: ", status);
    }

    /* *** Measurement layer *** */

    /* Air temperature */
    status = SensorTemperature_ReadInstant(&t_sample);
    if (status != STATUS_OK) {
        (void)fprintf(stderr,
                      "No air temperature data, using default value. "
                      "Reason: %s\n", Status_ToString(status));

        status = SensorTemperature_ReadDefault(&t_sample);
        if (status != STATUS_OK) {
            return PrintStatusAndReturn(
                "Critical error reading default air temperature data: ", status);
        }
    }

    /* Air humidity */
    status = SensorHumidity_ReadInstant(&humidity_sample);
    if (status != STATUS_OK) {
        (void)fprintf(stderr,
                      "No air humidity data, using default value. Reason: %s\n",
                      Status_ToString(status));
        status = SensorHumidity_ReadDefault(&humidity_sample);

        if (status != STATUS_OK) {
            return PrintStatusAndReturn(
                "Critical error reading default air humidity data: ", status);
        }
    }

    status = AirHumidity_Update(&humidity_data, humidity_sample.RH_pct, humidity_sample.timestamp);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Air humidity data update error: ", status);
    }

    /* Atmospheric pressure (priority sources for P) */
    status = SensorPressure_ReadInstant(&pressure_sample);
    if (status == STATUS_OK) {
        /* Source 1: sensor */
        P_source_kPa = pressure_sample.P_kPa;
    } else {
        /* Source 2: eq. 7 model, preferred fallback */
        (void)fprintf(stderr,
                      "Pressure sensor unavailable (%s). Using eq.7 model.\n",
                      Status_ToString(status));
        status = Calc_PressureFromElevation(location.elevation_m, &P_source_kPa);

        if (status != STATUS_OK) {
            /* Source 3: final fallback level */
            (void)fprintf(stderr,
                          "Eq. 7 model unavailable (%s). Using constant.\n",
                          Status_ToString(status));
            (void)SensorPressure_ReadDefault(&pressure_sample);

            P_source_kPa = pressure_sample.P_kPa;
        }
    }

    /* Wind speed */
    status = SensorWindSpeed_ReadInstant(&wind_sample);
    if (status != STATUS_OK) {
        (void)fprintf(stderr,
                      "No wind speed data, using default value. "
                      "Reason: %s\n", Status_ToString(status));
        status = SensorWindSpeed_ReadDefault(&wind_sample);
        if (status != STATUS_OK) {
            return PrintStatusAndReturn(
                "Critical error reading default wind speed data: ", status);
        }
    }

    status = WindSpeed_Update(&wind_data, wind_sample.speed_m_s, wind_sample.height_m, wind_sample.timestamp);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("Wind speed data update error: ", status);
    }

    /* Illuminance */
    /* At the PC version we read a sequence of mock values; on MCU the same call
     * through the same read contract will be used, but SensorLux_ReadInstant() will become driver-level */
    for (uint32_t i = 0U; i < 12U; ++i) {
        status = SensorLux_ReadInstant(&lux_sample);
        if (status != STATUS_OK) {
            (void)fprintf(stderr,
                          "No illuminance data, using default value. "
                          "Reason: %s\n", Status_ToString(status));

            status = SensorLux_ReadDefault(&lux_sample);
            if (status != STATUS_OK) {
                return PrintStatusAndReturn(
                    "Critical error reading default illuminance data: ", status);
            }
        }

        status = SunshineLux_Update(&sunshine_data, lux_sample.lux, lux_sample.source);
        if (status != STATUS_OK) {
            return PrintStatusAndReturn(
                "Sunshine duration counter update error: ", status);
        }

        (void)printf("lux[%02u] = %.0f, source = %s\n",
                     (unsigned)i, lux_sample.lux, SensorValueSource_ToString(lux_sample.source));
    }

    status = SunshineLux_FinalizeDay(&sunshine_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Daily sunshine duration finalization error: ", status);
    }

    /* *** Calculation layer *** */

    /* Air temperature */
    status = AirTemperature_Update(&temperature_data, t_sample.instant_c, t_sample.timestamp);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Air temperature update error: ", status);
    }

    /* Saturation vapour pressure */
    status = Calc_SaturationVapourPressure(temperature_data.T_mean_C, &e_tmean);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "e(Tmean) calculation error: ", status);
    }

    status = Calc_MeanSaturationVapourPressure(&temperature_data, &e_s);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "e_s calculation error: ", status);
    }

    status = Calc_SlopeDelta(&temperature_data, &delta);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Delta calculation error: ", status);
    }

    /* Psychrometric constant from P (eq. 8) */
    status = Calc_AtmosphericParameters(&atmos_data, P_source_kPa);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Psychrometric constant calculation error: ", status);
    }

    /* Actual vapour pressure ea (eq. 17) */
    status = Calc_ActualVapourPressure(&ea_kpa, &temperature_data, &humidity_data);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Actual vapour pressure (ea) calculation error: ", status);
    }

    /* Wind speed at 2 m height (eq. 47) */
    status = Calc_WindSpeedAt2m(wind_data.u_z_mean_m_s, wind_data.height_m, &u2);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("Wind speed conversion to 2 m height error (eq. 47): ", status);
    }

    /* Astronomy */
    status = DateProvider_Read(&date);  /* Get current day of year */
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("Day of year read error: ", status);
    }

    current_j = DayCalc_JFromDate(date.day, date.month, date.year);

    status = DayCalc_Update(&day_data, current_j, &location);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Astronomical data calculation error: ", status);
    }

    /* Extraterrestrial radiation */
    status = Calc_Ra(&ra_data, &day_data, &location);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Extraterrestrial radiation (Ra) calculation error: ", status);
    }

    /* Solar radiation */
    status = SolarRadiation_Calc(&angstrom, &solar_radiation, &ra_data, &day_data, &sunshine_data, &location);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn(
            "Solar radiation (Rs/Rso) calculation error: ", status);
    }

    /* Net radiation */
    status = Calc_NetRadiation(&net_radiation, &temperature_data, &solar_radiation, ea_kpa);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("Net radiation (Rn) calculation error: ", status);
    }

    /* Reference evapotranspiration (eq. 6, Penman-Monteith) */
    status = Calc_ETo(
        delta,                               /* Δ [kPa/C]                 */
        net_radiation.Rn_daily,              /* Rn [MJ m-2 day-1]         */
        ETO_G_DAILY_MJ_M2_DAY,               /* G = 0 for daily (eq. 42)  */
        atmos_data.gamma_kPa_per_C,          /* γ [kPa/C]                 */
        temperature_data.T_mean_C,           /* Tmean [C]                 */
        u2,                                  /* u2 [m/s]                  */
        e_s,                                 /* es [kPa]                  */
        ea_kpa,                              /* ea [kPa]                  */
        &eto_mm_day                          /* eto [mm/day]              */
    );

    if (status != STATUS_OK) {
        return PrintStatusAndReturn("ETo (eq. 6) calculation error: ", status);
    }

    /* Crop evapotranspiration (eq. 56) */
    status = Calc_ETc(eto_mm_day, CONFIG_CROP_KC, &etc_mm_day);
    if (status != STATUS_OK) {
        return PrintStatusAndReturn("ETc (eq. 56) calculation error: ", status);
    }

    /* *** Output *** */
    #define COL_W 38
    #define SEP " = "
    
    /* Data sources */
    (void)printf("\n=== Data sources ===\n");
    (void)printf("%-*s%s%s\n", COL_W, "Air temperature",
                 SEP, SensorValueSource_ToString(t_sample.source));

    (void)printf("%-*s%s%s\n", COL_W, "Illuminance (daily data)",
                 SEP, SensorValueSource_ToString(sunshine_data.source));
    
    /* Air temperature & saturation vapour pressure */
    (void)printf("\n=== Air temperature and saturation vapour pressure ===\n");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmin", SEP, temperature_data.T_min_C, "C");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmax", SEP, temperature_data.T_max_C, "C");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Tmean", SEP, temperature_data.T_mean_C, "C");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "e(Tmean)", SEP, e_tmean, "kPa");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "es", SEP, e_s, "kPa");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "delta", SEP, delta, "kPa/C");
    
    /* Atmospheric parameters */
    (void)printf("\n=== Atmospheric parameters ===\n");
    (void)printf("%-*s%s%12.2f kPa (source: %s)\n", COL_W, "P", SEP, atmos_data.P_kPa,
                 (pressure_sample.source == SENSOR_VALUE_MEASURED) ? "sensor" : "model/constant");

    (void)printf("%-*s%s%12.5f %-6s\n", COL_W, "gamma", SEP, atmos_data.gamma_kPa_per_C, "kPa/C");
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "RHmax", SEP, humidity_data.RH_max, "%");
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "RHmin", SEP, humidity_data.RH_min, "%");
    (void)printf("%-*s%s%12.4f %-6s\n", COL_W, "ea", SEP, ea_kpa, "kPa");
    
    /* Wind speed */
    (void)printf("\n=== Wind speed ===\n");
    (void)printf("%-*s%s%s\n", COL_W, "Source", SEP, SensorValueSource_ToString(wind_sample.source));
    (void)printf("%-*s%s%12.1f %-6s\n", COL_W, "Anemometer height (z)", SEP, wind_data.height_m, "m");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "uzmean", SEP, wind_data.u_z_mean_m_s, "m/s");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "u2 (eq. 47)", SEP, u2, "m/s");
    
    /* Astronomy */
    (void)printf("\n=== Astronomy, at J = %u, phi = %.4f rad = %.2f deg ===\n",
                 day_data.J, location.latitude_rad, location.latitude_rad * (180.0 / PI));

    (void)printf("%-*s%s%12u\n", COL_W, "Current day of year (J)", SEP, current_j);
    (void)printf("%-*s%s%12.4f\n", COL_W, "Inverse relative distance", SEP, day_data.dr);
    
    (void)printf("%-*s%s%12.4f rad (%6.2f deg)\n", COL_W, "Solar declination",
                 SEP, day_data.delta_rad, day_data.delta_rad * (180.0 / PI));

    (void)printf("%-*s%s%12.4f rad\n", COL_W, "Sunset hour angle", SEP, day_data.omega_s_rad);
    (void)printf("%-*s%s%12.2f h\n", COL_W, "Daylight hours (N)", SEP, day_data.N_hours);
    
    /* Extraterrestrial radiation & equivalent evaporation */
    (void)printf("\n=== Extraterrestrial radiation and equivalent evaporation ===\n");
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Extraterrestrial radiation (Ra)",
                 SEP, ra_data.Ra_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Equivalent evaporation (from Ra_daily)",
                 SEP, ra_data.Ra_daily * 0.408, "mm/day");
    
    /* Solar & clear-sky radiation */
    (void)printf("\n=== Solar and clear-sky radiation ===\n");
    (void)printf("%-*s%s%12.2f\n", COL_W, "Angstrom a_s", SEP, angstrom.a_s);
    (void)printf("%-*s%s%12.2f\n", COL_W, "Angstrom b_s", SEP, angstrom.b_s);
    
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Solar radiation (Rs)",
                 SEP, solar_radiation.Rs_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Clear-sky radiation (Rso)",
                 SEP, solar_radiation.Rso_daily, "MJ m-2 day-1");
    
    /* Net radiation */
    (void)printf("\n=== Net radiation ===\n");
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "ea (actual vapour pressure)", SEP, ea_kpa, "kPa");
    
    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net shortwave radiation (Rns)",
                 SEP, net_radiation.Rns_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net longwave radiation (Rnl)",
                 SEP, net_radiation.Rnl_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Net radiation (Rn)",
                 SEP, net_radiation.Rn_daily, "MJ m-2 day-1");

    (void)printf("%-*s%s%12.2f %-14s\n", COL_W, "Equivalent evaporation (from Rn_daily)",
                 SEP, net_radiation.Rn_daily * 0.408, "mm/day");
    
    /* Sunshine duration */
    (void)printf("\n=== Sunshine duration ===\n");
    (void)printf("%-*s%s%12.0f %-6s\n", COL_W, "Binarization threshold",
                 SEP, CONFIG_BRIGHT_LUX_THRESHOLD, "lux");

    (void)printf("%-*s%s%12u %-6s\n", COL_W, "Sampling interval",
                 SEP, (unsigned)CONFIG_SAMPLE_PERIOD_SEC, "s");

    (void)printf("%-*s%s%12u\n", COL_W, "Total samples", SEP, sunshine_data.total_samples);
    (void)printf("%-*s%s%12u\n", COL_W, "Bright samples", SEP, sunshine_data.bright_samples);
    (void)printf("%-*s%s%12.2f %-6s\n", COL_W, "Sunshine duration (n)", SEP, sunshine_data.n_hours, "h");
    
    /* Evapotranspiration */
    (void)printf("\n=== Evapotranspiration ===\n");
    (void)printf("%-*s%s%12.3f %-6s\n", COL_W, "ETo (eq. 6, Penman-Monteith)", SEP, eto_mm_day, "mm/day");
    (void)printf("%-*s%s%12.2f\n", COL_W, "Kc (crop coefficient)", SEP, CONFIG_CROP_KC);
    (void)printf("%-*s%s%12.3f %-6s\n", COL_W, "ETc (eq. 56, Kc * ETo)", SEP, etc_mm_day, "mm/day");

    #undef COL_W
    #undef SEP

    return 0;
}
