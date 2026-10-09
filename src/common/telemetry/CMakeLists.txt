# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.21)
project(telemetry)

add_library(telemetry STATIC
    Encoder.c
    Event.c
    Http.c
    Proxy.c
    Resolver.c
    ResolverLookup.c
    Telemetry.c
    Tls.c
    Transport.c
    Worker.c)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(telemetry PUBLIC logging
    PRIVATE commonutils parsonlib ${CMAKE_DL_LIBS} rt)

include(../../cmake/TelemetryKey.cmake)
osconfig_configure_telemetry_key("${CMAKE_CURRENT_BINARY_DIR}/Keys.h" "")

# Production TLS and network operations stay in the owned worker, not the policy host.
add_executable(telemetryworker EXCLUDE_FROM_ALL WorkerMain.c)
set_target_properties(telemetryworker PROPERTIES OUTPUT_NAME OSConfigTelemetry)
target_include_directories(telemetryworker PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(telemetryworker PRIVATE telemetry commonutils rt)
add_custom_target(telemetrybin DEPENDS telemetryworker)
add_custom_target(telemetry-worker-package ALL DEPENDS telemetryworker)
install(TARGETS telemetryworker RUNTIME DESTINATION bin)
install(TARGETS telemetryworker RUNTIME DESTINATION lib/osconfig)

# Offline resolver fixture; the production worker owns its own DNS.
add_executable(telemetryresolverworker EXCLUDE_FROM_ALL ResolverWorker.c ResolverLookup.c)
target_link_libraries(telemetryresolverworker PRIVATE rt)

# Explicit live invocation only; not installed or registered with ctest.
add_executable(telemetryariatest EXCLUDE_FROM_ALL AriaTest.c)
target_include_directories(telemetryariatest PRIVATE ../commonutils ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(telemetryariatest PRIVATE telemetry rt)
