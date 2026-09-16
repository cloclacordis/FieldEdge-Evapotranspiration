# devlog19. Дополнительные проверки и документация *v0.1.0* (обновляется)

*Adds Valgrind memory checks, introduces optional AddressSanitizer and UndefinedBehaviorSanitizer builds via a dedicated `ENABLE_SANITIZERS` CMake option. Verifies the sanitizer configuration, confirms that all 58 tests pass without sanitizer errors, and validates the actual compile and link flags with a verbose `fao56_test` build. Extends CI with a dedicated sanitizer job that configures, builds, and runs the test suite with CTest. The devlog will be updated with the software architecture documentation and related supporting documents.*

* * *

## Дополнительные проверки

### Valgrind

Запустим проверку `fao56_test`:

```bash
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./fao56_test
```

![](resources/1900-valgrind-output-1.png)  
![](resources/1901-valgrind-output-2.png)

Проверка завершилась без обнаружения ошибок управления памятью.

**Valgrind** показал `9 allocs, 9 frees`. В файлах нашего исходного кода динамическое выделение памяти не применяется. Дополнительная проверка через **GDB** показала, что наблюдаемое выделение памяти происходит внутри `libc` при работе с локальным временем, а не непосредственно в коде приложения. `DateProvider_Read()` вызывает **API** работы со временем на ПК, после чего системная библиотека при обработке часового пояса (`/etc/localtime`) выполняет внутренний `malloc()`.

Цепочка вызовов имеет следующий вид:

```
__GI___libc_malloc(15)                   [malloc/malloc.c:3294]
    <- __GI___strdup("/etc/localtime")   [string/strdup.c:42]
    <- tzset_internal()                  [time/tzset.c:402]
    <- __tz_convert()                    [time/tzset.c:577]
    <- DateProvider_Read()               [date-provider.c:18]
    <- RunDailyCycle()                   [daily-cycle.c:268]
    <- main()                            [main.c:17]
```

При портировании на МК источник времени будет заменен на **RTC**. Реализацию `DateProvider` нужно будет проверить дополнительно - чтобы работа с **RTC** не приводила к динамическому выделению памяти.

* * *

### AddressSanitizer, UndefinedBehaviorSanitizer

Добавим в `CMakeLists.txt`:

```Cmake
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Добавление
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

...

target_link_libraries(fao56_test m unity)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)
target_compile_options(fao56_test PRIVATE -Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror)

# Добавление
if(ENABLE_SANITIZERS)
    target_compile_options(fao56_test PRIVATE 
            -fsanitize=address,undefined 
            -fno-omit-frame-pointer 
            -g
    )

    target_link_options(fao56_test PRIVATE 
            -fsanitize=address,undefined
    )
endif()

# CTest
enable_testing()
add_test(NAME fao56_suite COMMAND fao56_test)
```

Добавим конфигурацию в **CLion**:

```md
File -> Settings -> Build, Execution, Deployment -> CMake
```

Создадим новую конфигурацию с именем `Sanitizers` и включим в **CMake options** строку:

```bash
-DENABLE_SANITIZERS=ON
```

![](resources/1902-cmake-options.png)

Сохраним настройки.

**CMake profile** `Sanitizers`, затем **target** `fao56_test`.

![](resources/1903-cmake-profile.png)

Запустим сборку.

Получили успешную сборку.

![](resources/1904-sanitizer-build-output.png)

Запустим `fao56_test`.

![](resources/1905-sanitizer-run-output.png)


Тестовая программа завершилась успешно. Сборка и запуск тестов выполнены с включенными **AddressSanitizer** и **UndefinedBehaviorSanitizer**. Все 58 тестов пройдены успешно, сообщений об ошибках не получено.

Корректность применения *sanitizer*-флагов дополнительно проверена с помощью *verbose*-сборки:

```bash
cmake --build cmake-build-sanitizers --target fao56_test --verbose
```

