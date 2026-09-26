# Contracts and conventions

## Scope

Full contract goes on the **declaration** in the `.h` file. A function with no `.h` declaration (`static`, internal to one `.c` file) gets its full contract directly above its **definition** in the `.c` file.

`Status` has five values (see [`status.h`](../Code/03-validation/033-status/status.h)): `STATUS_OK`, `STATUS_NULL_POINTER`, `STATUS_INVALID_VALUE`, `STATUS_UNAVAILABLE`, `STATUS_INTERNAL_ERROR`. Only the first three are returned anywhere in the current v0.1.x codebase; `STATUS_UNAVAILABLE` and `STATUS_INTERNAL_ERROR` are reserved for the v0.2.x stage. Contracts below list only what a function actually returns in v0.1.x.

* * *

## Conventions: five recurring patterns


**A — `*_Init(data)`.** `NULL`-checked, zeroed via `memset`, and returns `STATUS_OK`. A few modules (`AtmosphericData`, `NetRadiationData`, `SolarRadiationData`) also set `initialized = true` immediately, which changes the postcondition.

**B — `Sensor<X>_ReadInstant` and `Sensor<X>_ReadDefault` pair.** Currently identical in structure: `NULL`-checked, return a fixed PC-mock value, and tag it as `SENSOR_VALUE_MEASURED` or `SENSOR_VALUE_DEFAULT`. In v0.1.x, `ReadInstant()` fails only on a `NULL` pointer; a real driver will fail for other reasons as well.

**C — `*_Update(data, value, timestamp)`.** Validates the new value before mutating `*data`; on invalid input, `*data` is left unchanged. Some functions are designed to accumulate a running min/max/mean across repeated calls: `AirTemperatureData`, `AirHumidityData`, `WindSpeedData`, `SunshineLuxData` — the actual “Layer state” accumulators (this temporal accumulation was deferred to v0.2.x). One function (`Calc_AtmosphericParameters`) merely overwrites a single-call result and is documented under Pattern D despite the `_Init()` pairing. See the [`Data flow specification`](data-flow-specification.md).

**D — `Calc_X(...)`.** Pure calculation: reads only its parameters, writes the result via an out parameter, performs no I/O, and maintains no persistent state.

**E — fail-fast idiom, `RunDailyCycle()` only.**

```C
status = Step(...);
if (status != STATUS_OK) {
    *out_failed_step = "Step";
    return status;
}
```

* * *

## `01-measurement`

### `air-temperature-read.h`

```C
/**
 * @brief Reads an instantaneous air temperature value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_INSTANT_C) with a live timestamp; fails only on a
 * NULL pointer. A real sensor driver will fail for other reasons too.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pairs with SensorTemperature_ReadDefault(), the fallback used
 *       by RunDailyCycle() when this call does not return STATUS_OK.
 */
Status SensorTemperature_ReadInstant(TemperatureSample* out_sample);

/**
 * @brief Returns the fallback air temperature reading.
 *
 * Fixed value (SENSOR_DEFAULT_INSTANT_C), zero timestamp, `.source`
 * set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorTemperature_ReadDefault(TemperatureSample* out_sample);
```

* * *

### `air-humidity-read.h`

```C
/**
 * @brief Reads an instantaneous relative humidity value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_INSTANT_RH_PCT) with a live timestamp; fails only on
 * a NULL pointer.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pairs with SensorHumidity_ReadDefault().
 */
Status SensorHumidity_ReadInstant(AirHumiditySample *out_sample);

/**
 * @brief Returns the fallback relative humidity reading.
 *
 * Fixed value (SENSOR_DEFAULT_INSTANT_RH_PCT), zero timestamp,
 * `.source` set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorHumidity_ReadDefault(AirHumiditySample *out_sample);
```

* * *

### `atm-pressure-read.h`

```C
/**
 * @brief Reads an instantaneous atmospheric pressure value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_P_KPA) with a live timestamp; fails only on a NULL
 * pointer.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pressure has a three-source priority in RunDailyCycle():
 *       sensor (this function), then Calc_PressureFromElevation()
 *       (FAO-56, eq.), then SensorPressure_ReadDefault() as
 *       the last resort. See also atm-pressure-model.c.
 */
Status SensorPressure_ReadInstant(AtmPressureSample *out_sample);

/**
 * @brief Returns the final-fallback atmospheric pressure reading.
 *
 * Fixed standard sea-level value (SENSOR_DEFAULT_P_KPA), zero
 * timestamp, `.source` set to SENSOR_VALUE_DEFAULT. Used only if both
 * SensorPressure_ReadInstant() and Calc_PressureFromElevation() fail.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorPressure_ReadDefault(AtmPressureSample *out_sample);
```

* * *

### `sunshine-lux-read.h`

