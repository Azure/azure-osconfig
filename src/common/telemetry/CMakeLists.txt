# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(telemetry)

add_library(telemetry STATIC
    EventPayload.c
    Telemetry.c
    IpcInterface.c)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetry PUBLIC logging
    PRIVATE commonutils parsonlib ${CMAKE_DL_LIBS} rt pthread)

if(OSCONFIG_TELEMETRY_EXECUTABLE)
    install(PROGRAMS "${OSCONFIG_TELEMETRY_EXECUTABLE}" DESTINATION bin RENAME OSConfigTelemetry)
    install(PROGRAMS "${OSCONFIG_TELEMETRY_EXECUTABLE}" DESTINATION lib/osconfig RENAME OSConfigTelemetry)
endif()