> В командах компиляции исходных файлов `fao56_test` можно видеть флаги `-fsanitize=address,undefined`. При финальной линковке исполняемого файла `fao56_test` этот флаг также присутствует.

* * *

### Автоматизация CI

В файл `.github/workflows/build-and-test.yml` добавим рядом с `build-and-test` новую проверку/`job` (`sanitizer`):

```yaml
name: Build and Test
on: [push, pull_request]

...

  sanitizer:
    runs-on: ubuntu-latest

    steps:
      - name: Get source code
        uses: actions/checkout@v6

      - name: Configure
        run: cmake -S Code -B build-sanitizers -DENABLE_SANITIZERS=ON

      - name: Build
        run: cmake --build build-sanitizers

      - name: Test
        run: ctest --test-dir build-sanitizers --output-on-failure
```

> Автоматические проверки настроены и выполняют работу.

* * *

## Документация

### Контекстная диаграмма

The system operates at the edge, acquiring agrometeorological data from sensors, processing and validating the measurements, and calculating reference evapotranspiration (ETo). The resulting ETo value is provided to an external irrigation system for local irrigation decision-making and control. Version v0.1.x implements the measurement and computation pipeline on a PC using emulated sensor data. Version v0.2.x is the STM32 implementation under development with real sensors, LoRa communication, and real-time operation.

```mermaid
flowchart LR
    ENV["Environmental<br/>conditions"]
    SENS["Sensors"]

    subgraph FieldEdge["FieldEdge"]
        direction LR
        MEAS["Measurement<br/>subsystem"]
        CALC["Computation<br/>kernel"]
        MEAS -->|"Measurement data"| CALC
    end

    DEC["Irrigation<br/>system"]

    ENV -->|"Physical quantities"| SENS
    SENS -->|"Sensor readings"| MEAS
    CALC -->|"ETo value"| DEC

    classDef external fill:#f7f7f7,stroke:#555,stroke-width:1.5px,color:#222
    classDef system fill:#eaf2f8,stroke:#2c5f85,stroke-width:2px,color:#111
    classDef internal fill:#fff,stroke:#2c5f85,stroke-width:1.5px,color:#111

    class ENV,SENS,DEC external
    class MEAS,CALC internal
```

* * *

### Диаграмма архитектуры ПО

#### Общие сведения

The **FieldEdge-Evapotranspiration** software is organized into five functional subsystems with unidirectional dependencies. The **Orchestration** subsystem coordinates the overall measurement and calculation pipeline. Among the downstream components, the **Measurement** subsystem acquires sensor data, **Providers** supply external and deployment-specific data, **Validation** ensures shared validation and status handling, and **Calculation** executes the ETo computation.

The **Calculation** subsystem remains independent of sensor hardware, decoupled from data ingestion, and is invoked by the **Orchestration** subsystem with pre-validated inputs.

* * *

#### Структура проекта

The project structure for v0.1.x is as follows.