```C
/**
 * @brief Reads an instantaneous illuminance value from the sensor.
 *
 * PC-mock implementation: always returns a fixed representative
 * illuminance value (SENSOR_MOCK_INSTANT_LUX = 55000 lux as "clear sky")
 * with a live timestamp; fails only on a NULL pointer.
 *
 * @warning lux is a photometric quantity (human-eye-weighted), not
 *          the radiometric direct-beam irradiance (W/m^2). No single
 *          lux-to-W/m^2 conversion factor exists (~21-131 lm/W in the
 *          literature). Furthermore, this global-illuminance reading
 *          includes diffuse skylight, which the WMO reference excludes.
 *          See issues-v01x.md (item 4) and illuminance-proxy.md.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Called once per loop iteration by RunDailyCycle()
 *       (DAILY_CYCLE_MOCK_LUX_SAMPLE_COUNT times), not once per run.
 *       See verified-call-graph.md.
 *       Illuminance is used as a proxy; the "clear sky" threshold
 *       is provisional and requires further validation.
 */
Status SensorLux_ReadInstant(SunshineLuxSample* out_sample);

/**
 * @brief Returns the fallback illuminance reading.
 *
 * Fixed value (SENSOR_DEFAULT_INSTANT_LUX = 0, i.e. "no data"),
 * zero timestamp, `.source` set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorLux_ReadDefault(SunshineLuxSample* out_sample);
```

* * *

### `wind-speed-read.h`

```C
/**
 * @brief Reads an instantaneous wind speed value from the sensor.
 *
 * PC-mock implementation: always returns a fixed value
 * (SENSOR_MOCK_WIND_SPEED_MS) at the WMO standard measurement height 10 m
 * (CONFIG_WIND_HEIGHT_WMO_M, deployment-config.h), with a live
 * timestamp; fails only on a NULL pointer.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid; `.source` is
 *                             SENSOR_VALUE_MEASURED.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 *
 * @note Pairs with SensorWindSpeed_ReadDefault(). The fallback uses a
 *       different measurement height (2 m, not 10 m) - downstream
 *       WindSpeed_Update() rejects a height change between calls, so
 *       this is safe because ReadInstant and ReadDefault are
 *       never both accepted within the same run.
 */
Status SensorWindSpeed_ReadInstant(WindSpeedSample *out_sample);

/**
 * @brief Returns the fallback wind speed reading.
 *
 * Fixed value (SENSOR_DEFAULT_WIND_SPEED_MS) at the FAO-56 standard
 * height 2 m (CONFIG_WIND_HEIGHT_FAO_M, deployment-config.h),
 * zero timestamp, `.source` set to SENSOR_VALUE_DEFAULT.
 *
 * @param[out] out_sample Destination for the reading. Must not be NULL.
 *
 * @retval STATUS_OK           *out_sample is valid.
 * @retval STATUS_NULL_POINTER out_sample was NULL.
 */
Status SensorWindSpeed_ReadDefault(WindSpeedSample *out_sample);
```

* * *

## `02-providers`

### `date-provider.h`

```C
/**
 * @brief Reads the current calendar date from the host system clock.
 *
 * Wraps time() and localtime(). Uses the host's configured local
 * time zone. This is a known open question for the RTC-based
 * replacement in v0.2.x (see README, "Limitations").
 *
 * @param[out] date Destination for the date. Must not be NULL.
 *
 * @retval STATUS_OK            *date is valid.
 * @retval STATUS_NULL_POINTER  date was NULL.
 * @retval STATUS_INVALID_VALUE `time()` or `localtime()`
 *                              failed (unable to obtain the date).
 */
Status DateProvider_Read(DateData* date);
```

* * *

## `03-validation`

### `value-source.h`

```C
/**
 * @brief Returns a human-readable name for a SensorValueSource value.
 *
 * @param[in] source A SensorValueSource value. Values not matching any
 *                   defined enumerator are returned as "UNKNOWN".
 *
 * @return "MEASURED", "DEFAULT", or "UNKNOWN" for an unrecognized value.
 */
const char* SensorValueSource_ToString(SensorValueSource source);
```

* * *

### `validation.h`

```C
/**
 * @brief Checks whether a value is within the accepted
 *        air-temperature range.
 *
 * @param[in] value Candidate temperature [C].
 *
 * @return true iff isfinite(value) && -100.0 <= value <= 100.0.
 *
 * @note The range is a temporary protective corridor, not a
 *       climate-derived bound.
 */
bool ValidTemperatureC(double value);

/**
 * @brief Checks whether a value is a valid relative humidity.
 *
 * @param[in] value Candidate relative humidity [%].
 *
 * @return true iff isfinite(value) && 0.0 <= value <= 100.0.
 */
bool ValidHumidityPercent(double value);

/**
 * @brief Checks whether a value is a valid latitude, in radians.
 *
 * @param[in] phi Candidate latitude [rad].
 *
 * @return true iff isfinite(phi) && -pi/2 <= phi <= +pi/2. The full
 *         range, including the poles, is intentionally accepted.
 */
bool ValidLatitudeRad(double phi);

/**
 * @brief Checks whether a value is a valid day-of-year value.
 *
 * @param[in] J Candidate day of year.
 *
 * @return true iff 1 <= J <= 366. 366 is always accepted.
 *         Whether the year associated with J is a leap year
 *         is outside the scope of this function.
 */
bool ValidDayOfYear(uint16_t J);
```

* * *

### `status.h`

```C
/**
 * @brief Returns a human-readable name for a Status value.
 *
 * @param[in] status A Status value. Values not matching any defined
 *                   enumerator are returned as "STATUS_UNKNOWN".
 *
 * @return A static string naming @p status, or "STATUS_UNKNOWN"
 *         for an unrecognized value.
 */
const char* Status_ToString(Status status);
```

* * *

### `math-utils.h`

