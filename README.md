# FieldEdge-Evapotranspiration

[![Host CI](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/build-and-test.yml/badge.svg)](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/build-and-test.yml) [![MCU Build](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/mcu-build.yml/badge.svg)](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/actions/workflows/mcu-build.yml) [![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)

## About

**FieldEdge-Evapotranspiration** is open-source software for calculating reference evapotranspiration (ETo) according to the FAO-56 Penman–Monteith methodology, designed for integration into irrigation control and decision-support systems. It comprises a hardware-agnostic C11 computation kernel and an STM32-based embedded implementation. The software aims to provide practical, accessible tools for local communities and small-scale agricultural users and is available for local deployment without reliance on proprietary platforms or centralized services.

**Computation kernel (v0.1.x)** — a C11 implementation of the ETo calculation pipeline. It is complete, tested, and runs on a PC using emulated sensor data. The computation kernel is designed to be reusable independently of the target hardware.

**Embedded implementation (v0.2.x under development)**. A first hardware smoke test has been completed: the calculation kernel ran on an STM32F446RET6 Nucleo-64 board and printed the full daily report over UART (see [`devlog20`](Docs/Devjournal/Devlogs/devlog20-mcu-smoke-test.md)). Active development continues on the [`v0.2-dev`](https://github.com/cloclacordis/FieldEdge-Evapotranspiration/tree/v0.2-dev) branch.

* * *

## Build and run

Requirements: CMake >= 3.31 and a C11-compatible compiler.

```bash
cmake -B build
cmake --build build
./build/fao56_app        # runs the PC computation kernel
./build/fao56_test       # runs the Unity test suite (58 tests)
```

To build with the same sanitizers used in CI.

```bash
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build
./build/fao56_test
```

### STM32F446RE firmware (v0.2.x, smoke test)

Requirements: CMake >= 3.31, `arm-none-eabi-gcc` with newlib-nano.

```bash
cmake -S Port/STM32F446RE -B build-mcu
cmake --build build-mcu
```

Flashing and debugging with OpenOCD and GDB: [`Port/STM32F446RE/flashing-routine.md`](Port/STM32F446RE/flashing-routine.md).

* * *

## System context

The system operates at the edge, acquiring agrometeorological data from sensors, processing and validating the measurements, and calculating ETo. The resulting ETo value is provided to an external irrigation system for local management. Version v0.1.x implements the measurement and computation pipeline on a PC using emulated sensor data. Version v0.2.x is an STM32 implementation under development.

See [`Docs/system-context-diagram.md`](Docs/system-context-diagram.md).

* * *

## Architecture

The software uses layers with strictly unidirectional dependencies.

* [`Code/01-measurement`](Code/01-measurement): sensor reading (emulated on PC in v0.1.x).  
* [`Code/02-providers`](Code/02-providers): date/time and deployment configuration (system time is used on PC in v0.1.x).  
* [`Code/03-validation`](Code/03-validation): status codes, value ranges, data source tracking, and shared mathematical constants/utilities.  
* [`Code/04-calculation`](Code/04-calculation): computation modules (model constants, measured data, and computed data).  
* [`Code/05-orchestration`](Code/05-orchestration): [`daily-cycle.c`](Code/05-orchestration/daily-cycle.c) runs the measurement-and-calculation pipeline; [`main.c`](Code/05-orchestration/main.c) calls it and reports the result, including which step failed if the cycle does not complete.  
* [`Code/06-test`](Code/06-test): Unity test suite with [`reference values`](Code/06-test/test-config.h) from FAO-56 worked examples (for PC v0.1.x).  
* [`Port/`](Port): platform-specific code for target hardware (currently [`STM32F446RE`](Port/STM32F446RE)); it is not part of the kernel — the kernel in `Code/` is built into it unmodified.

The calculation layer is a pure computation kernel. The computation kernel has no knowledge of sensors: it receives validated inputs and returns results. Integration with sensors occurs only in the orchestration layer, specifically in the daily cycle routine (in v0.1.x, sensor readings are provided by PC-based emulation).

See [`Docs/software-architecture-diagram.md`](Docs/software-architecture-diagram.md).

Deployment parameters (anemometer height, geographic coordinates, elevation, crop coefficient Kc, and sensor thresholds) are defined in [`Code/02-providers/022-configurations/deployment-config.h`](Code/02-providers/022-configurations/deployment-config.h). Model constants (λ, Stefan–Boltzmann σ, etc.) are defined locally within each computation module in [`Code/04-calculation/.../*-calc.c`](Code/04-calculation). Shared mathematical constants used across files (π, degree/radian conversion) are defined in [`Code/03-validation/034-math-utils/math-utils.h`](Code/03-validation/034-math-utils/math-utils.h).

All public functions return a [`Status`](Code/03-validation/033-status) value. Results are written to out-parameters. Pointer arguments are checked for `NULL` first in every function. Numeric inputs are checked for `NaN` and infinite values before any arithmetic operation.

Accumulator structs (`AirTemperatureData`, `WindSpeedData`, etc.) carry an `initialized` flag that is `false` after `_Init()` and `true` after the first valid `_Update()`. Single-call result structs (`AtmosphericData`, `NetRadiationData`) are immediately usable after `_Init()`.

* * *

## Testing and verification

```
58 Tests  0 Failures  0 Ignored
OK
```

All test cases are verified against worked examples from FAO-56 [`1998`](https://www.fao.org/4/x0490e/x0490e00.htm) (see also [`2025`](https://agrhysmo.agr.unipi.it/wp-content/uploads/2025/09/FAO56%202025.pdf)). Reference values and tolerances are documented in [`Code/06-test/test-config.h`](Code/06-test/test-config.h).

* * *

## Code quality

Beyond the FAO-56 reference tests, the codebase underwent a deliberate hardening pass before this release.

* The kernel is compiled with the following flags in every build (host app, tests, and the kernel library of the MCU firmware): `-Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror`.  
* C11 standard compliance is strictly enforced, with extensions disabled for `fao56_app` and for the kernel library of `fao56_mcu`: `C_STANDARD_REQUIRED ON`, `C_EXTENSIONS OFF`.  
* The codebase was [`checked`](Docs/Devjournal/Devlogs/devlog18-review-and-improvements-v010.md) with the `cppcheck` static analyzer.  
* The PC test binary was [`checked`](Docs/Devjournal/Devlogs/devlog19-checks-n-docs.md) with Valgrind (`--leak-check=full --track-origins=yes`), AddressSanitizer, and UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`) to detect memory errors, leaks, and undefined behavior.  
* Numeric inputs to the calculation layer are validated for `NaN` and infinite values and, where applicable, against valid ranges before use.  
* CI (GitHub Actions) builds all targets, runs the full test suite, runs the test suite with AddressSanitizer and UndefinedBehaviorSanitizer on every `push` and `pull request`, and cross-compiles the STM32 firmware (Debug and Release) with `arm-none-eabi-gcc`.

* * *

## Limitations and open questions of v0.1.x

* Sensor data is currently emulated using fixed constants; no real hardware is involved yet.  
* `time()` from `<time.h>` is used for the current day of year and illuminance measurement timestamps. The PC implementation causes an internal heap allocation in [`libc`](Docs/Devjournal/Devlogs/devlog19-checks-n-docs.md) when timezone information is initialized; this allocation is not performed by application code. When porting to an MCU, the PC time implementation will be replaced with RTC access, and the MCU implementation should be verified to ensure that it does not introduce dynamic memory allocation.  
* State is not persisted between runs (EEPROM/Flash persistence is planned for v0.2.x).  
* The pipeline computes a single daily cycle per run; the sampling model (some sensors read once, illuminance read on a fixed interval) is a PC-development convenience and will be unified into one periodic model once real-time sampling on the MCU is designed.  
* All computation uses `double` throughout, though the target MCUs (Arm Cortex-M4F) only have single-precision hardware floating point support, so double-precision arithmetic runs in software. This is a deliberate choice: accuracy took priority over speed for a value computed once per day, and the FAO-56 reference values were validated at `double` precision. A first measurement on an STM32F446RE (16 MHz, one full `RunDailyCycle()`, report output excluded) took 77,749 CPU cycles (about 4.9 ms; preliminary, not yet measured with an optimized build) — see [`devlog20`](Docs/Devjournal/Devlogs/devlog20-mcu-smoke-test.md). Speed is therefore not currently a reason to change this. The decision remains open for code-size and portability reasons (e.g. targets without an FPU) and pending a comparison with a `float` build.  
* Sensor fallback values (`Sensor*_ReadDefault()`) are fixed constants, not FAO-56’s climatic-data-estimation procedures (e.g. Hargreaves-based Rs, RH from Tmin) — adequate for occasional sensor failure, but not a substitute for a genuinely missing sensor. Proper estimation is planned alongside real sensor-driver development in v0.2.x, where the required cross-channel data access will exist “naturally”.  
* The illuminance-based sunshine-duration threshold is an uncalibrated placeholder for WMO’s radiometric [`definition`](https://library.wmo.int/idviewer/68695/358) of sunshine duration — lux and W/m^2 are different physical quantities. This is a known, bounded gap: low-cost proxy sensors have published precedent in agricultural/meteorological IoT, and an experimental fix — a dynamic, astronomy-aware threshold with a calibrated algorithm — is planned for v0.2.x. See [`Docs/illuminance-proxy.md`](Docs/illuminance-proxy.md). In addition, swapping in a pyranometer requires no change to the calculation layer.

Additional findings are tracked in [`Docs/issues-v01x.md`](Docs/issues-v01x.md).

* * *

## Porting or extending

A port is a directory under [`Port/`](Port) that supplies everything platform-specific: startup code, linker script, device headers, drivers, a `main()`, and a time source. The computation kernel in [`Code/`](Code) is compiled unmodified; use [`Port/STM32F446RE`](Port/STM32F446RE) as the reference. [`Code/04-calculation`](Code/04-calculation) has no dependency on [`Code/01-measurement`](Code/01-measurement) in either direction (see [`Software architecture diagram`](Docs/software-architecture-diagram.md)). Currently `Code/01-measurement` contains PC mock sensors; replacing them with real drivers is planned for v0.2.x. Before modifying anything, read [`Verified call graph`](Docs/verified-call-graph.md) and [`Data flow specification`](Docs/data-flow-specification.md). See also “Limitations and open questions of v0.1.x” above.

* * *

## Documentation

* [`System context diagram`](Docs/system-context-diagram.md).  
* [`Software architecture diagram`](Docs/software-architecture-diagram.md).  
* [`Verified call graph`](Docs/verified-call-graph.md).  
* [`Data flow specification`](Docs/data-flow-specification.md).  
* [`Contracts and conventions`](Docs/contracts-and-conventions.md).  
* [`Doxygen source documentation`](https://cloclacordis.github.io/FieldEdge-Evapotranspiration/files.html).  
* [`Illuminance as a proxy`](Docs/illuminance-proxy.md).  
* [`Issues v0.1.x`](Docs/issues-v01x.md).

* * *

## Development journal

Step-by-step development notes (in Russian) are in [`Docs/Devjournal/Devlogs`](Docs/Devjournal/Devlogs).  
See [`Docs/Devjournal/Disclaimer.md`](Docs/Devjournal/Disclaimer.md) for context on the format and purpose of those notes.  
See [`Docs/Devjournal/Index.md`](Docs/Devjournal/Index.md) for the devlog table of contents and English synopses.

* * *

## License

This project is licensed under the [`GNU AGPL v3.0`](https://www.gnu.org/licenses/agpl-3.0); see the [`LICENSE`](LICENSE) file for details.  
Third-party components in [`Port/`](Port) remain under their original licenses; see the license headers in the files and the license files next to them.