```md
FieldEdge-Evapotranspiration	
  01-measurement	
    011-air-temperature-read	
        air-temperature-read.c	
        air-temperature-read.h	
    012-air-humidity-read	
        air-humidity-read.c	
        air-humidity-read.h	
    013-atm-pressure-read	
        atm-pressure-read.c	
        atm-pressure-read.h	
    014-sunshine-lux-read	
        sunshine-lux-read.c	
        sunshine-lux-read.h	
    015-wind-speed-read	
        wind-speed-read.c	
        wind-speed-read.h	
  02-providers	
    021-date-provider	
        date-provider.c	
        date-provider.h	
    022-configurations	
        deployment-config.h	
  03-validation	
    031-value-source	
        value-source.c	
        value-source.h	
    032-validation	
        validation.c	
        validation.h	
    033-status	
        status.c	
        status.h	
    034-math-utils	
        math-utils.h	
  04-calculation	
    041-air-temperature-calc	
        air-temperature-calc.c	
        air-temperature-calc.h	
    042-air-humidity-calc	
        air-humidity-calc.c	
        air-humidity-calc.h	
    043-vapour-pressure-calc	
        vapour-pressure-calc.c	
        vapour-pressure-calc.h	
    044-atmospheric-calc	
        atm-pressure-model.c	
        atm-pressure-model.h	
        psychrometric-calc.c	
        psychrometric-calc.h	
    045-radiation-calc	
        day-in-year-calc.c	
        day-in-year-calc.h	
        extrater-radiation-calc.c	
        extrater-radiation-calc.h	
        geolocation-calc.c	
        geolocation-calc.h	
        net-radiation-calc.c	
        net-radiation-calc.h	
        solar-radiation-calc.c	
        solar-radiation-calc.h	
        sunshine-lux-calc.c	
        sunshine-lux-calc.h	
    046-wind-speed-calc	
        wind-speed-calc.c	
        wind-speed-calc.h	
    047-evapotranspiration-calc	
        eto-calc.c	
        eto-calc.h	
  05-orchestration	
        daily-cycle.c	
        daily-cycle.h	
        main.c	
  06-test	
        main-test.c	
        test-config.h
  CMakeLists.txt
```

* * *

#### Слои и модули

The core of the system consists of two independent layers: `measurement` (01) and `calculation` (04). Neither depends on the other — there is no dependency edge between them in either direction. They are connected only through `orchestration` (05), which reads sensor data and invokes the calculation pipeline within the same run.

`validation` (03) serves as the foundation: every other layer depends on it, and it has no outgoing dependencies of its own. `providers` (02) is a shared supporting layer that also depends on `validation` (03).

Calculation functions operate on validated physical values rather than raw sensor readings. This allows the entire `measurement` (01) layer can be reworked (e.g., to integrate sensor/peripheral drivers in v0.2.x) without touching a single `calculation` (04) module.