```C
/**
 * @brief Returns the smaller of two values.
 *
 * @param[in] a First value.
 * @param[in] b Second value.
 *
 * @return a if a < b, otherwise b. No special NaN handling: if either
 *         input is NaN, the comparison is false and b is returned.
 *         Thus, (NaN, b) returns b, while (a, NaN) returns NaN.
 *         NaN in a is discarded, whereas NaN in b propagates.
 */
static inline double Min(double a, double b);

/**
 * @brief Returns the larger of two values.
 *
 * @param[in] a First value.
 * @param[in] b Second value.
 *
 * @return a if a > b, otherwise b. No special NaN handling: if either
 *         input is NaN, the comparison is false and b is returned.
 *         Thus, (NaN, b) returns b, while (a, NaN) returns NaN.
 *         NaN in a is discarded, whereas NaN in b propagates.
 */
static inline double Max(double a, double b);
```

* * *

## `04-calculation`

### `air-temperature-calc.h`

```C
/**
 * @brief Initializes air temperature data to a safe zero state.
 *
 * @param[out] data Pointer to the AirTemperatureData structure to
 *                  initialize. Must not be NULL.
 *
 * @post On success, *data is zero-initialized; `.initialized` is false.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status AirTemperature_Init(AirTemperatureData* data);

/**
 * @brief Folds one instantaneous reading into the daily min/max/mean.
 *
 * Validates @p T_inst_C before accepting it. The first accepted
 * reading initializes both min and max to that value; each
 * subsequent one only widens the range. `.T_mean_C` is recomputed as
 * `(T_max_C + T_min_C) / 2` on every accepted call (a range midpoint,
 * not a running arithmetic mean of all samples).
 *
 * @param[in,out] data      Accumulator to update. Must not be NULL.
 * @param[in]     T_inst_C  Instantaneous air temperature [C]. Must
 *                          satisfy ValidTemperatureC().
 * @param[in]     timestamp Acquisition time of T_inst_C.
 *
 * @retval STATUS_OK            Accepted; *data reflects the new reading.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE T_inst_C failed ValidTemperatureC();
 *                              *data unchanged.
 */
Status AirTemperature_Update(AirTemperatureData* data, double T_inst_C, uint32_t timestamp);
```

* * *

### `air-humidity-calc.h`

```C
/**
 * @brief Initializes air humidity data to a safe zero state.
 *
 * @param[out] data Pointer to the AirHumidityData structure to
 *                  initialize. Must not be NULL.
 *
 * @post On success, *data is zero-initialized; `.initialized` is false.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status AirHumidity_Init(AirHumidityData *data);

/**
 * @brief Folds one instantaneous reading into the daily min/max/mean.
 *
 * Same accumulation pattern as AirTemperature_Update(): first accepted
 * reading initializes min and max; `.RH_mean` is the range midpoint,
 * recomputed on every accepted call.
 *
 * @param[in,out] data      Accumulator to update. Must not be NULL.
 * @param[in]     RH_pct    Instantaneous relative humidity [%]. Must
 *                          satisfy ValidHumidityPercent().
 * @param[in]     timestamp Acquisition time of RH_pct.
 *
 * @retval STATUS_OK            Accepted; *data reflects the new reading.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE RH_pct failed ValidHumidityPercent();
 *                              *data unchanged.
 */
Status AirHumidity_Update(AirHumidityData *data, double RH_pct, uint32_t timestamp);
```

* * *

### `vapour-pressure-calc.h`

