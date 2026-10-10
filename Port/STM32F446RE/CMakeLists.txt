# ### SPDX-License-Identifier: AGPL-3.0-or-later
# Copyright (C) 2026 Tim Alexeenko (@cloclacordis) # ###

cmake_minimum_required(VERSION 3.31)

# Cross-compile by default: no -DCMAKE_TOOLCHAIN_FILE needed
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/arm-none-eabi.cmake")
endif()

project(fao56_mcu C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS_RELEASE "-Os")

set(PORT_DIR "${CMAKE_CURRENT_LIST_DIR}")
get_filename_component(CODE_DIR "${PORT_DIR}/../../Code" ABSOLUTE)

# Kernel sources. Host main() and tests are not included
file(GLOB_RECURSE FAO56_CORE_SOURCES CONFIGURE_DEPENDS
        "${CODE_DIR}/01-measurement/*.c"
        "${CODE_DIR}/02-providers/*.c"
        "${CODE_DIR}/03-validation/*.c"
        "${CODE_DIR}/04-calculation/*.c"
)
list(APPEND FAO56_CORE_SOURCES "${CODE_DIR}/05-orchestration/daily-cycle.c")

set(MCU_CPU_FLAGS -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard)

add_library(fao56_kernel STATIC ${FAO56_CORE_SOURCES})
target_compile_options(fao56_kernel PRIVATE
        ${MCU_CPU_FLAGS} -ffunction-sections -fdata-sections
        -Wall -Wextra -Wpedantic -Wfloat-equal -Wconversion -Wshadow -Werror
)
set_target_properties(fao56_kernel PROPERTIES C_EXTENSIONS OFF)

set(STARTUP_FILE "${PORT_DIR}/Startup/startup_stm32f446retx.s")
set_source_files_properties("${STARTUP_FILE}" PROPERTIES
        COMPILE_OPTIONS "-x;assembler-with-cpp")

add_executable(fao56_mcu
        "${PORT_DIR}/Src/main-mcu.c"
        "${PORT_DIR}/Src/gpio.c"
        "${PORT_DIR}/Src/uart.c"
        "${PORT_DIR}/Src/platform-time.c"
        "${PORT_DIR}/Src/syscalls.c"
        "${PORT_DIR}/Src/sysmem.c"
        "${PORT_DIR}/Src/system_stm32f4xx.c"
        "${STARTUP_FILE}"
)
set_target_properties(fao56_mcu PROPERTIES SUFFIX ".elf")

target_include_directories(fao56_mcu PRIVATE
        "${PORT_DIR}/Inc"
        "${PORT_DIR}/CMSIS/Include"
        "${PORT_DIR}/CMSIS/Device/ST/STM32F4xx/Include"
        "${CODE_DIR}"
)
target_compile_definitions(fao56_mcu PRIVATE STM32F446xx)
target_compile_options(fao56_mcu PRIVATE
        ${MCU_CPU_FLAGS} -ffunction-sections -fdata-sections -Wall -Wextra)

target_link_options(fao56_mcu PRIVATE
        ${MCU_CPU_FLAGS}
        "-T${PORT_DIR}/STM32F446RETX_FLASH.ld"
        --specs=nano.specs --specs=nosys.specs
        -Wl,--gc-sections
        -Wl,-u,_printf_float
        "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/fao56_mcu.map"
)
target_link_libraries(fao56_mcu PRIVATE fao56_kernel m)
