# Issues: v0.1.x

## Scope

Findings from reading the code, beyond the FAO-56 reference test suite and the static/dynamic checks listed under “Code quality” in the project [`README`](../README.md). Each item is either deferred to v0.2.x (MCU port) with an explicit reason, or is independent of the port and can be addressed on its own schedule. This document complements “Limitations and open questions of v0.1.x” in the `README`.

* * *

## 1. `e_tmean` is computed but not propagated

* Location: `Code/04-calculation/043-vapour-pressure-calc`. Computed by `Calc_SaturationVapourPressure()` from `temperature_data.T_mean_C`, consumed only by `PrintReport()`.  
* The ETo calculation uses `e_s` (from `Calc_MeanSaturationVapourPressure()`), not `e_tmean`. The underlying formula is independently exercised by `test_AirTemperature_NormalPath_T20` in `main-test.c`; this item is about an unused field in the production data flow, not an unverified calculation.  
* Status: no functional impact. Either remove the field from `DailyResults`, or document explicitly that it is diagnostic-only.  
* See also [`Data flow specification`](data-flow-specification.md).

* * *

## 2. `double` precision margin relative to the target FPU

* Location: throughout `Code/`.  
* First measurement on target: one full `RunDailyCycle()` on an STM32F446RE at 16 MHz took 77,749 CPU cycles (about 4.9 ms; preliminary, not yet measured with an optimized build). With a daily cadence this does not justify a `float` migration on performance grounds.  
* Status: open. The remaining questions are code size, portability to targets without an FPU, and a comparison with a `float` build; see [`devlog20`](Devjournal/Devlogs/devlog20-mcu-smoke-test.md).

* * *

## 3. Time zone dependency of `DateProvider_Read`

* Location: `Code/02-providers/021-date-provider/date-provider.c`.  
* `localtime()` resolves the current date using the host’s/runtime’s configured local time zone. Day-of-year and solar declination calculations are date-sensitive at the midnight boundary.  
* Status: open design question for the RTC-based replacement planned for v0.2.x. The time-base semantics (UTC vs. local time, and any required time-zone/DST handling) should be defined explicitly rather than relying on host-runtime time-zone configuration that will not be implicitly available on the MCU.  
* The v0.2-dev smoke test uses a fixed date; this does not resolve the question above.

* * *

## 4. `CONFIG_BRIGHT_LUX_THRESHOLD` is an uncalibrated proxy for a different physical quantity

* Location: `Code/02-providers/022-configurations/deployment-config.h`; consumed by `SolarRadiation_Calc()` via `SunshineLux_*`.  
* Illuminance (lux) is not the radiometric quantity (W/m^2) WMO’s sunshine-duration [`definition`](https://library.wmo.int/idviewer/68695/358) uses. Known, documented gap, not an unexamined one — full analysis, citations, and current configuration in [`illuminance-proxy.md`](illuminance-proxy.md).  
* Status: open. A calibration experiment with two candidate algorithms (astronomy-aware dynamic threshold, variance-based discriminator) is in progress for v0.2.x.

* * *

## Related documents

* [`Verified call graph`](verified-call-graph.md).  
* [`Data flow specification`](data-flow-specification.md).  
* [`Contracts and conventions`](contracts-and-conventions.md).  
* [`Doxygen source documentation`](https://cloclacordis.github.io/FieldEdge-Evapotranspiration/files.html).  
* [`Software architecture diagram`](software-architecture-diagram.md).  
* [`Illuminance as a proxy`](illuminance-proxy.md).  
* [`README`](../README.md).
