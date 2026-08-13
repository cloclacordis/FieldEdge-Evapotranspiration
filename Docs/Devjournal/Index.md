## Devlog Table of Contents

| # | Devlog | Summary |
|---|--------|---------|
| 01 | Tasks Setting | Project start: goals, top-down/walking-skeleton principles, three-layer equation split. |
| 02 | First Modules Development | First working code: mock temperature sensor -> temperature module -> vapour-pressure module. |
| 03 | Improving the First Modules | Adds `Status` codes, explicit `initialized` flags, and a manual FAO-56 test harness. |
| 04 | Interim Documentation | Architecture diagrams and formal module contracts as of the current state. |
| 05 | Extraterrestrial Radiation Module | Geolocation, astronomy, and extraterrestrial radiation (Ra, eq. 21) modules. |
| 06 | Test Explanation, Checks, `goto` Note | Explains the test cases and justifies `goto` usage against MISRA C:2012. |
| 07 | Physical Meaning of Ra | An essay on solar geometry, transpiration, and photosynthesis. |
| 08 | Sunshine Duration Calculation Module | Sunshine-duration accumulator; fixes a source-tracking bug. |
| 09 | Solar Radiation Calculation Module | Angström–Prescott solar and clear-sky radiation (Rs, Rso, eq. 35, 37). |
| 10 | The “Default J” Dilemma & Configuration | Deployment-config layer, date provider, Type A/B/C/D data taxonomy. |
| 11 | Just-in-Time Improvements | Consolidation pass: real timestamps, `initialized`-flag audit across all structs. |
| 12 | Test Automation | Migrates the test suite to the Unity framework. |
| 13 | Net Radiation Module & Derivatives | Shortwave/longwave net radiation (Rns, Rnl, Rn, eq. 38–40). |
| 14 | Atmospheric Modules & Functions | Humidity, psychrometric constant, pressure model with 3-tier fallback (eq. 7, 8, 17). |
| 15 | Wind Speed Modules | Wind-speed read/calc and 2 m-height correction (eq. 47). |
| 16 | The Penman–Monteith Equation | Assembles all derivatives into ETo/ETc (eq. 6, 56). |
| 17 | Summarizing v0.1.0 | Status snapshot: 53 tests, coverage table, roadmap to MCU port. |
| 18 | Improvements to v0.1.0 | Hardening: compiler warnings, `NaN` checks, `main.c`/`daily-cycle.c` split, 58 tests, `cppcheck`. |

* * *

## Detailed Сontent

### devlog01 — Task Setting

Project start. Frames the overall goal (an embedded, FAO-56 Penman–Monteith reference evapotranspiration engine for field irrigation systems / agrometeorological nodes) and splits it into a PC-side computation kernel (v0.1.0) and a future MCU-side port (v0.2.0). Establishes two guiding development principles: top-down design (applied logic first, hardware constraints later) and “walking skeleton” (always keep a compiling, runnable end-to-end pipeline). Introduces the three-layer split of the Penman–Monteith equation (radiation, atmosphere, and normalization blocks) and the idea of “derivatives” — equation terms that require their own sub-calculations.

* * *

### devlog02 — First Modules Development

First working code. Builds the minimal vertical slice: a mock air-temperature sensor reading, a module computing daily min/max/mean temperature, and a vapour-pressure module computing saturation vapour pressure and the slope-of-curve term (“delta”) per FAO-56 eq. 9–13. Wires it all together in a first `main.c` and verifies output against FAO-56 Annex 2 tables at four temperature values (1.0, 20.0, 27.5, 48.5 °C), all matching within ~0.0005 kPa.

* * *

### devlog03 — Improving the First Modules

