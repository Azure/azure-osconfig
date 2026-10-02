# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(telemetry)

if(BUILD_EXTERNAL)
    set(BUILD_TELEMETRY ON)
    include(../../cmake/Dependencies.cmake)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.5")
        message(FATAL_ERROR "The telemetry subprocess requires GCC 5.5 or newer")
    endif()
    if(BUILD_TESTS)
        enable_testing()
    endif()
    add_subdirectory(lib)
    add_subdirectory(bin)
else()
    # Offline development inventory, available even when legacy builds disable tests.
    add_executable(telemetrytlsprobe EXCLUDE_FROM_ALL TelemetryTlsProbe.c)
    target_link_libraries(telemetrytlsprobe PRIVATE ${CMAKE_DL_LIBS})

    # Candidate C worker: not installed or staged into production ZIPs yet.
    add_executable(telemetryworker EXCLUDE_FROM_ALL
        TelemetryWorkerMain.c TelemetryResolverLookup.c)
    set_target_properties(telemetryworker PROPERTIES
        OUTPUT_NAME OSConfigTelemetry
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/distilled")
    target_link_libraries(telemetryworker PRIVATE logging rt)

    add_library(telemetryworkerclient STATIC TelemetryWorker.c)
    target_include_directories(telemetryworkerclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(telemetryworkerclient PUBLIC telemetryresolver)

    add_executable(telemetryresolverworker TelemetryResolverWorker.c TelemetryResolverLookup.c)
    target_link_libraries(telemetryresolverworker PRIVATE rt)

    add_library(telemetryresolver STATIC TelemetryResolver.c)
    target_include_directories(telemetryresolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(telemetryresolver PUBLIC logging PRIVATE rt)

    add_library(telemetryencoder STATIC TelemetryEncoder.c)

    target_include_directories(telemetryencoder
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(telemetryencoder PUBLIC logging)

    add_library(telemetryhttp STATIC TelemetryHttp.c)
    target_include_directories(telemetryhttp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(telemetryhttp PUBLIC logging PRIVATE parsonlib)

    add_library(telemetryproxy STATIC TelemetryProxy.c)
    target_include_directories(telemetryproxy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(telemetryproxy PUBLIC logging)

    # Loaded OS TLS belongs only to the owned worker, never to the policy SO.
    add_library(telemetrytls STATIC TelemetryTls.c)
    target_include_directories(telemetrytls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(telemetrytls PUBLIC logging PRIVATE ${CMAKE_DL_LIBS} rt)

    add_library(telemetry
        STATIC
            Telemetry.c)

    target_include_directories(telemetry
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(telemetry
        PRIVATE
            commonutils
        PUBLIC
            dl)
endif()