```mermaid
%%{init: {
  "theme": "base",
  "themeVariables": {
    "background": "#12141a",
    "primaryColor": "#1c1f27",
    "primaryBorderColor": "#3a3f4b",
    "primaryTextColor": "#e8e9ec",
    "lineColor": "#7a8291",
    "fontFamily": "-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif",
    "fontSize": "14px"
  },
  "flowchart": {
    "curve": "linear",
    "nodeSpacing": 60,
    "rankSpacing": 90,
    "htmlLabels": true
  },
  "style": {
    "global": ".node *, .node td { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif !important; } .node td span { font-family: ui-monospace, SFMono-Regular, SF Pro Text, Menlo, Monaco, Consolas, 'Liberation Mono', monospace !important; font-size: 12px !important; letter-spacing: -0.2px; }"
  }
}}%%
 
flowchart TB
 
  %% LAYER 05 — ORCHESTRATION
  L05["<table style='width:560px;border-collapse:collapse'>
    <tr><td style='text-align:left;font-size:18px;font-weight:700;color:#f2c14e;padding:2px 4px 8px 4px'>05 · orchestration</td></tr>
    <tr><td style='text-align:left;padding:4px;border-top:1px solid #3a3f4b'>
      <span style='color:#cfd3da'>main&nbsp;&nbsp;·&nbsp;&nbsp;daily-cycle</span>
    </td></tr>
  </table>"]
 
  %% LAYER 01 — MEASUREMENT
  L01["<table style='width:560px;border-collapse:collapse'>
    <tr><td style='text-align:left;font-size:18px;font-weight:700;color:#7fb0e0;padding:2px 4px 8px 4px'>01 · measurement</td></tr>
    <tr><td style='text-align:left;padding:4px;border-top:1px solid #3a3f4b'>
      <span style='color:#cfd3da'>
      011 air-temperature-read&nbsp;&nbsp;·&nbsp;&nbsp;012 air-humidity-read<br/>
      013 atm-pressure-read&nbsp;&nbsp;·&nbsp;&nbsp;014 sunshine-lux-read<br/>
      015 wind-speed-read<br/>
      &nbsp;
      </span>
    </td></tr>
  </table>"]
 
  %% LAYER 04 — CALCULATION
  L04["<table style='width:560px;border-collapse:collapse'>
    <tr><td style='text-align:left;font-size:18px;font-weight:700;color:#e08a6b;padding:2px 4px 8px 4px'>04 · calculation</td></tr>
    <tr><td style='text-align:left;padding:4px;border-top:1px solid #3a3f4b'>
      <span style='color:#cfd3da'>
      041 air-temperature-calc&nbsp;&nbsp;·&nbsp;&nbsp;042 air-humidity-calc<br/>
      043 vapour-pressure-calc&nbsp;&nbsp;·&nbsp;&nbsp;044 atmospheric-calc<br/>
      045 radiation-calc&nbsp;&nbsp;·&nbsp;&nbsp;046 wind-speed-calc<br/>
      047 evapotranspiration-calc
      </span>
    </td></tr>
  </table>"]
 
  %% LAYER 02 — PROVIDERS
  L02["<table style='width:560px;border-collapse:collapse'>
    <tr><td style='text-align:left;font-size:18px;font-weight:700;color:#7fc9a4;padding:2px 4px 8px 4px'>02 · providers</td></tr>
    <tr><td style='text-align:left;padding:4px;border-top:1px solid #3a3f4b'>
      <span style='color:#cfd3da'>
      021 date-provider&nbsp;&nbsp;·&nbsp;&nbsp;022 configurations<br/>
      &nbsp;
      </span>
    </td></tr>
  </table>"]
 
  %% LAYER 03 — VALIDATION
  L03["<table style='width:560px;border-collapse:collapse'>
    <tr><td style='text-align:left;font-size:18px;font-weight:700;color:#c9cdd6;padding:2px 4px 8px 4px'>03 · validation</td></tr>
    <tr><td style='text-align:left;padding:4px;border-top:1px solid #3a3f4b'>
      <span style='color:#cfd3da'>
      031 value-source&nbsp;&nbsp;·&nbsp;&nbsp;032 validation<br/>
      033 status&nbsp;&nbsp;·&nbsp;&nbsp;034 math-utils
      </span>
    </td></tr>
  </table>"]
 
  %% HIGH-LEVEL DEPENDENCIES ONLY
  L05 --> L01
  L05 --> L02
  L05 --> L03
  L05 --> L04
 
  L04 --> L02
  L04 --> L03
 
  L01 --> L02
  L01 --> L03
 
  L02 --> L03
 
  %% NODE STYLES
  classDef layer fill:#1c1f27,stroke:#3a3f4b,stroke-width:1.5px,color:#e8e9ec,rx:10,ry:10;
  class L05,L01,L04,L02,L03 layer;
 
  linkStyle default stroke:#7a8291,stroke-width:1.6px;
```

* * *

For dependencies between individual functions, types, and modules, see the Doxygen reference (a link will be added later).

A fallback image in case the Mermaid diagram does not display correctly.

![](resources/1906-v01x-layer-diagram.png)

* * *

### Подтвержденный порядок вызовов

*Verified call graph* описан на основе анализа кода статическими и динамическими средствами: *cflow* и *gdb*. Подробный лог работы с отладчиком занял бы слишком много места даже для девлога. Результаты анализа и документацию вызовов см. непосредственно в файле [`verified-call-graph.md`](../../verified-call-graph.md). Ниже приводится вывод статической структуры вызовов первого уровня для оркестрирующей функции `RunDailyCycle()`.

![](resources/1907-cflow-run-daily-cycle.png)

* * *

### Документация открытых проблем

Для v0.1.0 задокументированы следующие проблемы (см. [`issues-v01x.md`](../../issues-v01x.md)).

#### 1. Из раздела *"Limitations and open questions of v0.1.x"* `README.md`:

