# ### SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Tim Alexeenko (@cloclacordis) # ###

cmake_minimum_required(VERSION 4.1)
project(FAO56 C)

set(CMAKE_C_STANDARD 11)

### ## # Source code files used for both targets # ## ###
set(FAO56_SOURCES
        01-measurement/011-air-temperature-read/air-temperature-read.c
        01-measurement/011-air-temperature-read/air-temperature-read.h

        01-measurement/012-air-humidity-read/air-humidity-read.c
        01-measurement/012-air-humidity-read/air-humidity-read.h

        01-measurement/013-atm-pressure-read/atm-pressure-read.c
        01-measurement/013-atm-pressure-read/atm-pressure-read.h

        01-measurement/014-sunshine-lux-read/sunshine-lux-read.c
        01-measurement/014-sunshine-lux-read/sunshine-lux-read.h

        01-measurement/015-wind-speed-read/wind-speed-read.c
        01-measurement/015-wind-speed-read/wind-speed-read.h

        02-providers/021-date-provider/date-provider.c
        02-providers/021-date-provider/date-provider.h

        02-providers/022-configurations/deployment-config.h

        03-validation/031-value-source/value-source.c
        03-validation/031-value-source/value-source.h

        03-validation/032-validation/validation.c
        03-validation/032-validation/validation.h

        03-validation/033-status/status.c
        03-validation/033-status/status.h

        04-calculation/041-air-temperature-calc/air-temperature-calc.c
        04-calculation/041-air-temperature-calc/air-temperature-calc.h

        04-calculation/042-air-humidity-calc/air-humidity-calc.c
        04-calculation/042-air-humidity-calc/air-humidity-calc.h

        04-calculation/043-vapour-pressure-calc/vapour-pressure-calc.c
        04-calculation/043-vapour-pressure-calc/vapour-pressure-calc.h

        04-calculation/044-atmospheric-calc/atm-pressure-model.c
        04-calculation/044-atmospheric-calc/atm-pressure-model.h

        04-calculation/044-atmospheric-calc/psychrometric-calc.c
        04-calculation/044-atmospheric-calc/psychrometric-calc.h

        04-calculation/045-radiation-calc/geolocation-calc.c
        04-calculation/045-radiation-calc/geolocation-calc.h

        04-calculation/045-radiation-calc/day-in-year-calc.c
        04-calculation/045-radiation-calc/day-in-year-calc.h

        04-calculation/045-radiation-calc/sunshine-lux-calc.c
        04-calculation/045-radiation-calc/sunshine-lux-calc.h

        04-calculation/045-radiation-calc/extrater-radiation-calc.c
        04-calculation/045-radiation-calc/extrater-radiation-calc.h

        04-calculation/045-radiation-calc/solar-radiation-calc.c
        04-calculation/045-radiation-calc/solar-radiation-calc.h

        04-calculation/045-radiation-calc/net-radiation-calc.c
        04-calculation/045-radiation-calc/net-radiation-calc.h

        04-calculation/046-wind-speed-calc/wind-speed-calc.c
        04-calculation/046-wind-speed-calc/wind-speed-calc.h

        04-calculation/047-evapotranspiration-calc/eto-calc.c
        04-calculation/047-evapotranspiration-calc/eto-calc.h
)

### ## # Target 1: application binary [orchestration] # ## ###
add_executable(fao56_app
        ${FAO56_SOURCES}
        05-orchestration/main.c
)

target_link_libraries(fao56_app m)

### ## # Target 2: test binary [unity] # ## ###
include(FetchContent)

FetchContent_Declare(
        unity
        GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
        GIT_TAG        v2.6.0
)

FetchContent_MakeAvailable(unity)

add_executable(fao56_test
        ${FAO56_SOURCES}
        06-test/test-config.h
        06-test/main-test.c
)

target_link_libraries(fao56_test m unity)
target_compile_definitions(unity PUBLIC UNITY_INCLUDE_DOUBLE)

# CTest
enable_testing()
add_test(NAME fao56_suite COMMAND fao56_test)