```C
/**
 * @brief Computes the saturation vapour pressure e(T) at an arbitrary
 *        temperature using the Magnus-Tetens formula (FAO-56, eq. 11).
 *
 * e(T) = 0.6108 * exp((17.27 * T) / (T + 237.3)).
 *
 * @param[in]  T_c       Air temperature [C]. Must be finite;
 *                       physically valid outside a narrow range,
 *                       no additional bound is enforced here.
 * @param[out] e_sat_kPa Destination for the result [kPa]. Must not
 *                       be NULL.
 *
 * @retval STATUS_OK            *e_sat_kPa is valid.
 * @retval STATUS_NULL_POINTER  e_sat_kPa was NULL.
 * @retval STATUS_INVALID_VALUE T_c was NaN or infinite.
 */
Status Calc_SaturationVapourPressure(double T_c, double *e_sat_kPa);

/**
 * @brief Computes mean saturation vapour pressure (FAO-56, eq. 12).
 *
 * es = (e(Tmax) + e(Tmin)) / 2, each e(T) per eq. 11.
 *
 * @param[in]  Tdata   Accumulated daily temperature data. Must not
 *                     be NULL; `.initialized` must be true and
 *                     `.T_max_C`/`.T_min_C` must satisfy
 *                     ValidTemperatureC().
 * @param[out] out_kPa Destination for the result [kPa]. Must not be
 *                     NULL.
 *
 * @retval STATUS_OK            *out_kPa is valid.
 * @retval STATUS_NULL_POINTER  Tdata or out_kPa was NULL.
 * @retval STATUS_INVALID_VALUE Tdata not initialized, or its stored
 *                              T_max_C/T_min_C is out of range.
 */
Status Calc_MeanSaturationVapourPressure(const AirTemperatureData* Tdata, double* out_kPa);

/**
 * @brief Computes the slope of the saturation vapour pressure curve
 *        using mean air temperature (FAO-56, eq. 13).
 *
 * delta = (4098 * e(Tmean)) / (Tmean + 237.3)^2, e(T) per eq. 11.
 *
 * @param[in]  Tdata         Accumulated daily temperature data. Must
 *                           not be NULL; `.initialized` must be
 *                           true and `.T_mean_C` must satisfy
 *                           ValidTemperatureC().
 * @param[out] out_kPa_per_C Destination for the result [kPa/C]. Must
 *                           not be NULL.
 *
 * @retval STATUS_OK            *out_kPa_per_C is valid.
 * @retval STATUS_NULL_POINTER  Tdata or out_kPa_per_C was NULL.
 * @retval STATUS_INVALID_VALUE Tdata not initialized, its T_mean_C is
 *                              out of range, or the denominator is
 *                              numerically degenerate (T_mean_C at or
 *                              near -237.3 C).
 */
Status Calc_SlopeDelta(const AirTemperatureData* Tdata, double* out_kPa_per_C);

/**
 * @brief Computes actual vapour pressure from accumulated relative
 *        humidity and temperature (FAO-56, eq. 17).
 *
 * ea = [e(Tmin)*RHmax/100 + e(Tmax)*RHmin/100] / 2, e(T) per eq. 11.
 *
 * @param[out] ea_kPa   Destination for the result [kPa]. Must not
 *                      be NULL.
 * @param[in]  temp     Accumulated air temperature data. Must not
 *                      be NULL; `.initialized` must be true.
 * @param[in]  humidity Accumulated relative humidity data. Must not
 *                      be NULL; `.initialized` must be true.
 *
 * @retval STATUS_OK            *ea_kPa is valid.
 * @retval STATUS_NULL_POINTER  ea_kPa, temp, or humidity was NULL.
 * @retval STATUS_INVALID_VALUE temp or humidity not initialized; or
 *                              the result was non-finite or negative;
 *                              also propagated verbatim from an
 *                              internal Calc_SaturationVapourPressure()
 *                              call, in principle unreachable given
 *                              already-validated temp data.
 */
Status Calc_ActualVapourPressure(double *ea_kPa, const AirTemperatureData *temp, const AirHumidityData *humidity);
```

Static helper in **`vapour-pressure-calc.c`**:

```C
/**
 * @brief Magnus-Tetens saturation vapour pressure formula (FAO-56, eq. 11).
 *
 * e(T) = 0.6108 * exp((17.27 * T) / (T + 237.3)).
 *
 * Internal to this file; no NULL check (never called with an invalid
 * pointer - it takes no pointer) and no input validation - callers
 * (Calc_SaturationVapourPressure() and this file's other Calc_*
 * functions) are responsible for rejecting non-finite input first.
 *
 * @param[in] temperature_c Air temperature [C].
 *
 * @return e(T) [kPa], per eq. 11. Not itself range-checked; may be
 *         non-finite if @p temperature_c is extreme enough to
 *         overflow `exp()`.
 */
static double Calc_TetensSaturationPressure(const double temperature_c);
```

* * *

### `atm-pressure-model.h`

```C
/**
 * @brief Estimates atmospheric pressure from elevation (FAO-56, eq. 7).
 *
 * P = 101.3 * [(293 - 0.0065 * z) / 293]^5.26.
 *
 * Used as the second-priority fallback (after the sensor, before the
 * fixed constant) when the pressure sensor is unavailable - see
 * data-flow-specification.md.
 *
 * @param[in]  elevation_m Station elevation above sea level [m].
 *                         Must be in [-500, 6000].
 * @param[out] P_kPa       Destination for the result [kPa]. Must
 *                         not be NULL.
 *
 * @retval STATUS_OK            *P_kPa is valid.
 * @retval STATUS_NULL_POINTER  P_kPa was NULL.
 * @retval STATUS_INVALID_VALUE elevation_m out of range, or the
 *                              result was non-finite or non-positive.
 *
 * @note "The effect is, however, small and in the calculation
 *       procedures, the average value for a location is sufficient"
 *       (FAO-56, p. 31).
 */
Status Calc_PressureFromElevation(double elevation_m, double *P_kPa);
```

* * *

### `psychrometric-calc.h`

```C
/**
 * @brief Initializes atmospheric parameter data to a safe zero state.
 *
 * Unlike the "Layer state" Init functions, this one also sets
 * `.initialized = true` immediately - the structure is usable right
 * after Init(), before any Calc_AtmosphericParameters() call.
 *
 * @param[out] data Pointer to the AtmosphericData structure to
 *                  initialize. Must not be NULL.
 *
 * @post On success, `.P_kPa` and `.gamma_kPa_per_C` are zero and
 *       `.initialized` is true.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status AtmosphericData_Init(AtmosphericData *data);

/**
 * @brief Computes psychrometric constant from pressure (FAO-56, eq. 8).
 *
 * gamma = 0.000665 * P. Takes P_kPa as a plain double and has no
 * knowledge of where it came from (sensor, elevation model, or
 * constant) - see data-flow-specification.md ("Derived intermediates").
 * P_kPa is stored alongside gamma_kPa_per_C purely so the pair stays
 * together for reporting.
 *
 * @param[in,out] out   Structure to update; must already be
 *                      initialized (see AtmosphericData_Init()).
 *                      Must not be NULL.
 * @param[in]     P_kPa Resolved atmospheric pressure [kPa]. Must be
 *                      finite and in [50, 120].
 *
 * @retval STATUS_OK            out->P_kPa and out->gamma_kPa_per_C
 *                              are valid.
 * @retval STATUS_NULL_POINTER  out was NULL.
 * @retval STATUS_INVALID_VALUE out was not initialized, P_kPa was out
 *                              of range or non-finite, or the computed
 *                              gamma was non-finite.
 */
Status Calc_AtmosphericParameters(AtmosphericData *out, double P_kPa);
```