Hardening pass on the skeleton before tackling the harder radiation block. Introduces a shared validation layer: a `Status` enum for explicit error signaling, `isfinite()`-based range checks, and explicit `initialized` flags instead of relying on `0.0` as an implicit “not set” marker. Adds a manual test harness (`main-test.c` with `#define TEST_CASE`) and walks through seven categories of checks (NaN, Infinity, NULL pointers, min/max update logic, etc.), each with expected FAO-56-derived output. Establishes the layering that persists through the whole project: Sensor Read -> Validation -> State Update -> Calculation -> Output.

* * *

### devlog04 — Interim Documentation

A pause-and-document entry, no new code. Snapshots the file structure, adds four Mermaid diagrams (context, layering, data-flow, state chart) reflecting the architecture as of devlog03, and writes out formal module contracts (function signatures, status meanings, invariants) for validation, measurement, and the two calculation modules built so far. Lays down conventions for future modules: the `initialized`-flag rule, `Calc_` naming for public calculation functions, one-directional layer dependencies, and FAO-56 reference checking via `main-test.c` before integration. Closes by previewing the next big block: net radiation and its four sub-terms (Ra, Rs, Rnl, Rns), plus the project’s plan to substitute a cheap illuminance sensor + astronomical model for an expensive pyranometer.

* * *

### devlog05 — Extraterrestrial Radiation Module

First module of the radiation block: extraterrestrial radiation, Ra (FAO-56 eq. 21). Adds a geolocation module (DMS-to-decimal-degree conversion, latitude in radians), a day-of-year/astronomy module (inverse Earth–Sun distance, solar declination, sunset hour angle, day length — eq. 23–25, 34), and the Ra calculation itself, including correct handling of polar day/night edge cases via clamped arccos arguments. Also adds a mock illuminance (“sunshine lux”) sensor module for later use in computing actual sunshine duration. Sixteen manual test scenarios verify astronomical derivatives and Ra against FAO-56 worked examples 7–9 (Bangkok and Rio de Janeiro latitude conversions; Ra at 20°S, J = 246; polar night at 80°N).

* * *

### devlog06 — Test Explanation, Checks, `goto` Note

An explanatory entry (no new features) walking through why each of the 16 manual test cases from devlog05 exists and what class of bug it guards against — separating “does the math match FAO-56” tests from “does the defensive layer correctly reject bad input” tests. Includes a reasoned justification for the two uses of `goto done` in the test harness, citing MISRA C:2012 rules 15.1–15.3 and explaining why the two required rules are satisfied while the one advisory rule is knowingly violated.

* * *

### devlog07 — Physical Meaning of Ra

An essay, written to make the astronomy and trigonometry behind Ra intuitively clear, and as an optional postscript, to sketch the broader picture of solar energy flow through plant transpiration and photosynthesis. Covers: the solar constant and its origin in fusion at the Sun’s core; Earth’s elliptical orbit and the seasonal variation in received energy; solar declination and day length as functions of Earth’s axial tilt; and, in the postscript, why plants transpire so much water relative to the CO2 they fix (stomatal gas exchange trade-off, cohesion-tension mechanism of water transport, C4/CAM photosynthesis as evolved responses, and open questions about engineering around the transpiration “cost”).

* * *

### devlog08 — Sunshine Duration Calculation Module

Builds the sunshine-duration accumulator module (`sunshine-lux-calc`), which converts a series of binary “bright/not bright” lux readings (mimicking a Campbell–Stokes heliograph) into hours of actual sunshine, n — needed for the Angström–Prescott solar radiation formula. This entry finds and fixes a logic bug: the daily data-source flag (`MEASURED` vs `DEFAULT`) never updated to `MEASURED` because of a flawed `if/else if` condition combined with a `DEFAULT`-initialized starting state — traced from a mismatch in program output, then fixed with explicit `has_any_samples`/`has_default_samples` flags finalized once per day. Also notes an open design question (“what to do when the calendar date itself is unavailable”) deferred to devlogs 09–10.

* * *

### devlog09 — Solar Radiation Calculation Module

