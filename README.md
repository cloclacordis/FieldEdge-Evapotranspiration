# FieldEdge-Evapotranspiration

## About

**FieldEdge-Evapotranspiration** is open-source software for calculating reference evapotranspiration (ETo) according to the FAO-56 Penman–Monteith methodology, designed for integration into irrigation control and decision-support systems. It comprises a hardware-agnostic C11 computation kernel and an STM32-based embedded implementation. The software aims to provide practical, accessible tools for local communities and small-scale agricultural users and is available for local deployment without reliance on proprietary platforms or centralized services.

**Computation kernel (v0.1.x)** — a C11 implementation of the ETo calculation pipeline. It is complete, tested, and runs on a PC using emulated sensor data. The computation kernel is designed to be reusable independently of the target hardware.

**Embedded implementation (v0.2.x under development)** will integrate the computation kernel into an STM32-based embedded system with real sensors, LoRa wireless communication, and real-time operation.

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

The calculation layer is a pure computation kernel. The computation kernel has no knowledge of sensors: it receives validated inputs and returns results. Integration with sensors occurs only in the orchestration layer, specifically in the daily cycle routine (in v0.1.x, sensor readings are provided by PC-based emulation).

Deployment parameters (anemometer height, geographic coordinates, elevation, crop coefficient Kc, and sensor thresholds) are defined in [`Code/02-providers/022-configurations/deployment-config.h`](Code/02-providers/022-configurations/deployment-config.h). Model constants (λ, Stefan–Boltzmann σ, etc.) are defined locally within each computation module in [`Code/04-calculation/.../*-calc.c`](Code/04-calculation). Shared mathematical constants used across files (π, degree/radian conversion) are defined in [`Code/03-validation/034-math-utils/math-utils.h`](Code/03-validation/034-math-utils/math-utils.h).

All public functions return a [`Status`](Code/03-validation/033-status) value. Results are written to out-parameters. Pointer arguments are checked for `NULL` first in every function. Numeric inputs are checked for `NaN` and infinite values before any arithmetic operation.

Accumulator structs (`AirTemperatureData`, `WindSpeedData`, etc.) carry an `initialized` flag that is `false` after `_Init()` and `true` after the first valid `_Update()`. Single-call result structs (`AtmosphericData`, `NetRadiationData`) are immediately usable after `_Init()`.

* * *

## Testing and verification

```
58 Tests  0 Failures  0 Ignored
OK
```

All test cases are verified against worked examples from FAO-56 ([1998](https://www.fao.org/4/x0490e/x0490e00.htm); see also [2025](https://agrhysmo.agr.unipi.it/wp-content/uploads/2025/09/FAO56%202025.pdf)).  
Reference values and tolerances are documented in [`Code/06-test/test-config.h`](Code/06-test/test-config.h).

* * *

## Code quality

Beyond the FAO-56 reference tests, the codebase underwent a deliberate hardening pass before this release.

* Both build targets compile with the following compiler flags: `-Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror`.  
* C11 standard compliance is strictly enforced, with extensions disabled for the MCU-bound `fao56_app` target: `C_STANDARD_REQUIRED ON`, `C_EXTENSIONS OFF`.  
* The codebase was [`checked`](Docs/Devjournal/Devlogs/devlog18-review-and-improvements-v010.md) with the `cppcheck` static analyzer.  
* The PC test binary was [`checked`](Docs/Devjournal/Devlogs/devlog19-checks-n-docs.md) with Valgrind (`--leak-check=full --track-origins=yes`), AddressSanitizer, and UndefinedBehaviorSanitizer (`-fsanitize=address,undefined`) to detect memory errors, leaks, and undefined behavior.  
* Numeric inputs to the calculation layer are validated for `NaN` and infinite values and, where applicable, against valid ranges before use.  
* CI (GitHub Actions) builds both targets, runs the full test suite, and runs the test suite with AddressSanitizer and UndefinedBehaviorSanitizer on every `push` and `pull request`.

* * *

## Limitations and open questions of v0.1.x

* Sensor data is currently emulated using fixed constants; no real hardware is involved yet.  
* `time()` from `<time.h>` is used for the current day of year and illuminance measurement timestamps. The PC implementation causes an internal heap allocation in [`libc`](Docs/Devjournal/Devlogs/devlog19-checks-n-docs.md) when timezone information is initialized; this allocation is not performed by application code. When porting to an MCU, the PC time implementation will be replaced with RTC access, and the MCU implementation should be verified to ensure that it does not introduce dynamic memory allocation.  
* State is not persisted between runs (EEPROM/Flash persistence is planned for v0.2.x).  
* The pipeline computes a single daily cycle per run; the sampling model (some sensors read once, illuminance read on a fixed interval) is a PC-development convenience and will be unified into one periodic model once real-time sampling on the MCU is designed.  
* All computation uses `double` throughout, though the target MCUs (Arm Cortex-M4F) only have single-precision hardware floating point support. This is a deliberate choice: accuracy took priority over speed for a value computed once per day, and the FAO-56 reference values were validated at `double` precision. This decision will be revisited when real timing data from the MCU port is available.  
* The illuminance-based sunshine-duration threshold is a preliminary estimate, not yet empirically calibrated against real hardware — planned for the sensor-driver development stage.

* * *

## Documentation (in progress)

* [`System context diagram`](Docs/system-context-diagram.md).

* * *

## Development journal

Step-by-step development notes (in Russian) are in [`Docs/Devjournal/Devlogs`](Docs/Devjournal/Devlogs).  
See [`Docs/Devjournal/Disclaimer.md`](Docs/Devjournal/Disclaimer.md) for context on the format and purpose of those notes.  
See [`Docs/Devjournal/Index.md`](Docs/Devjournal/Index.md) for the devlog table of contents and English synopses.

* * *

## License

This project is licensed under the [`GNU AGPL v3.0`](https://www.gnu.org/licenses/agpl-3.0).  
See the [`LICENSE`](LICENSE) file for details.
