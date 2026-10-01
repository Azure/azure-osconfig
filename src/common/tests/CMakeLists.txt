# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
    SshUtilsUT.cpp
    TelemetryUT.cpp
    TelemetryEncoderUT.cpp
    TelemetryResolverUT.cpp
    Helper.c
)

target_link_libraries(commontests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    pthread
    logging
    commonutils
    asb
    parsonlib
    telemetry
    telemetryencoder
    telemetryresolver
)

add_executable(telemetryresolverlookupworker
    ../telemetry/TelemetryResolverWorker.c
    TelemetryResolverLookupStub.c)
target_include_directories(telemetryresolverlookupworker PRIVATE ../telemetry)
target_compile_definitions(telemetryresolverlookupworker PRIVATE
    getaddrinfo=TelemetryTestGetAddrInfo
    freeaddrinfo=TelemetryTestFreeAddrInfo)
target_link_libraries(telemetryresolverlookupworker PRIVATE rt)

add_executable(telemetryresolverfaultworker TelemetryResolverFaultWorker.c)
target_include_directories(telemetryresolverfaultworker PRIVATE ../telemetry)

add_dependencies(commontests telemetryresolverworker telemetryresolverlookupworker telemetryresolverfaultworker)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_RESOLVER_WORKER_PATH="$<TARGET_FILE:telemetryresolverworker>"
    TELEMETRY_RESOLVER_LOOKUP_WORKER_PATH="$<TARGET_FILE:telemetryresolverlookupworker>"
    TELEMETRY_RESOLVER_FAULT_WORKER_PATH="$<TARGET_FILE:telemetryresolverfaultworker>")

gtest_discover_tests(commontests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