Implements the Angström–Prescott solar radiation formula, Rs (FAO-56 eq. 35) and the clear-sky radiation formula, Rso (eq. 37), including elevation-dependent clear-sky coefficient, n/N clamping to [0,1], and polar-night zero handling. Along the way, upgrades `Location_DMS_to_decimal()` from a bare `double`-returning function to a `Status`-based contract with explicit minute-range validation (0 ≤ minutes < 60), for consistency with the rest of the codebase. Adds eight new test cases (17–24) covering the sunshine accumulator in isolation (full sun, no sun, mixed day), the FAO-56 ex. 10 reference case (Rio de Janeiro), n > N clamping, invalid Angström coefficients, and uninitialized-input rejection.

* * *

### devlog10 — The “Default J” Dilemma & Configuration

An architectural entry. Introduces a new `02-providers` layer separating deployment-time configuration (`deployment-config.h`, e.g. latitude, elevation, sensor thresholds — “Type A” data) from FAO-56 model constants (“Type B”), sensor/measured data (“Type C”), and derived data (“Type D”). Adds a `date-provider` module wrapping `time()`/`localtime()` to supply the real calendar day-of-year to the orchestration layer, replacing a previously hardcoded J = 246. Also adds a `test-config.h` file to centralize FAO-56 reference values and tolerances used across the test file, keeping them out of production code. New tests (25–27) validate the date provider and add a real-time-delay emulation of the sunshine sensor polling loop, using `SleepMs()` across platforms.

* * *

### devlog11 — Just-in-Time Improvements

Makes sensor timestamp fields use real `time(NULL)` values instead of mock zeros (so the monotonicity test from devlog10 is actually meaningful). Formalizes and documents, via a table, which data structures need an `initialized` flag and which don’t, based on whether they pass through multiple lifecycle stages (`Init -> Update -> downstream use`) or are filled in a single call. Finds and closes a gap: `LocationData` was missing the flag despite needing it, and propagates the fix through all dependent modules and their test cases. Also replaces several “magic numbers” in older tests with named constants from `test-config.h`/`deployment-config.h`, and adds a new test (TC28) for the `Location_Init()` initialization flag.

* * *

### devlog12 — Test Automation

Migrates the test suite from a hand-rolled `#define TEST_CASE N` / manual-recompile scheme to the Unity C testing framework, fetched via CMake’s `FetchContent`. Rewrites all 28 existing test cases as individual `test_*()` functions run through `RUN_TEST()`, eliminating the `goto done` pattern and the manual `failures` counter in favor of Unity’s built-in assertion and reporting machinery. Also restructures `CMakeLists.txt` into two clean build targets (production `fao56_app` and test `fao56_test`) runnable independently from the IDE, and reorganizes `test-config.h` with clearer section headers.

* * *

### devlog13 — Net Radiation Module & Derivatives

Implements the net radiation module: net shortwave radiation Rns (eq. 38, using the FAO-56 reference grass albedo of 0.23), net longwave radiation Rnl (eq. 39, Stefan–Boltzmann thermal emission corrected for humidity and cloudiness), and their combination into net radiation Rn (eq. 40). Includes an explanation of why each of Rnl’s three multiplicative factors behaves the way it does. Implementation clamps the cloudiness factor to avoid a physically impossible negative Rnl. Four new tests (29–32) verify against FAO-56 worked examples 11 and 12 (Rio de Janeiro), matching within tolerances explicitly justified by the documentation’s own intermediate rounding.

* * *

### devlog14 — Atmospheric Modules & Functions

Adding the humidity and atmospheric-pressure side of the equation. Builds an air-humidity read/calc pair (mirroring the existing air temperature pattern), a psychrometric-constant module (FAO-56 eq. 8, γ from atmospheric pressure), and an atmospheric-pressure model (eq. 7, pressure as a function of elevation) — the latter explicitly designed as a three-tier fallback chain (real barometric sensor -> elevation-based model -> hardcoded sea-level constant), with the priority ordering decided in the orchestration layer rather than baked into any single module, preserving the project’s “read modules never compute” rule. Adds `Calc_ActualVapourPressure()` (eq. 17, ea from humidity and temperature extremes) and a general-purpose `Calc_SaturationVapourPressure(T, ...)`, then retires the now-redundant `Calc_SaturationVapourPressureForTmean()`. Eight new tests (33–40) check against FAO-56 worked examples 2, 3, and 5.

