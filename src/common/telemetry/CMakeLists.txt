# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(telemetry)

add_library(telemetryencoder STATIC Encoder.c)
target_include_directories(telemetryencoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetryencoder PUBLIC logging)

add_library(telemetryevent STATIC Event.c)
target_include_directories(telemetryevent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetryevent PUBLIC telemetryencoder)

add_library(telemetryhttp STATIC Http.c)
target_include_directories(telemetryhttp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetryhttp PUBLIC logging PRIVATE parsonlib)

add_library(telemetryproxy STATIC Proxy.c)
target_include_directories(telemetryproxy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetryproxy PUBLIC logging)

# TLS providers are used only by the owned executable, never by the policy SO.
add_library(telemetrytls STATIC Tls.c)
target_include_directories(telemetrytls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetrytls PUBLIC logging PRIVATE ${CMAKE_DL_LIBS} rt)

add_library(telemetryresolver STATIC Resolver.c)
target_include_directories(telemetryresolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetryresolver PUBLIC logging PRIVATE rt)

add_library(telemetrytransport STATIC Transport.c ResolverLookup.c)
target_include_directories(telemetrytransport PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetrytransport PUBLIC telemetryhttp telemetryproxy
    PRIVATE telemetrytls telemetryresolver rt)

add_library(telemetryworkerclient STATIC Worker.c)
target_include_directories(telemetryworkerclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetryworkerclient PUBLIC telemetryresolver telemetryevent)

add_library(telemetry STATIC Telemetry.c)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetry PUBLIC logging
    PRIVATE commonutils telemetryworkerclient ${CMAKE_DL_LIBS})

include(../../cmake/TelemetryKey.cmake)
osconfig_configure_telemetry_key("${CMAKE_CURRENT_BINARY_DIR}/Keys.h" "")

add_executable(telemetryworker EXCLUDE_FROM_ALL WorkerMain.c)
set_target_properties(telemetryworker PROPERTIES OUTPUT_NAME OSConfigTelemetry)
target_include_directories(telemetryworker PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(telemetryworker PRIVATE telemetrytransport telemetryevent commonutils rt)
add_custom_target(telemetrybin DEPENDS telemetryworker)
add_custom_target(telemetry-worker-package ALL DEPENDS telemetryworker)
install(TARGETS telemetryworker RUNTIME DESTINATION bin)
install(TARGETS telemetryworker RUNTIME DESTINATION lib/osconfig)

# Offline resolver/probe utilities; the production worker owns its own DNS.
add_executable(telemetryresolverworker EXCLUDE_FROM_ALL ResolverWorker.c ResolverLookup.c)
target_link_libraries(telemetryresolverworker PRIVATE rt)
add_executable(telemetrytlsprobe EXCLUDE_FROM_ALL TlsProbe.c)
target_link_libraries(telemetrytlsprobe PRIVATE ${CMAKE_DL_LIBS})

# Explicit live invocation only; not installed or registered with ctest.
add_executable(telemetryariatest EXCLUDE_FROM_ALL AriaTest.c)
target_include_directories(telemetryariatest PRIVATE ../commonutils ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(telemetryariatest PRIVATE telemetrytransport telemetryevent rt)
