# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetryjsonlib)
set (CMAKE_C_STANDARD 11)

add_library(telemetryjsonlib STATIC
    TelemetryJson.c)

target_link_libraries(telemetryjsonlib
    PUBLIC
        parsonlib
        dl)

target_include_directories(telemetryjsonlib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
        ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_TELEMETRY)
    target_compile_definitions(telemetryjsonlib
        PUBLIC
            -DBUILD_TELEMETRY=1)
endif()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