* * *

### devlog15 — Wind Speed Modules

Adds wind-speed read and calculation modules, implementing the FAO-56 logarithmic wind profile correction (eq. 47) that converts an anemometer reading at any height to the standard 2 m reference height (u2), needed for both the aerodynamic term and the denominator of the Penman–Monteith equation. Discusses the difference between WMO-standard (10 m) and FAO-standard (2 m) anemometer placement, and why the conversion matters when reusing an existing weather station’s equipment. The calculation module accumulates daily min/max/mean wind speed and enforces that the measurement height stays constant between updates within a day (a height change mid-day signals a configuration error). Six new tests (41–46) verify against FAO-56 example 14 and cover null-pointer, invalid-value, and height-mismatch cases.

* * *

### devlog16 — The Penman–Monteith Equation

The capstone entry of the v0.1.0 computational core: assembles all previously built derivatives into the full FAO-56 Penman–Monteith reference evapotranspiration equation (eq. 6, `Calc_ETo`) and the crop evapotranspiration equation (eq. 56, `Calc_ETc = Kc × ETo`). Handles two physically meaningful edge cases explicitly: clamping actual vapour pressure to saturation vapour pressure when relative humidity would otherwise exceed 100% (a measurement-error case), and clamping ETo to zero when net radiation is negative (winter/polar-night conditions, where the raw formula could otherwise go negative). Adds a default crop coefficient (Kc mid = 1.00, reference grass) to the deployment configuration. Seven new tests (47–53) verify the equation end-to-end against a composite scenario built from earlier verified intermediate values, plus explicit soil-heat-flux, calm-wind, and negative-Rn edge cases. Closes with a cosmetic overhaul of the orchestration’s terminal output formatting (aligned columns, English labels/translation).

* * *

### devlog17 — Summarizing v0.1.0

A short status/wrap-up entry. Tags the current state as v0.1.0 (not a git tag), tallies the development process, and presents a summary table plus a full test-coverage table mapping TC1–53 to the modules they exercise. Previews the two next stages: a near-term MCU smoke test (compile the existing computational core for Arm Cortex-M4F, verify UART output, without yet building real sensor drivers) and the larger v0.2.0 effort (real sensors, RTOS, EEPROM persistence, daily scheduler).

* * *

### devlog18 — Improvements to v0.1.0

Hardening pass. Enables stricter compiler warnings (`-Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror`, plus `C_EXTENSIONS OFF` for the MCU-bound `fao56_app` target) and works through the two `-Wfloat-equal` warnings that surface, correctly distinguishing a genuinely dead/redundant equality check (fixed via an epsilon tolerance) from a physically meaningful “exactly zero” check (fixed by relying on an already-established `< 0` rejection). Adds missing `isfinite()`/`NaN` input validation to `Calc_ETo()`, `Calc_ETc()`, and `AirHumidity_Update()` (the last via a new shared `ValidHumidityPercent()` validator), with regression tests for each. Splits `main.c` into a pure `daily-cycle.c/.h` computation module (returning a `DailyResults` struct, and reporting which step failed via an `out_failed_step` parameter) and a slim `main.c` that only calls it and prints the report. Consolidates repeated math constants and clamp patterns into a shared `math-utils.h`. Runs `cppcheck` static analysis (general + MISRA), re-verifies the full pipeline against FAO-56 worked examples, and closes two smaller gaps found on a final pass: a stray diagnostic `printf()` removed from inside the otherwise print-free `daily-cycle.c`, and a missing `log_arg > 0` guard added before `log()` in `Calc_WindSpeedAt2m()`. Ends the v0.1.0 development cycle at 58 tests.
