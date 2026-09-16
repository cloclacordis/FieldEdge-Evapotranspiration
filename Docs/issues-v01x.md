# Issues: v0.1.x

## Scope

Findings from reading the code, beyond the FAO-56 reference test suite and the static/dynamic checks listed under “Code quality” in the project `README`. Each item is either deferred to v0.2.x (MCU port) with an explicit reason, or is independent of the port and can be addressed on its own schedule. This document complements “Limitations and open questions of v0.1.x” in the `README`.

* * *

## 1. `pressure_sample.source` may read indeterminate memory

* Location: `Code/05-orchestration/daily-cycle.c`, pressure acquisition branch. Root cause: `Code/05-orchestration/main.c:15` (`DailyResults results;`, declared without an initializer).  
* If `SensorPressure_ReadInstant()` fails while `Calc_PressureFromElevation()` (the elevation-model fallback) succeeds, `pressure_sample` is never written in that branch. `PrintReport()` later reads `pressure_sample.source` to label the pressure source as “sensor” or “model/constant”; in this branch, the field holds indeterminate stack memory rather than a defined value. Because `SENSOR_VALUE_MEASURED` is `0` (see `value-source.h`), a zero-valued leftover byte pattern would silently print “sensor” for a value that came from the model — no error, no abnormal `Status`, only a mislabeled report line.  
* Status: latent on the PC build. All `Sensor*_ReadInstant()` mock implementations return a non-`STATUS_OK` status only on a `NULL` output pointer, and every call site in `RunDailyCycle()` passes a valid pointer — so this branch is currently unreachable (see `verified-call-graph.md`, “Unverified branches”). It becomes live once a real pressure sensor driver can fail independently of the model. Recommended fix before enabling that driver: zero-initialize `results` in `main()`, or set `pressure_sample.source` explicitly on the model-fallback path.  
* Full derivation: `dataflow-specification.md`, Observations, item 2 (link to be added).

* * *

## 2. `e_tmean` is computed but not propagated

* Location: `Code/04-calculation/043-vapour-pressure-calc`. Computed by `Calc_SaturationVapourPressure()` from `temperature_data.T_mean_C`, consumed only by `PrintReport()`.  
* The ETo calculation uses `e_s` (from `Calc_MeanSaturationVapourPressure()`), not `e_tmean`. The underlying formula is independently exercised by `test_AirTemperature_NormalPath_T20` in `main-test.c`; this item is about an unused field in the production dataflow, not an unverified calculation.  
* Status: no functional impact. Either remove the field from `DailyResults`, or document explicitly that it is diagnostic-only.  
* Full derivation: `dataflow-specification.md`, Observations, item 1 (link to be added).

* * *

## 3. `double` precision margin relative to the target FPU

* Location: throughout `Code/04-calculation`.  
* Already tracked in the `README`’s “Limitations and open questions of v0.1.x” as a deliberate, revisit-later decision. Added here as supporting evidence: the existing tolerances, e.g. `TOL_KPA = 0.0001` in `Code/06-test/test-config.h`, are broadly compatible with the numerical resolution of single-precision float for values in the expected operating range. Since the STM32’s Cortex-M4F provides a single-precision FPv4-SP FPU, this supports the feasibility of a future float migration, but the migration should still be validated against actual value ranges and accumulated numerical error.  
* Status: open, per `README`, pending real data from the MCU port.

* * *

## 4. Time zone dependency of `DateProvider_Read`

* Location: `Code/02-providers/021-date-provider/date-provider.c`.  
* `localtime()` resolves the current date using the host’s/runtime’s configured local time zone. Day-of-year and solar declination calculations are date-sensitive at the midnight boundary.  
* Status: open design question for the RTC-based replacement planned for v0.2.x. The time-base semantics (UTC vs. local time, and any required time-zone/DST handling) should be defined explicitly rather than relying on host-runtime time-zone configuration that will not be implicitly available on the MCU.

* * *

## Related documents

* [`Verified call graph`](verified-call-graph.md).  
* `Dataflow specification` (link to be added).