* Sensor data is currently emulated using fixed constants; no real hardware is involved yet.  
* `time()` from `<time.h>` is used for the current day of year and illuminance measurement timestamps. The PC implementation causes an internal heap allocation in `libc` when timezone information is initialized; this allocation is not performed by application code. When porting to an MCU, the PC time implementation will be replaced with RTC access, and the MCU implementation should be verified to ensure that it does not introduce dynamic memory allocation.  
* State is not persisted between runs (EEPROM/Flash persistence is planned for v0.2.x).  
* The pipeline computes a single daily cycle per run; the sampling model (some sensors read once, illuminance read on a fixed interval) is a PC-development convenience and will be unified into one periodic model once real-time sampling on the MCU is designed.  
* All computation uses `double` throughout, though the target MCUs (Arm Cortex-M4F) only have single-precision hardware floating point support. This is a deliberate choice: accuracy took priority over speed for a value computed once per day, and the FAO-56 reference values were validated at `double` precision. This decision will be revisited when real timing data from the MCU port is available.  
* The illuminance-based sunshine-duration threshold is a preliminary estimate, not yet empirically calibrated against real hardware — planned for the sensor-driver development stage.

* * *

#### 2. Из первой версии документа `issues-v01x.md`:

**A) `pressure_sample.source` may read indeterminate memory.**

* Location: `Code/05-orchestration/daily-cycle.c`, pressure acquisition branch. Root cause: `Code/05-orchestration/main.c:15` (`DailyResults results;`, declared without an initializer).  
* If `SensorPressure_ReadInstant()` fails while `Calc_PressureFromElevation()` (the elevation-model fallback) succeeds, `pressure_sample` is never written in that branch. `PrintReport()` later reads `pressure_sample.source` to label the pressure source as “sensor” or “model/constant”; in this branch, the field holds indeterminate stack memory rather than a defined value. Because `SENSOR_VALUE_MEASURED` is `0` (see `value-source.h`), a zero-valued leftover byte pattern would silently print “sensor” for a value that came from the model — no error, no abnormal `Status`, only a mislabeled report line.  
* Status: latent on the PC build. All `Sensor*_ReadInstant()` mock implementations return a non-`STATUS_OK` status only on a `NULL` output pointer, and every call site in `RunDailyCycle()` passes a valid pointer — so this branch is currently unreachable (see `verified-call-graph.md`, “Unverified branches”). It becomes live once a real pressure sensor driver can fail independently of the model. Recommended fix before enabling that driver: zero-initialize `results` in `main()`, or set `pressure_sample.source` explicitly on the model-fallback path.  
* Full derivation: `dataflow-specification.md`, Observations, item 2 (link to be added).

**B) `e_tmean` is computed but not propagated.**

* Location: `Code/04-calculation/043-vapour-pressure-calc`. Computed by `Calc_SaturationVapourPressure()` from `temperature_data.T_mean_C`, consumed only by `PrintReport()`.  
* The ETo calculation uses `e_s` (from `Calc_MeanSaturationVapourPressure()`), not `e_tmean`. The underlying formula is independently exercised by `test_AirTemperature_NormalPath_T20` in `main-test.c`; this item is about an unused field in the production dataflow, not an unverified calculation.  
* Status: no functional impact. Either remove the field from `DailyResults`, or document explicitly that it is diagnostic-only.  
* Full derivation: `dataflow-specification.md`, Observations, item 1 (link to be added).

**C) `double` precision margin relative to the target FPU.**

* Location: throughout `Code/04-calculation`.  
* Already tracked in the `README`’s “Limitations and open questions of v0.1.x” as a deliberate, revisit-later decision. Added here as supporting evidence: the existing tolerances, e.g. `TOL_KPA = 0.0001` in `Code/06-test/test-config.h`, are broadly compatible with the numerical resolution of single-precision float for values in the expected operating range. Since the STM32’s Cortex-M4F provides a single-precision FPv4-SP FPU, this supports the feasibility of a future float migration, but the migration should still be validated against actual value ranges and accumulated numerical error.  
* Status: open, per `README`, pending real data from the MCU port.

