# FieldEdge-Evapotranspiration

FieldEdge-Evapotranspiration is a portable C11 implementation of the FAO-56 [Penman–Monteith](https://en.wikipedia.org/wiki/Penman%E2%80%93Monteith_equation) reference evapotranspiration equation (Allen et al., [1998](https://www.fao.org/4/x0490e/x0490e00.htm); [2025](https://agrhysmo.agr.unipi.it/wp-content/uploads/2025/09/FAO56%202025.pdf)), built as a layered computational pipeline in which sensor reading, validation, calculation, and orchestration are isolated behind explicit interfaces.

The current release, **v0.1.0**, executes that full pipeline on a PC against mock/emulated sensor data, with every calculation checked line-by-line against FAO-56’s own worked examples.

The target is a self-contained field node — a microcontroller reading its own temperature, humidity, wind, pressure, and illuminance sensors and computing daily reference and crop evapotranspiration locally, with no cloud service, no stable network connection, and no proprietary platform. It is intended as open-source, non-commercial infrastructure — something small-scale agricultural users and local communities can run and modify themselves, not a product they depend on someone else to keep online.

* * *

## Architecture (in outlines)

The project uses layers with strictly unidirectional dependencies.

* `Code/01-measurement`: sensor reading (emulated on PC, drivers on MCU).  
* `Code/02-providers`: date/time, deployment configuration.  
* `Code/03-validation`: status codes, value ranges, data source tracking, shared math constants/utils.  
* `Code/04-calculation`: computation modules (model constants, measured and computed data).  
* `Code/05-orchestration`: `daily-cycle.c` runs the measurement-and-calculation pipeline; `main.c` is a thin entry point that calls it and reports the result, including which step failed if the cycle doesn’t complete.  
* `Code/06-test`: Unity test suite, reference values from FAO-56 worked examples.

The calculation layer is a pure computation kernel: it has no knowledge of sensors, time, or fallback logic — it receives validated inputs and returns results; the connection happens only in the orchestration layer.

Deployment parameters (anemometer height, geographic coordinates, elevation, crop coefficient *Kc*, sensor thresholds) live in `Code/02-providers/022-configurations/deployment-config.h`. Model constants (λ, Stefan-Boltzmann σ, etc.) are defined locally inside each computation module in `Code/04-calculation/.../*-calc.c`; portable math constants shared across files (π, degree/radian conversion) live in `Code/03-validation/034-math-utils/math-utils.h`.

All public functions return a `Status` value. Results are written to out-parameters. NULL is checked first in every function. Every numeric input is checked for `NaN`/`Infinity` before any arithmetic operation.

Accumulator structs (`AirTemperatureData`, `WindSpeedData`, etc.) carry an `initialized` flag that is `false` after `_Init()` and `true` after the first valid `_Update()`. Single-call result structs (`AtmosphericData`, `NetRadiationData`) are ready immediately after `_Init()`.

> More detailed architecture documentation (diagrams, function-level contracts) is planned for after this release.

* * *

## Code quality and verification

Beyond the FAO-56 reference tests below, the codebase underwent a deliberate hardening pass before this release:

* Both build targets compile with the following compiler flags: `-Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror`.  
* The C11 standard compliance is strictly enforced, extensions are disabled (for the `fao56_app` target — the code intended for MCU deployment): `C_STANDARD_REQUIRED ON`, `C_EXTENSIONS OFF`.  
* The codebase was checked with the `cppcheck` static analyzer.  
* Every numeric input to the calculation layer is validated for range and for `NaN`/`Infinity` before use.  
* CI (GitHub Actions) builds both targets and runs the full test suite on every push and pull request.

* * *

## Test results

```
58 Tests  0 Failures  0 Ignored
OK
```

All test cases are verified against worked examples from FAO-56 (1998).  
Reference values and tolerances are documented in `Code/06-test/test-config.h`.

* * *

## Limitations and open questions of v0.1.0

* Sensors are mock constants; no real hardware is involved yet.  
* `time()` from `<time.h>` is used for the current day of year and lux timestamps; on a bare MCU, this requires RTC integration.  
* State is not persisted between runs (EEPROM/Flash persistence is planned for v0.2.0).  
* The pipeline computes a single daily cycle per run; the sampling model (some sensors read once, illuminance read on a fixed interval) is a PC-development convenience and will be unified into one periodic model once real-time sampling on the MCU is designed.  
* All computation uses `double` throughout, though the target MCUs (Arm Cortex-M4F) only have single-precision hardware floating point. This is a deliberate choice: accuracy took priority over speed for a value computed once per day, and the FAO-56 reference values were validated at `double` precision. This decision will be revisited when real timing data from the MCU port is available.  
* The illuminance-based sunshine-duration threshold is a preliminary estimate, not yet empirically calibrated against real hardware — planned for the sensor-driver development stage.

* * *

## Status and roadmap

* **v0.1.0 (current)** — the full computational core, validated on PC against FAO-56 worked examples, 58 passing tests, hardened through a comprehensive review (compiler warnings, static analysis, `NaN`/range checks on every input).  
* **v0.2.0 (next)** — port to STM32: real sensor drivers, an RTC-backed time source, and a periodic sampling model.  
* **v0.3.0** — LoRaWAN telemetry and field deployment.

* * *

## Development journal

Step-by-step development notes (in Russian) are in `Docs/Devjournal/Devlogs/`.  
See `Docs/Devjournal/Disclaimer.md` for context on the format and purpose of those notes.  
See `Docs/Devjournal/Index.md` for the devlog table of contents and English synopses.

* * *

## License

This project is licensed under the [GNU AGPL v3.0](https://www.gnu.org/licenses/agpl-3.0).  
See LICENSE file for details.
