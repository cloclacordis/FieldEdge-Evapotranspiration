# FieldEdge-Evapotranspiration

A portable C11 implementation of the FAO56 [Penman–Monteith](https://en.wikipedia.org/wiki/Penman%E2%80%93Monteith_equation) reference evapotranspiration equation, built as the computational kernel for a field-deployed edge node.

* * *

## What this is

This is not a calculator you interact with directly. It is a computation engine — a C library and pipeline designed to run on a microcontroller that reads sensor data, computes daily reference evapotranspiration (ETo) and crop evapotranspiration (ETc), and reports the result.

The repository currently contains **Phase 1 (v0.1.0)**: the full computational model running on PC with mock sensors. All modules are written in portable C11 with no platform-specific dependencies in the computation layers, in preparation for the MCU port in Phase 2.

* * *

## Why this exists

The aim is an autonomous, field-deployable irrigation and soil monitoring node that computes ETo locally at the edge — without cloud dependency, without a stable network connection, without proprietary platforms. The FAO56 Penman-Monteith method is the internationally recognised standard for this computation (Allen et al., [1998](https://www.fao.org/4/x0490e/x0490e00.htm); [2025](https://agrhysmo.agr.unipi.it/wp-content/uploads/2025/09/FAO56%202025.pdf)).

* * *

## Architecture in broad strokes

The project uses layers with strictly unidirectional dependencies.

* `Code/01-measurement`: sensor reading (mock on PC, drivers on MCU).  
* `Code/02-providers`: date/time, deployment configuration.  
* `Code/03-validation`: status codes, value ranges, data source tracking.  
* `Code/04-calculation`: computation modules (model constants, measured and computed data).  
* `Code/05-orchestration`: main pipeline — no stored state, no magic numbers.  
* `Code/06-test`: Unity test suite, reference values from FAO56 worked examples.

The calculation layer is a pure computational kernel: it has no knowledge of sensors, time, or fallback logic — it receives validated inputs and returns results; the connection happens only in the orchestration layer.

Deployment parameters (anemometer height, geographic coordinates, elevation, crop coefficient Kc, sensor thresholds) live in `Code/02-providers/022-configurations/deployment-config.h`. Model constants (λ, Stefan-Boltzmann σ, etc.) are defined locally inside each computation module in `Code/04-calculation/.../*-calc.c`.

All public functions return a `Status` value. Results are written to out-parameters. NULL is checked first in every function.

Accumulator structs (`AirTemperatureData`, `WindSpeedData`, etc.) carry an `initialized` flag that is `false` after `_Init()` and `true` after the first valid `_Update()`. Single-call result structs (`AtmosphericData`, `NetRadiationData`) are ready immediately after `_Init()`.

> More detailed architecture documentation will be added later.

* * *

## Test results

```
53 Tests  0 Failures  0 Ignored
OK
```

All test cases are verified against worked examples from FAO56 (1998).  
Reference values and tolerances are documented in `Code/06-test/test-config.h`.

* * *

## Limitations of v0.1.0

* Sensors are mock constants; no real hardware is involved yet.  
* `time()` from `<time.h>` is used for the current Julian day and lux timestamps; on a bare MCU, this requires a stub or RTC integration.  
* State is not persisted between runs (EEPROM/Flash persistence is planned for v0.2.0).  
* The pipeline computes a single daily cycle per run.

* * *

## Roadmap

* [x] v0.1.0 — Complete PC computational model, 53 unit tests.  
* [ ] v0.2.0 — MCU port, sensor drivers, RTOS scheduling.  
* [ ] v0.3.0 — LoRaWAN telemetry, field deployment.

* * *

## Development journal

Step-by-step development notes (in Russian) are in `Docs/Devjournal/Devlogs/`.  
See `Docs/Devjournal/Disclaimer.md` for context on the format and purpose of those notes.

* * *

## License

This project is licensed under the [GNU AGPL v3.0](https://www.gnu.org/licenses/agpl-3.0).
See LICENSE file for details.

* * *
