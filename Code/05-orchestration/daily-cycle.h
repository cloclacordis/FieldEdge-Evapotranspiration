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

/**
 * @brief Runs one full daily measurement-and-calculation cycle
 *        (FAO-56 ETo/ETc).
 *
 * Executes, in strict sequence: initialization of all layer-owned
 * data structures, measurement acquisition (temperature, humidity,
 * pressure, wind, illuminance) with per-sensor fallback to defaults,
 * astronomical and psychrometric calculations, and the FAO-56
 * Penman-Monteith evapotranspiration chain (ETo, ETc). Performs no
 * I/O (see PrintReport() / PrintTrace() for reporting).
 *
 * Recoverable sensor failures (a single ReadInstant() call failing)
 * do not abort the cycle: the matching ReadDefault() fallback is used
 * instead, and the failure is recorded in out->trace. Atmospheric
 * pressure is the one exception with three, not two, priority sources
 * (sensor, elevation model, constant) - see the pressure-acquisition
 * block and data-flow-specification.md. Only an unrecoverable failure
 * (every fallback for a step failing, or a calculation step failing)
 * aborts the cycle.
 *
 * @param[out] out             Destination for all results. Must not
 *                             be NULL. On success, every field is
 *                             populated. On failure, out->trace
 *                             reflects diagnostics captured up to
 *                             the failure point; other fields are
 *                             only partially populated.
 * @param[out] out_failed_step Destination for a static string naming
 *                             the failing step. Must not be NULL.
 *                             Set to "OK" at the start of the call.
 *
 * @pre  out != NULL && out_failed_step != NULL.
 * @post On STATUS_OK: *out_failed_step == "OK" and every *out field
 *       is valid.
 * @post On failure: *out_failed_step names the failing step.
 *
 * @retval STATUS_OK           The cycle completed; all results are valid.
 * @retval STATUS_NULL_POINTER out or out_failed_step was NULL.
 * @retval (other)             Propagated verbatim from the failing
 *                             step; see *out_failed_step and
 *                             Status_ToString().
 *
 * @see verified-call-graph.md for the exact, gdb-verified call order.
 * @see data-flow-specification.md for field-level producer/consumer detail.
 */
Status RunDailyCycle(DailyResults *out, const char **out_failed_step);

/**
 * @brief Prints acquisition diagnostics captured during a
 *        RunDailyCycle() run.
 *
 * For each measurement channel that fell back to a default or
 * alternate source, prints a one-line warning to stderr naming the
 * reason (Status_ToString()). Always prints one line per captured
 * illuminance sample to stdout, regardless of whether that sample
 * succeeded.
 *
 * @param[in] trace Diagnostics to report. Must not be NULL - no NULL
 *                  check is performed.
 *
 * @warning No NULL check on @p trace; passing NULL is undefined behavior.
 *
 * @note Shared by both outcomes: called from PrintReport() on success
 *       and from PrintStatusAndReturn() (main.c) on failure.
 */
void PrintTrace(const DailyCycleTrace *trace);

/**
 * @brief Prints the full report for a successful RunDailyCycle() run.
 *
 * Calls PrintTrace() first, then every report section (data sources,
 * temperature, atmospheric parameters, wind, astronomy, radiation,
 * sunshine duration, evapotranspiration).
 *
 * @param[in] results A fully populated DailyResults from a run that
 *                    returned STATUS_OK.
 *
 * @warning No NULL check is performed; passing NULL, or a partially
 *          populated DailyResults from a failed run, is undefined
 *          behavior - use PrintStatusAndReturn() for the failure case.
 */
void PrintReport(const DailyResults *results);

#ifdef __cplusplus
}
#endif

#endif /* DAILY_CYCLE_H */
