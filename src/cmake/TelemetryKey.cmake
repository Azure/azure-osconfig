# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

function(osconfig_configure_telemetry_key output fallback)
    if(DEFINED ENV{OsConfigTelemetryApiKey} AND NOT "$ENV{OsConfigTelemetryApiKey}" STREQUAL "")
        set(OsConfigTelemetryApiKey "$ENV{OsConfigTelemetryApiKey}")
        message(STATUS "Using OsConfigTelemetryApiKey from environment variable")
    else()
        message(WARNING "OsConfigTelemetryApiKey not set, using target default")
        set(OsConfigTelemetryApiKey "${fallback}")
    endif()
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../common/telemetry/Keys.h.in"
        "${output}" @ONLY)
endfunction()
