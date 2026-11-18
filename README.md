# EvapoCore

[![Host CI](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/build-and-test.yml) [![MCU Build](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/mcu-build.yml/badge.svg)](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/mcu-build.yml) [![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

## About

**EvapoCore** is a portable C11 computation core for calculating reference evapotranspiration using the FAO-56 Penman–Monteith method. It is intended for integration into irrigation control and decision-support systems.

The repository includes a host-side application and test suite using emulated sensor data, plus an initial bare-metal validation on the STM32F446RE. It also includes a bare-metal RTC driver. Physical meteorological sensors are not integrated.

The computation core is independent of MCU- and RTOS-specific code and can be integrated into bare-metal or RTOS-based applications. A separate Zephyr RTOS 4.5 STM32 firmware application is maintained in [`Evapotranspiration Firmware`](https://github.com/cloclacordis/evapotranspiration-firmware).

* * *

## Build and run

Requirements: CMake >= 3.28.3 and a C11-compatible compiler. The first configure may require network access to fetch Unity.

```bash
cmake -S Code -B build
cmake --build build
./build/fao56_app
ctest --test-dir build --output-on-failure
```

`fao56_app` runs the PC computation kernel. The build produces two test executables: `fao56_test` (58 FAO-56 tests) and `fao56_time_test` (calendar and time-provider tests). CTest runs both registered test suites.

To run the FAO-56 computation suite with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S Code -B build-sanitize -DENABLE_SANITIZERS=ON
cmake --build build-sanitize --target fao56_test
./build-sanitize/fao56_test
```

### STM32F446RE bare-metal validation

Requirements: CMake >= 3.28.3, `arm-none-eabi-gcc` with newlib-nano.

```bash
cmake -S Port/STM32F446RE -B build-mcu
cmake --build build-mcu
```

Flashing and debugging with OpenOCD and GDB: [`Docs/flashing-routine.md`](Docs/flashing-routine.md).  
RTC commissioning and time-setting: [`Docs/rtc-commissioning.md`](Docs/rtc-commissioning.md).

* * *

## System context

The intended system acquires agrometeorological data, validates measurements, and calculates ETo for use by an external irrigation system. The current host-side application uses emulated sensor inputs. An initial bare-metal validation has been performed on the STM32F446RE.

See [`Docs/system-context-diagram.md`](Docs/system-context-diagram.md).

* * *

## Architecture

The software uses layers with strictly unidirectional dependencies.

* [`Code/01-measurement`](Code/01-measurement): sensor reading (emulated on PC in v0.1.x).  
* [`Code/02-providers`](Code/02-providers): portable UTC timestamp interface, Gregorian calendar conversion, station date and deployment configuration. The PC and STM32 ports provide the time source separately.  
* [`Code/03-validation`](Code/03-validation): status codes, value ranges, data source tracking, and shared mathematical constants/utilities.  
* [`Code/04-calculation`](Code/04-calculation): computation modules (model constants, measured data, and computed data).  
* [`Code/05-orchestration`](Code/05-orchestration): [`daily-cycle.c`](Code/05-orchestration/daily-cycle.c) runs the measurement-and-calculation pipeline; [`main.c`](Code/05-orchestration/main.c) calls it and reports the result, including which step failed if the cycle does not complete.  
* [`Code/06-test`](Code/06-test): Unity test suite with [`reference values`](Code/06-test/test-config.h) from FAO-56 worked examples (for PC v0.1.x).  
* [`Port/`](Port): target-specific code used to build and run the computation core on an MCU. `Port/STM32F446RE` provides the initial bare-metal validation setup, including an RTC driver and supporting platform code.

The computation core in `Code/` remains independent of the target platform. A separate Zephyr RTOS 4.5 STM32 firmware application is maintained in [`Evapotranspiration Firmware`](https://github.com/cloclacordis/evapotranspiration-firmware).

See [`Docs/software-architecture-diagram.md`](Docs/software-architecture-diagram.md).

Deployment parameters (anemometer height, geographic coordinates, elevation, crop coefficient Kc, and sensor thresholds) are defined in [`Code/02-providers/022-configurations/deployment-config.h`](Code/02-providers/022-configurations/deployment-config.h). Model constants (λ, Stefan–Boltzmann σ, etc.) are defined locally within each computation module in [`Code/04-calculation`](Code/04-calculation). Shared mathematical constants used across files (π, degree/radian conversion) are defined in [`Code/03-validation/034-math-utils/math-utils.h`](Code/03-validation/034-math-utils/math-utils.h).

Public operations that can fail return a [`Status`](Code/03-validation/033-status) value. Output values are provided through out-parameters where applicable. Public functions accepting pointer arguments validate them before dereferencing. Floating-point inputs used by the calculation layer are checked for finiteness and, where applicable, valid ranges before use.

Accumulator structs (`AirTemperatureData`, `WindSpeedData`, etc.) carry an `initialized` flag that is `false` after `_Init()` and `true` after the first valid `_Update()`. Single-call result structs (`AtmosphericData`, `NetRadiationData`) are immediately usable after `_Init()`.

* * *

## Testing and verification

The FAO-56 computation suite (`fao56_test`) contains 58 tests verified against worked examples from FAO-56 [`1998`](https://www.fao.org/4/x0490e/x0490e00.htm) (see also [`2025`](https://agrhysmo.agr.unipi.it/wp-content/uploads/2025/09/FAO56%202025.pdf)). The separate `fao56_time_test` suite covers calendar conversion, Unix timestamp limits, leap years, station UTC offset, and unavailable time sources.

Run both test suites with:

```bash
ctest --test-dir build --output-on-failure
```

Reference values and tolerances for the FAO-56 computation suite are documented in [`Code/06-test/test-config.h`](Code/06-test/test-config.h).

* * *

## Code quality

Beyond the FAO-56 reference tests, the codebase underwent a deliberate hardening pass before this release.

* The kernel is compiled with the following flags in every build (host app, tests, and the kernel library of the MCU firmware): `-Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror`.  
* C11 standard compliance is strictly enforced, with extensions disabled for `fao56_app` and for the kernel library of `fao56_mcu`: `C_STANDARD_REQUIRED ON`, `C_EXTENSIONS OFF`.  
* The codebase was checked with the `cppcheck` static analyzer.  
* The PC test binary was checked with Valgrind (`--leak-check=full --track-origins=yes`), AddressSanitizer, and UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`) to detect memory errors, leaks, and undefined behavior.  
* Numeric inputs to the calculation layer are validated for `NaN` and infinite values and, where applicable, against valid ranges before use.  
* CI runs host builds and tests, including AddressSanitizer and UndefinedBehaviorSanitizer checks, on every push and pull request. The MCU workflow cross-compiles the STM32 firmware in Debug and Release configurations when `Code/`, `Port/`, or the MCU workflow changes.

* * *

## Limitations and open questions

* Sensor inputs are currently emulated using fixed constants; no physical environmental sensors are integrated yet.  
* The PC time provider obtains UTC Unix timestamps from `time()`; the STM32 port obtains them from the on-chip RTC. The portable computation kernel does not depend on `time_t`, `localtime()`, RTC registers, or the host time zone. The station date uses the fixed `CONFIG_STATION_UTC_OFFSET_MIN`; daylight-saving transitions are not implemented. The RTC requires a functioning LSE source and explicit time-setting after loss of the backup-domain state. It is not synchronized automatically and does not compensate oscillator drift. See [`RTC commissioning`](Docs/rtc-commissioning.md).  
* Measurement and calculation state is not persisted between runs. Persistent storage of application data in EEPROM or Flash is not implemented.  
* The pipeline computes a single daily cycle per run; the sampling model (some sensors read once, illuminance read on a fixed interval) is a PC-development convenience and will be unified into one periodic model once real-time sampling on the MCU is designed.  
* All computation uses `double` throughout, though the target MCUs (Arm Cortex-M4F) only have single-precision hardware floating point support, so double-precision arithmetic runs in software. This is a deliberate choice: accuracy took priority over speed for a value computed once per day, and the FAO-56 reference values were validated at `double` precision. A first measurement on an STM32F446RE (16 MHz, one full `RunDailyCycle()`, report output excluded) took 77,749 CPU cycles (about 4.9 ms; preliminary, not yet measured with an optimized build). Speed is therefore not currently a reason to change this. The decision remains open for code-size and portability reasons (e.g. targets without an FPU) and pending a comparison with a `float` build.  
* Sensor fallback values (`Sensor*_ReadDefault()`) are fixed constants, not FAO-56’s climatic-data-estimation procedures (e.g. Hargreaves-based Rs, RH from Tmin) — adequate for occasional sensor failure, but not a substitute for a genuinely missing sensor. Implementing FAO-56 procedures for estimating missing meteorological data remains future work.  
* The illuminance-based sunshine-duration threshold is an uncalibrated placeholder for WMO’s radiometric [`definition`](https://library.wmo.int/idviewer/68695/358) of sunshine duration — lux and W/m^2 are different physical quantities. This is a known, bounded gap: low-cost proxy sensors have published precedent in agricultural/meteorological IoT, and an experimental fix — a dynamic, astronomy-aware threshold with a calibrated algorithm — remains future calibration work. See [`Docs/illuminance-proxy.md`](Docs/illuminance-proxy.md). In addition, swapping in a pyranometer requires no change to the calculation layer.

Additional findings are tracked in [`Docs/issues-v01x.md`](Docs/issues-v01x.md).

* * *

## Porting or extending

A port is a directory under [`Port/`](Port) that supplies everything platform-specific: startup code, linker script, device headers, drivers, a `main()`, and a time source. The computation kernel in [`Code/`](Code) is compiled unmodified; use [`Port/STM32F446RE`](Port/STM32F446RE) as the reference. [`Code/04-calculation`](Code/04-calculation) has no dependency on [`Code/01-measurement`](Code/01-measurement) in either direction (see [`Software architecture diagram`](Docs/software-architecture-diagram.md)). Currently `Code/01-measurement` contains PC mock sensors. Physical sensor integration remains future work. Before modifying anything, read [`Verified call graph`](Docs/verified-call-graph.md) and [`Data flow specification`](Docs/data-flow-specification.md). See also “Limitations and open questions of v0.1.x” above.

The computation core can be reused in additional bare-metal or RTOS-based applications. A separate Zephyr RTOS 4.5 STM32 firmware application is maintained in [`Evapotranspiration Firmware`](https://github.com/cloclacordis/evapotranspiration-firmware).

* * *

## Documentation

* [`System context diagram`](Docs/system-context-diagram.md).  
* [`Software architecture diagram`](Docs/software-architecture-diagram.md).  
* [`Verified call graph`](Docs/verified-call-graph.md).  
* [`Data flow specification`](Docs/data-flow-specification.md).  
* [`Contracts and conventions`](Docs/contracts-and-conventions.md).  
* [`Doxygen source documentation`](https://cloclacordis.github.io/FieldEdge-Evapotranspiration/files.html).  
* [`Illuminance as a proxy`](Docs/illuminance-proxy.md).  
* [`RTC commissioning`](Docs/rtc-commissioning.md).  
* [`Issues v0.1.x`](Docs/issues-v01x.md).

* * *

## License

This project is licensed under the [`GNU AGPL v3.0`](https://www.gnu.org/licenses/agpl-3.0); see the [`LICENSE`](LICENSE) file for details.  
Third-party components in [`Port/`](Port) remain under their original licenses; see the license headers in the files and the license files next to them.