* * *

### `day-in-year-calc.h`

```C
/**
 * @brief Initializes day/astronomy data to a safe zero state.
 *
 * @param[out] data Pointer to the DayData structure to initialize.
 *                  Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status DayCalc_Init(DayData* data);

/**
 * @brief Computes all astronomical day quantities for a day of year
 *        and latitude (FAO-56, eq. 23-25, 34).
 *
 * Inverse relative distance Earth-Sun dr (eq. 23), solar declination
 * delta (eq. 24, adapted Cooper's equation), sunset hour angle omega_s
 * (eq. 25), and daylight hours N (eq. 34). At polar latitudes, the eq. 25
 * arccos argument can fall outside [-1, 1]: this is handled, not rejected -
 * argument > 1 (polar night) yields omega_s = 0, N = 0; argument < -1
 * (polar day) yields omega_s = pi, N = 24.
 *
 * @param[in,out] data Structure to update. Must not be NULL.
 * @param[in]     J    Day of year. Must satisfy ValidDayOfYear().
 * @param[in]     loc  Location data. Must not be NULL;
 *                     `.initialized` must be true and
 *                     `.latitude_rad` must satisfy ValidLatitudeRad().
 *
 * @retval STATUS_OK            *data is valid for every latitude in
 *                              the accepted range, including the poles.
 * @retval STATUS_NULL_POINTER  data or loc was NULL.
 * @retval STATUS_INVALID_VALUE loc not initialized, J invalid, or
 *                              loc->latitude_rad invalid.
 */
Status DayCalc_Update(DayData* data, uint16_t J, const LocationData* loc);

/**
 * @brief Converts a calendar date to day-of-year, with leap-year
 *        correction.
 *
 * Pure function, no side effects. Does not validate its inputs - the
 * caller is responsible for passing a plausible date (no Status
 * return is available to report a problem).
 *
 * @param[in] day   Day of month.
 * @param[in] month Month (1-12).
 * @param[in] year  Calendar year, used only to test the leap-year rule.
 *
 * @return Day of year (1-366).
 */
uint16_t DayCalc_JFromDate(uint8_t day, uint8_t month, uint16_t year);
```

* * *

### `extrater-radiation-calc.h`

```C
/**
 * @brief Initializes extraterrestrial radiation data to a safe zero state.
 *
 * @param[out] data Pointer to the RaData structure to initialize.
 *                  Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status RaCalc_Init(RaData* data);

/**
 * @brief Computes daily extraterrestrial radiation (FAO-56, eq. 21).
 *
 * Ra = (24*60 / pi) * Gsc * dr * [omega_s*sin(phi)*sin(delta) +
 *      cos(phi)*cos(delta)*sin(omega_s)]
 *
 * During polar night (omega_s = 0, from DayCalc_Update()), both terms
 * vanish and Ra = 0 - no special case needed here.
 *
 * @param[out] out Destination for the result. Must not be NULL.
 * @param[in]  day Day/astronomy data. Must not be NULL;
 *                 `.initialized` must be true.
 * @param[in]  loc Location data. Must not be NULL; `.initialized`
 *                 must be true.
 *
 * @retval STATUS_OK            out->Ra_daily is valid.
 * @retval STATUS_NULL_POINTER  out, day, or loc was NULL.
 * @retval STATUS_INVALID_VALUE day or loc not initialized.
 */
Status Calc_Ra(RaData* out, const DayData* day, const LocationData* loc);
```

* * *

### `geolocation-calc.h`

```C
/**
 * @brief Converts a latitude from degrees-minutes to decimal degrees.
 *
 * The sign of @p degrees determines the hemisphere and is applied to
 * the combined 'degrees + minutes' magnitude (a negative @p minutes is
 * rejected, not treated as a second sign).
 *
 * @param[in]  degrees     Degrees component. Must be in [-90, 90].
 * @param[in]  minutes     Minutes component. Must be in [0, 60).
 * @param[out] decimal_deg Destination for the result [decimal
 *                         degrees]. Must not be NULL.
 *
 * @retval STATUS_OK            *decimal_deg is valid.
 * @retval STATUS_NULL_POINTER  decimal_deg was NULL.
 * @retval STATUS_INVALID_VALUE degrees or minutes out of range.
 *
 * @note   The sign of @p degrees determines the hemisphere:
 *         negative -> southern hemisphere.
 */
Status Location_DMS_to_decimal(double degrees, double minutes, double* decimal_deg);

/**
 * @brief Initializes location data from deployment configuration.
 *
 * Reads CONFIG_ELEVATION_M, CONFIG_LATITUDE_DEG, and
 * CONFIG_LATITUDE_MIN (deployment-config.h); converts
 * latitude via Location_DMS_to_decimal() and to radians.
 *
 * @param[out] loc Pointer to the LocationData structure to
 *                 initialize. Must not be NULL.
 *
 * @post On success, `.latitude_deg`, `.latitude_rad`, `.elevation_m`
 *       are set from configuration and `.initialized` is true.
 *
 * @retval STATUS_OK            Initialization succeeded.
 * @retval STATUS_NULL_POINTER  loc was NULL.
 * @retval STATUS_INVALID_VALUE Propagated verbatim from
 *                              Location_DMS_to_decimal(); in practice
 *                              unreachable with the current constants.
 */
Status Location_Init(LocationData* loc);
```

