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
    TelemetryHttpUT.cpp
    TelemetryResolverUT.cpp
    TelemetryWorkerUT.cpp
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
    telemetryhttp
    telemetryresolver
    telemetryworkerclient
)

add_executable(telemetryresolverlookupworker
    ../telemetry/TelemetryResolverWorker.c
    ../telemetry/TelemetryResolverLookup.c
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

add_executable(telemetryworkerlookup
    ../telemetry/TelemetryWorkerMain.c
    ../telemetry/TelemetryResolverLookup.c
    TelemetryResolverLookupStub.c)
target_include_directories(telemetryworkerlookup PRIVATE ../telemetry)
target_compile_definitions(telemetryworkerlookup PRIVATE
    getaddrinfo=TelemetryTestGetAddrInfo
    freeaddrinfo=TelemetryTestFreeAddrInfo)
target_link_libraries(telemetryworkerlookup PRIVATE logging rt)

add_executable(telemetryworkerfault TelemetryWorkerFault.c)
target_include_directories(telemetryworkerfault PRIVATE ../telemetry)

add_executable(telemetryworkerbadready TelemetryWorkerFault.c)
target_include_directories(telemetryworkerbadready PRIVATE ../telemetry)
target_compile_definitions(telemetryworkerbadready PRIVATE TELEMETRY_TEST_BAD_READY=1)

add_dependencies(commontests telemetryworker telemetryworkerlookup telemetryworkerfault telemetryworkerbadready)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_WORKER_PATH="$<TARGET_FILE:telemetryworker>"
    TELEMETRY_WORKER_LOOKUP_PATH="$<TARGET_FILE:telemetryworkerlookup>"
    TELEMETRY_WORKER_BAD_READY_PATH="$<TARGET_FILE:telemetryworkerbadready>"
    TELEMETRY_WORKER_FAULT_PATH="$<TARGET_FILE:telemetryworkerfault>")

gtest_discover_tests(commontests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
