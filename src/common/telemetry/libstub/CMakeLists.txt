# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrystublib)
set (CMAKE_C_STANDARD 11)

add_library(telemetrystublib STATIC
    TelemetryStub.c
)

target_include_directories(telemetrystublib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