* * *

### `net-radiation-calc.h`

```C
/**
 * @brief Initializes net radiation data to a safe zero state.
 *
 * Unlike the "Layer state" Init functions, also sets
 * `.initialized = true` immediately.
 *
 * @param[out] data Pointer to the NetRadiationData structure to
 *                  initialize. Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status NetRadiation_Init(NetRadiationData *data);

/**
 * @brief Computes daily net radiation (FAO-56, eq. 38-40).
 *
 * Eq. 38: Rns = (1 - α) * Rs, α = 0.23 for hypothetical grass cover.
 * Eq. 39: Rnl = sigma * [(Tmax,K^4 + Tmin,K^4) / 2]
 *         * (0.34 - 0.14 * sqrt(ea)) * (1.35 * Rs/Rso - 0.35).
 * Eq. 40: Rn  = Rns - Rnl.
 *
 * T is converted to Kelvin with FAO-56's eq. 39 constant, 273.16
 * (not 273.15). Rs/Rso is clamped to <= 1.0 and computed as 0
 * (not a division by zero) when Rso = 0 (polar night).
 * The cloudiness factor `1.35 * Rs/Rso - 0.35` is clamped to >= 0,
 * since a very overcast sky would otherwise make it negative,
 * which is not physically meaningful for Rnl.
 *
 * @param[out] out    Destination for the result; must already be
 *                    initialized. Must not be NULL.
 * @param[in]  temp   Accumulated daily temperature data. Must not be
 *                    NULL; `.initialized` true.
 * @param[in]  solar  Solar radiation data. Must not be NULL;
 *                    `.initialized` true, Rs_daily/Rso_daily >= 0.
 * @param[in]  ea_kPa Actual vapour pressure [kPa]. Must be finite
 *                    and >= 0.
 *
 * @retval STATUS_OK            out->Rns_daily, out->Rnl_daily,
 *                              out->Rn_daily are valid.
 * @retval STATUS_NULL_POINTER  out, temp, or solar was NULL.
 * @retval STATUS_INVALID_VALUE Any structure not initialized, ea_kPa
 *                              out of range, Rs_daily/Rso_daily
 *                              negative, or a result was non-finite.
 */
Status Calc_NetRadiation(NetRadiationData *out,
    const AirTemperatureData *temp, const SolarRadiationData *solar, double ea_kPa);
```

* * *

### `solar-radiation-calc.h`

```C
/**
 * @brief Sets Angstrom-Prescott coefficients to their FAO-56 default
 *        values (a_s = 0.25, b_s = 0.50).
 *
 * @param[out] ang Destination structure. Must not be NULL.
 *
 * @retval STATUS_OK           *ang is valid.
 * @retval STATUS_NULL_POINTER ang was NULL.
 */
Status AngstromValues_Default(AngstromValues* ang);

/**
 * @brief Initializes solar radiation data to a safe zero state.
 *
 * Unlike the "Layer state" Init functions, also sets
 * `.initialized = true` immediately.
 *
 * @param[out] data Pointer to the SolarRadiationData structure to
 *                  initialize. Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status SolarRadiation_Init(SolarRadiationData* data);

/**
 * @brief Computes daily solar and clear-sky radiation (FAO-56, eq. 35, 37).
 *
 * Eq. 35: Rs  = (a_s + b_s * n/N) * Ra.
 * Eq. 37: Rso = (0.75 + 2e-5 * z) * Ra.
 *
 * n/N is clamped to [0, 1] (actual sunshine duration cannot exceed
 * the possible daylight duration). During polar night (N = 0),
 * returns STATUS_OK early with Rs = Rso = 0, bypassing
 * the eq. 35/37 arithmetic entirely.
 *
 * @param[in]  ang      Angstrom-Prescott coefficients. Must not be
 *                      NULL; a_s >= 0, b_s >= 0, a_s + b_s <= 1.
 * @param[out] out      Destination for the result; must already be
 *                      initialized. Must not be NULL.
 * @param[in]  ra       Extraterrestrial radiation. Must not be NULL;
 *                      `.initialized` true, `.Ra_daily` >= 0.
 * @param[in]  day      Day/astronomy data. Must not be NULL;
 *                      `.initialized` true, `.N_hours` >= 0.
 * @param[in]  sunshine Accumulated sunshine data. Must not be NULL;
 *                      `.initialized` true, `.n_hours` >= 0.
 * @param[in]  loc      Location data (for elevation z). Must not be
 *                      NULL; `.initialized` true.
 *
 * @retval STATUS_OK            out->Rs_daily and out->Rso_daily are
 *                              valid (including the polar-night
 *                              zero case).
 * @retval STATUS_NULL_POINTER  Any parameter was NULL.
 * @retval STATUS_INVALID_VALUE Any structure not initialized, ang out
 *                              of range, any of Ra_daily / N_hours /
 *                              n_hours negative, or the computed
 *                              Rs/Rso was non-finite or negative.
 */
Status SolarRadiation_Calc(const AngstromValues* ang, SolarRadiationData* out,
    const RaData* ra, const DayData* day, const SunshineLuxData* sunshine, const LocationData* loc);
```

