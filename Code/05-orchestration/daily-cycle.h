/* SPDX-License-Identifier: AGPL-3.0-or-later
 * Copyright (C) 2026 Tim Alexeenko (@cloclacordis) */

#ifndef DAILY_CYCLE_H
#define DAILY_CYCLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "../01-measurement/011-air-temperature-read/air-temperature-read.h"
#include "../01-measurement/012-air-humidity-read/air-humidity-read.h"
#include "../01-measurement/013-atm-pressure-read/atm-pressure-read.h"
#include "../01-measurement/014-sunshine-lux-read/sunshine-lux-read.h"
#include "../01-measurement/015-wind-speed-read/wind-speed-read.h"

#include "../02-providers/021-date-provider/date-provider.h"
#include "../03-validation/033-status/status.h"

#include "../04-calculation/041-air-temperature-calc/air-temperature-calc.h"
#include "../04-calculation/042-air-humidity-calc/air-humidity-calc.h"
#include "../04-calculation/044-atmospheric-calc/psychrometric-calc.h"

#include "../04-calculation/045-radiation-calc/geolocation-calc.h"
#include "../04-calculation/045-radiation-calc/day-in-year-calc.h"
#include "../04-calculation/045-radiation-calc/sunshine-lux-calc.h"

#include "../04-calculation/045-radiation-calc/extrater-radiation-calc.h"
#include "../04-calculation/045-radiation-calc/solar-radiation-calc.h"
#include "../04-calculation/045-radiation-calc/net-radiation-calc.h"

#include "../04-calculation/046-wind-speed-calc/wind-speed-calc.h"

/* Number of illuminance samples used by the PC mock per RunDailyCycle();
 * independent of CONFIG_SAMPLE_PERIOD_SEC; replaced by real periodic
 * sampling when the MCU implementation is integrated */
#define DAILY_CYCLE_MOCK_LUX_SAMPLE_COUNT 12U

/*  Illuminance reading and its acquisition status, stored for later diagnostics */
typedef struct {
    SunshineLuxSample sample;         /* Value + source finally used ** *** **** */
    Status            read_status;    /* SensorLux_ReadInstant() result **** *** */
} LuxSampleTrace;

/* Acquisition diagnostics captured by RunDailyCycle() for later reporting */
typedef struct {
    Status         temperature_read_status;  /* SensorTemperature_ReadInstant() result */
    Status         humidity_read_status;     /* SensorHumidity_ReadInstant() result ** */
    Status         pressure_read_status;     /* SensorPressure_ReadInstant() result ** */
    Status         pressure_model_status;    /* Calc_PressureFromElevation() result ** */
    Status         wind_read_status;         /* SensorWindSpeed_ReadInstant() result * */
    LuxSampleTrace lux_samples[DAILY_CYCLE_MOCK_LUX_SAMPLE_COUNT];
    uint32_t       lux_sample_count;         /* Number of valid lux_samples entries ** */
} DailyCycleTrace;

typedef struct {
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
    DailyCycleTrace     trace;
    uint16_t            current_j;
    double              e_tmean;
    double              e_s;
    double              delta;
    double              ea_kpa;
    double              P_source_kPa;
    double              u2;
    double              eto_mm_day;
    double              etc_mm_day;
} DailyResults;

/* Runs the daily measurement and calculation cycle without I/O;
 * recoverable sensor failures use defaults and are recorded in trace;
 * on failure, returns the status and stores the failed step in
 * out_failed_step; on success, returns STATUS_OK and stores "OK" */
Status RunDailyCycle(DailyResults *out, const char **out_failed_step);

/* Prints acquisition diagnostics stored in trace; performs stdio I/O */
void PrintTrace(const DailyCycleTrace *trace);

/* Prints the full report for a successful RunDailyCycle() run */
void PrintReport(const DailyResults *results);

#ifdef __cplusplus
}
#endif

#endif /* DAILY_CYCLE_H */
