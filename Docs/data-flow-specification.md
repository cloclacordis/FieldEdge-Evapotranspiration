# Data flow specification

## Scope

This document specifies the data flow of the `RunDailyCycle()` daily measurement-and-calculation cycle, expressed as producer/consumer relationships over the fields of the [`DailyResults`](../Code/05-orchestration/daily-cycle.h) structure. It complements [`verified-call-graph.md`](verified-call-graph.md), which specifies control flow (call order) for the same function.

* * *

## Data model

`DailyResults` is a single struct, owned and fully populated by one call to `RunDailyCycle()`. It holds six categories of data:

1. **Acquisition samples** — raw sensor readings, one struct per channel.  
2. **Layer state** — per-channel running data, derived from an initialization call and updated from the corresponding acquisition sample.  
3. **Time and location context** — date, computed day-of-year, and static location constants.  
4. **Radiation chain** — radiation quantities feeding the evapotranspiration calculation.  
5. **Derived intermediates** — single-shot values and small composite results computed once from earlier layers.  
6. **Final outputs** — two final results, ETo and ETc.

**Note.** A **seventh field**, `trace`, is a cross-cutting **diagnostics** record written throughout the function rather than by a single step.

* * *

## Field reference

### Acquisition samples

| Field | Producer | Consumers | Note |
|---|---|---|---|
| `t_sample` | `SensorTemperature_ReadInstant`, or `SensorTemperature_ReadDefault` on failure | `AirTemperature_Update`; `PrintReport` (`.source`) | — |
| `humidity_sample` | `SensorHumidity_ReadInstant`, or `SensorHumidity_ReadDefault` on failure | `AirHumidity_Update` | — |
| `pressure_sample` | `SensorPressure_ReadInstant`; `SensorPressure_ReadDefault` only if both the sensor read and the elevation model fail | Assignment to `P_source_kPa`; `PrintReport` (`.source`) | — |
| `wind_sample` | `SensorWindSpeed_ReadInstant`, or `SensorWindSpeed_ReadDefault` on failure | `WindSpeed_Update`; `PrintReport` (`.source`) | — |
| `lux_sample` | `SensorLux_ReadInstant`, or `SensorLux_ReadDefault` on failure; written once per loop iteration (12 iterations) | `SunshineLux_Update` (same iteration); copied into `trace.lux_samples[i]` | Holds only the final iteration’s value after the loop; not read again after loop exit |

* * *

### Layer state

| Field | Producer `init` | Producer `update` | Consumers |
|---|---|---|---|
| `temperature_data` | `AirTemperature_Init` | `AirTemperature_Update`, from `t_sample` | `Calc_SaturationVapourPressure`, `Calc_MeanSaturationVapourPressure`, `Calc_SlopeDelta`, `Calc_ActualVapourPressure`, `Calc_NetRadiation`, `Calc_ETo`, `PrintReport` |
| `humidity_data` | `AirHumidity_Init` | `AirHumidity_Update`, from `humidity_sample` | `Calc_ActualVapourPressure`, `PrintReport` |
| `wind_data` | `WindSpeed_Init` | `WindSpeed_Update`, from `wind_sample` | `Calc_WindSpeedAt2m`, `PrintReport` |
| `sunshine_data` | `SunshineLux_Init`, `SunshineLux_ResetDay` | `SunshineLux_Update` (x12), `SunshineLux_FinalizeDay` | `SolarRadiation_Calc`, `PrintReport` |

**Note.** The “Layer state” groups functions implementing a stateful accumulator pattern: data structures are initialized once and then updated incrementally by dedicated `_Update()` functions, with an `initialized` flag indicating whether valid data has been accumulated. In the subsequent processing layer, `Calc_X()` functions are used instead; they are stateless and simply transform inputs into outputs without retaining data between calls.

* * *

### Time and location context

| Field | Producer | Consumers |
|---|---|---|
| `location` | `Location_Init` (static; no update step) | `Calc_PressureFromElevation`, `DayCalc_Update`, `Calc_Ra`, `SolarRadiation_Calc`, `PrintReport` |
| `date` | `DateProvider_Read` | `DayCalc_JFromDate` |
| `current_j` | `DayCalc_JFromDate`, from `date` | `DayCalc_Update`, `PrintReport` |
| `day_data` | `DayCalc_Init`, then `DayCalc_Update` from `current_j` and `location` | `Calc_Ra`, `SolarRadiation_Calc`, `PrintReport` |

* * *

### Radiation chain