* * *

### `sunshine-lux-calc.h`

```C
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
```

Static helper in **`sunshine-lux-calc.c`**:

```C
/**
 * @brief Classifies one illuminance sample as bright or not.
 *
 * Internal to this file; no NULL check on @p data - callers within
 * this file only invoke it with an already-validated pointer. The
 * comparison logic lives in exactly one place so that a future
 * calibration change (e.g. hysteresis) touches only this function.
 *
 * @param[in] data Accumulator holding the configured threshold.
 * @param[in] lux  Sample to classify [lux].
 *
 * @return true iff `lux >= data->threshold_lux`.
 *
 * @note   On the illuminance threshold, see illuminance-proxy.md.
 */
static bool SunshineLux_IsBright(const SunshineLuxData* data, const double lux);
```

* * *

### `wind-speed-calc.h`

```C
/**
 * @brief Initializes wind speed data to a safe zero state.
 *
 * @param[out] data Pointer to the WindSpeedData structure to
 *                  initialize. Must not be NULL.
 *
 * @retval STATUS_OK           Initialization succeeded.
 * @retval STATUS_NULL_POINTER data was NULL.
 */
Status WindSpeed_Init(WindSpeedData *data);

/**
 * @brief Folds one instantaneous reading into the daily min/max/mean.
 *
 * Unlike the temperature/humidity accumulators, this one tracks a
 * true running arithmetic mean (`.u_z_mean_m_s = sum/count`), not a
 * range midpoint. The measurement height is fixed by the first
 * accepted sample; every later sample in the same run must match it
 * within `WIND_HEIGHT_TOL_M`, or the call is rejected - heights
 * cannot be mixed within one accumulation.
 *
 * @param[in,out] data          Accumulator to update. Must not be NULL.
 * @param[in]     speed_m_s     Instantaneous wind speed [m/s]. Must be
 *                              finite and in [0.0, 100.0].
 * @param[in]     height_m      Measurement height [m]. Must be finite
 *                              and in [0.1, 200.0]; must match the
 *                              height fixed by the first accepted
 *                              sample, if any.
 * @param[in]     timestamp     Currently unused (reserved for future
 *                              time-ordered sample handling).
 *
 * @retval STATUS_OK            Accepted; *data reflects the new reading.
 * @retval STATUS_NULL_POINTER  data was NULL.
 * @retval STATUS_INVALID_VALUE speed_m_s or height_m out of range, or
 *                              height_m differs from the fixed height
 *                              by more than WIND_HEIGHT_TOL_M;
 *                              *data unchanged.
 */
Status WindSpeed_Update(WindSpeedData *data, double speed_m_s, double height_m, uint32_t timestamp);

/**
 * @brief Converts a wind speed measured at height z to the FAO-56
 *        standard 2 m height (eq. 47).
 *
 * u2 = u_z * (4.87 / ln(67.8 * z - 5.42)).
 *
 * @param[in]  u_z    Wind speed at height @p z [m/s]. Must be finite
 *                    and in [0.0, 100.0].
 * @param[in]  z      Measurement height [m]. Must be finite and in
 *                    [0.1, 200.0]; must also keep `67.8 * z - 5.42 > 0`
 *                    (true for any z within that range).
 * @param[out] out_u2 Destination for the result [m/s]. Must not be
 *                    NULL.
 *
 * @retval STATUS_OK            *out_u2 is valid.
 * @retval STATUS_NULL_POINTER  out_u2 was NULL.
 * @retval STATUS_INVALID_VALUE u_z or z out of range, or the result
 *                              was non-finite.
 */
Status Calc_WindSpeedAt2m(double u_z, double z, double *out_u2);
```

Static helpers in **`wind-speed-calc.c`**:

```C
/**
 * @brief Checks whether a value is a physically plausible wind speed.
 *
 * Internal to this file, shared by WindSpeed_Update() and
 * Calc_WindSpeedAt2m().
 *
 * @param[in] u Candidate wind speed [m/s].
 *
 * @return true iff `isfinite(u) && 0.0 <= u <= 100.0`.
 */
static bool IsValidSpeed(const double u);

/**
 * @brief Checks whether a value is a plausible anemometer height.
 *
 * Internal to this file, shared by WindSpeed_Update() and
 * Calc_WindSpeedAt2m().
 *
 * @param[in] z Candidate height [m].
 *
 * @return true iff `isfinite(z) && 0.1 <= z <= 200.0`.
 */
static bool IsValidHeight(const double z);
```

* * *

### `eto-calc.h`

