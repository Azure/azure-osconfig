# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(osconfig_telemetry_reference LANGUAGES C)

# Temporary standalone reference; deliberately absent from the parent build.
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Configure ~telemetry_exe directly, not as part of the OSConfig build")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(DEFINED ENV{OsConfigTelemetryApiKey} AND NOT "$ENV{OsConfigTelemetryApiKey}" STREQUAL "")
    set(OsConfigTelemetryApiKey "$ENV{OsConfigTelemetryApiKey}")
else()
    set(OsConfigTelemetryApiKey "")
    message(STATUS "No embedded telemetry key; supply OsConfigTelemetryApiKey at runtime for delivery")
endif()
configure_file(Keys.h.in "${CMAKE_CURRENT_BINARY_DIR}/Keys.h" @ONLY)

set(OsConfigVersionStringFull "standalone-telemetry-reference")
configure_file(../../cmake/templates/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

set(TELEMETRY_REFERENCE_SOURCES
    SchemaEncoder.c
    EventPayload.c
    AriaInterface.c
    Utils.c
    OpenSslAdapter.c
    ../logging/Logging.c
    ../parson/parson.c)

add_executable(telemetryworker WorkerMain.c ${TELEMETRY_REFERENCE_SOURCES})
set_target_properties(telemetryworker PROPERTIES OUTPUT_NAME OSConfigTelemetry)

# Explicit live test only; never part of the default build or ctest.
add_executable(telemetryariatest EXCLUDE_FROM_ALL AriaTest.c ${TELEMETRY_REFERENCE_SOURCES})

foreach(target telemetryworker telemetryariatest)
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        ../commonutils
        ../logging
        ../parson)
    target_compile_options(${target} PRIVATE
        -Wall -Wextra -Wunused -Werror -Wformat -Wformat-security -Wno-unused-result
        -Wno-psabi -fno-omit-frame-pointer)
    target_link_libraries(${target} PRIVATE ${CMAKE_DL_LIBS} rt m)
endforeach()