| Field | Producer | Consumers |
|---|---|---|
| `ra_data` | `RaCalc_Init`, then `Calc_Ra` from `day_data`, `location` | `SolarRadiation_Calc`, `PrintReport` |
| `angstrom` | `AngstromValues_Default` | `SolarRadiation_Calc`, `PrintReport` |
| `solar_radiation` | `SolarRadiation_Init`, then `SolarRadiation_Calc` from `angstrom`, `ra_data`, `day_data`, `sunshine_data`, `location` | `Calc_NetRadiation`, `PrintReport` |
| `net_radiation` | `NetRadiation_Init`, then `Calc_NetRadiation` from `temperature_data`, `solar_radiation`, `ea_kpa` | `Calc_ETo`, `PrintReport` |

* * *

### Derived intermediates

| Field | Producer | Consumers | Note |
|---|---|---|---|
| `e_tmean` | `Calc_SaturationVapourPressure`, from `temperature_data.T_mean_C` | `PrintReport` only | See note 1 below |
| `e_s` | `Calc_MeanSaturationVapourPressure`, from `temperature_data` | `Calc_ETo`, `PrintReport` | — |
| `delta` | `Calc_SlopeDelta`, from `temperature_data` | `Calc_ETo`, `PrintReport` | — |
| `ea_kpa` | `Calc_ActualVapourPressure`, from `temperature_data`, `humidity_data` | `Calc_NetRadiation`, `Calc_ETo`, `PrintReport` | — |
| `P_source_kPa` | `pressure_sample.P_kPa`, or `Calc_PressureFromElevation` | `Calc_AtmosphericParameters` | Not printed directly; the reported value is `atmos_data.P_kPa`; two of three possible sources are plain field copies, not function results — see the `pressure_sample` row above |
| `atmos_data` | `AtmosphericData_Init`, then `Calc_AtmosphericParameters`, from `P_source_kPa` | `Calc_ETo`, `PrintReport` | See note 2 below |
| `u2` | `Calc_WindSpeedAt2m`, from `wind_data` | `Calc_ETo`, `PrintReport` | — |

**Note 1.** `e_tmean` is computed by `Calc_SaturationVapourPressure` from `temperature_data.T_mean_C`, but is not read by any subsequent calculation step; its only consumer is `PrintReport`. The final ETo calculation uses `e_s` (from `Calc_MeanSaturationVapourPressure`), not `e_tmean`. The formula itself is independently exercised by `test_AirTemperature_NormalPath_T20` in `main-test.c`, against `TEST_E_TMEAN_EXPECTED`; this observation is about an unused field in the production data flow, not an unverified calculation. See also [`Issues v0.1.x`](issues-v01x.md).

**Note 2.** `atmos_data` sits here, not under “Layer state”, despite having its own `_Init()`: it has no acquisition sample and no `_Update()` — it is a single-shot `Calc_AtmosphericParameters()` result, same as every other field in this section, and its `gamma_kPa_per_C` is one of `Calc_ETo`’s eight direct inputs. It is the one struct-typed member of this section; every other field here is a bare `double`. It stays a 2-field struct rather than two bare `double`s only “so that P and γ are kept as a pair” (`psychrometric-calc.h`).

* * *

### Final outputs

| Field | Producer | Consumers |
|---|---|---|
| `eto_mm_day` | `Calc_ETo`, from `delta`, `net_radiation.Rn_daily`, `atmos_data.gamma_kPa_per_C`, `temperature_data.T_mean_C`, `u2`, `e_s`, `ea_kpa` | `Calc_ETc`, `PrintReport` |
| `etc_mm_day` | `Calc_ETc`, from `eto_mm_day` | `PrintReport`; terminal, no further consumer in this codebase |

* * *

### Diagnostics

| Field | Producer | Consumers |
|---|---|---|
| `trace` | `RunDailyCycle()` directly — one assignment per acquisition step, plus `lux_samples[i]` in the measurement loop | `PrintTrace`, called from both `PrintReport` (success) and `PrintStatusAndReturn` (failure, `main.c`) |

* * *

## Data flow diagram

A view of the same data flow, grouped by the categories defined in “Data model” and “Field reference” above.

![](Devjournal/Devlogs/resources/1908-v01x-data-flow-diagram.png)

**Note.** The `trace` is omitted from the diagram — it is written by nearly every step.

* * *

## Related documents

* [`Software architecture diagram`](software-architecture-diagram.md).  
* [`Verified call graph`](verified-call-graph.md).  
* `Doxygen source documentation` (link to be added).  
* [`Issues v0.1.x`](issues-v01x.md).