```C
/**
 * @brief Computes reference evapotranspiration by the FAO-56
 *        Penman-Monteith method (eq. 6).
 *
 * ETo = [0.408*delta*(Rn-G) + gamma*(900/(T+273))*u2*(es-ea)]
 *       / [delta + gamma*(1+0.34*u2)].
 *
 * All eight inputs are leaf values, already computed by earlier
 * RunDailyCycle() steps - this function does not read any sensor or
 * acquisition data itself; see data-flow-specification.md ("Final
 * outputs"). If @p ea_kpa exceeds @p es_kpa (RH would be over
 * 100%, a measurement artifact), ea is capped to es before use. The
 * result is clamped to >= 0 (a negative Rn, e.g. in winter, can
 * otherwise drive the numerator negative).
 *
 * @param[in]  delta_kpa_c    Slope of the saturation vapour pressure
 *                            curve [kPa/C] (eq. 13). Must be finite
 *                            and > 0.
 * @param[in]  Rn_mj_m2_day   Net radiation [MJ/m2/day] (eq. 40). Must
 *                            be finite; may be negative.
 * @param[in]  G_mj_m2_day    Daily soil heat flux [MJ/m2/day]; pass
 *                            ETO_G_DAILY_MJ_M2_DAY (0, per eq. 42) for
 *                            a daily time step. Must be finite.
 * @param[in]  gamma_kpa_c    Psychrometric constant [kPa/C] (eq. 8).
 *                            Must be finite and > 0.
 * @param[in]  T_mean_c       Mean daily air temperature [C]. Must be
 *                            finite.
 * @param[in]  u2_m_s         Wind speed at 2 m height [m/s] (eq. 47).
 *                            Must be finite and >= 0.
 * @param[in]  es_kpa         Saturation vapour pressure [kPa] (eq. 12).
 *                            Must be finite and > 0.
 * @param[in]  ea_kpa         Actual vapour pressure [kPa] (eq. 17).
 *                            Must be finite and >= 0.
 * @param[out] out_eto_mm_day Destination for the result [mm/day].
 *                            Always >= 0. Must not be NULL.
 *
 * @retval STATUS_OK            *out_eto_mm_day is valid.
 * @retval STATUS_NULL_POINTER  out_eto_mm_day was NULL.
 * @retval STATUS_INVALID_VALUE Any input was non-finite, or delta,
 *                              gamma, u2, or es was outside the
 *                              ranges listed above.
 */
Status Calc_ETo(double delta_kpa_c, double Rn_mj_m2_day, double G_mj_m2_day, double gamma_kpa_c,
    double T_mean_c, double u2_m_s, double es_kpa, double ea_kpa, double *out_eto_mm_day);

/**
 * @brief Computes crop evapotranspiration (FAO-56, eq. 56).
 *
 * ETc = Kc * ETo.
 *
 * @param[in]  eto_mm_day     Reference evapotranspiration, from
 *                            Calc_ETo() [mm/day]. Must be finite
 *                            and >= 0.
 * @param[in]  kc             Crop coefficient (Type A, from
 *                            deployment-config.h). Must be finite
 *                            and > 0.
 * @param[out] out_etc_mm_day Destination for the result [mm/day].
 *                            Must not be NULL.
 *
 * @retval STATUS_OK            *out_etc_mm_day is valid.
 * @retval STATUS_NULL_POINTER  out_etc_mm_day was NULL.
 * @retval STATUS_INVALID_VALUE eto_mm_day or kc was non-finite or
 *                              out of range.
 */
Status Calc_ETc(double eto_mm_day, double kc, double *out_etc_mm_day);
```

* * *

## `05-orchestration`

### `daily-cycle.h`

```C
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
```

Static helpers in **`daily-cycle.c`** (internal formatting details):

```C
/** @internal @brief Prints a section header (`=== title ===`) to stdout. */
static void PrintSectionHeader(const char *title);

/** @internal @brief Prints one label/value/unit row to stdout, right-aligned. */
static void PrintLabeledDouble(const char *label, double value, int precision, const char *unit, int unit_width);

/** @internal @brief Prints one label/value/unit row to stdout, for an unsigned integer. */
static void PrintLabeledUint(const char *label, unsigned int value, const char *unit, int unit_width);
```

* * *

### `main.c`

```C
/**
 * @brief Reports a RunDailyCycle() failure and produces the process
 *        exit code.
 *
 * Prints whatever acquisition diagnostics were captured before the
 * failure (via PrintTrace()), then a one-line failure message to
 * stderr naming the failed step and its status.
 *
 * @param[in] results     Partially filled results from the failed
 *                        run; only results->trace is read. Must not
 *                        be NULL.
 * @param[in] failed_step Name of the step that failed, as set by
 *                        RunDailyCycle() via out_failed_step. Must
 *                        not be NULL.
 * @param[in] status      The Status value returned by RunDailyCycle().
 *
 * @return 1, unconditionally (used directly as the process exit code).
 */
static int PrintStatusAndReturn(const DailyResults *results, const char *failed_step, const Status status);

/**
 * @brief Program entry point: runs one daily cycle and reports the outcome.
 *
 * Calls RunDailyCycle() exactly once. On success, prints the full
 * report via PrintReport(). On failure, reports it via
 * PrintStatusAndReturn().
 *
 * @return 0 on success (STATUS_OK); 1 if RunDailyCycle() failed.
 */
int main(void);
```

* * *

## Related documents

* [`Software architecture diagram`](software-architecture-diagram.md).  
* [`Verified call graph`](verified-call-graph.md).  
* [`Data flow specification`](data-flow-specification.md).  
* `Doxygen source documentation` (link to be added).  
* `Illuminance as a proxy` (link to be added).
* [`Issues v0.1.x`](issues-v01x.md).