**D) Time zone dependency of `DateProvider_Read`.**

* Location: `Code/02-providers/021-date-provider/date-provider.c`.  
* `localtime()` resolves the current date using the host’s/runtime’s configured local time zone. Day-of-year and solar declination calculations are date-sensitive at the midnight boundary.  
* Status: open design question for the RTC-based replacement planned for v0.2.x. The time-base semantics (UTC vs. local time, and any required time-zone/DST handling) should be defined explicitly rather than relying on host-runtime time-zone configuration that will not be implicitly available on the MCU.

* * *

### Решение некоторых проблем и обновление документации

Некоторые проблемы из вышеприведенного списка решим сразу же. В частности, решим проблему **A)** и обновим документацию. Кроме того, при анализе проблемы **А)** мы обнаружили, что тот же блок кода не содержит, как нам бы хотелось, инструкции для трассирования потенциальных сбоев. Речь об этом участке кода из функции `RunDailyCycle()` файла `daily-cycle.c`:

```C
    /* Atmospheric pressure (priority sources for P) */
    status = SensorPressure_ReadInstant(&out->pressure_sample);
    out->trace.pressure_read_status = status;
    if (status == STATUS_OK) {
        /* Source 1: sensor */
        out->P_source_kPa = out->pressure_sample.P_kPa;
    } else {
        /* Source 2: eq. 7 model, preferred fallback */
        status = Calc_PressureFromElevation(out->location.elevation_m,
            &out->P_source_kPa);

        out->trace.pressure_model_status = status;

        if (status != STATUS_OK) {
            /* Source 3: final fallback level */
            (void)SensorPressure_ReadDefault(&out->pressure_sample);

            out->P_source_kPa = out->pressure_sample.P_kPa;
        }
    }
```

Как видно, кроме описанной выше проблемы **A)**, здесь также отсутствует регулярная для этой функции *fail-fast* идиома, а именно:

```C
	status = Step(...);
	if (status != STATUS_OK) {
	    *out_failed_step = "Step";
	    return status;
	}
```

Решим обе эти проблемы следующим образом:

```C
    /* Atmospheric pressure (priority sources for P) */
    status = SensorPressure_ReadInstant(&out->pressure_sample);
    out->trace.pressure_read_status = status;
    if (status == STATUS_OK) {
        /* Source 1: sensor */
        out->P_source_kPa = out->pressure_sample.P_kPa;
    } else {
        /* Not from the sensor: everything below is reported as "model/constant" */
        out->pressure_sample.source = SENSOR_VALUE_DEFAULT;

        /* Source 2: eq. 7 model, preferred fallback */
        status = Calc_PressureFromElevation(out->location.elevation_m, &out->P_source_kPa);
        out->trace.pressure_model_status = status;

        if (status != STATUS_OK) {
            /* Source 3: final fallback level */
            status = SensorPressure_ReadDefault(&out->pressure_sample);
            if (status != STATUS_OK) {
                *out_failed_step = "SensorPressure_ReadDefault";
                return status;
            }

            out->P_source_kPa = out->pressure_sample.P_kPa;
        }
    }
```

> После внесенных изменений тесты, сборка, анализатор, санитайзеры, программа были запущены - итоговое состояние кода v0.1.0 **проверено** перед публикацией обновлений, программа работает корректно, все ранее описанные проверки и результаты остаются в силе.

* * *

#### Обновим документацию

- Уберем из `issues-v01x.md` пункт **"`pressure_sample.source` may read indeterminate memory"**.

- Уберем из `dataflow-specification.md` (документ готовится и к настоящему моменту еще не опубликован) пункт `Observations, item 2` с тем же содержанием.

- Обновим нумерацию строк кода в документе `verified-call-graph.md` - все *GDB*-документированные функции ниже `SensorPressure_ReadInstant()` изменили построчную нумерацию на +5 строк, это должно быть отражено в обновленном документе (в этом девлоге нумерация остается без изменений и хранит первоначальную запись).
