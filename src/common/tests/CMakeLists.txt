# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)
find_package(Python3 3.4 REQUIRED COMPONENTS Interpreter)
find_program(TELEMETRY_TLS_OPENSSL_EXECUTABLE NAMES openssl REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
    SshUtilsUT.cpp
    TelemetryEncoderUT.cpp
    TelemetryEventUT.cpp
    TelemetryHttpUT.cpp
    TelemetryProxyUT.cpp
    TelemetryTlsUT.cpp
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
    telemetryevent
    telemetryhttp
    telemetryproxy
    telemetrytls
    telemetrytransport
    telemetryresolver
    telemetryworkerclient
    rt
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
target_include_directories(telemetryworkerlookup PRIVATE ../telemetry "${CMAKE_CURRENT_BINARY_DIR}/../telemetry")
target_compile_definitions(telemetryworkerlookup PRIVATE
    getaddrinfo=TelemetryTestGetAddrInfo
    freeaddrinfo=TelemetryTestFreeAddrInfo)
target_link_libraries(telemetryworkerlookup PRIVATE telemetrytransport telemetryevent commonutils rt)

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

add_executable(telemetrytlsprovidertests TelemetryTlsProviderUT.cpp ../telemetry/TelemetryTls.c)
target_include_directories(telemetrytlsprovidertests PRIVATE ../telemetry)
target_compile_definitions(telemetrytlsprovidertests PRIVATE
    dlopen=TelemetryTestDlopen
    dlsym=TelemetryTestDlsym)
target_link_libraries(telemetrytlsprovidertests PRIVATE GTest::gtest_main logging rt)
gtest_discover_tests(telemetrytlsprovidertests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

target_compile_definitions(commontests PRIVATE
    TELEMETRY_TLS_PYTHON="${Python3_EXECUTABLE}"
    TELEMETRY_TLS_OPENSSL="${TELEMETRY_TLS_OPENSSL_EXECUTABLE}"
    TELEMETRY_TLS_PEER_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/TelemetryTlsPeer.py")

add_dependencies(commontests telemetryariatest)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_ARIA_TEST_PATH="$<TARGET_FILE:telemetryariatest>")

add_executable(telemetryproducertests TelemetryUT.cpp ../telemetry/Telemetry.c)
target_compile_definitions(telemetryproducertests PRIVATE
    BUILD_TELEMETRY=1
    TelemetryWorkerCreate=TelemetryTestWorkerCreate
    TelemetryWorkerSend=TelemetryTestWorkerSend
    TelemetryWorkerAccountPreparation=TelemetryTestWorkerAccountPreparation
    TelemetryWorkerDestroy=TelemetryTestWorkerDestroy
    SetFileAccess=TelemetryTestSetFileAccess
    GetOsPrettyName=TelemetryTestGetOsPrettyName)
target_link_libraries(telemetryproducertests PRIVATE
    GTest::gtest_main commonutils telemetryevent ${CMAKE_DL_LIBS})
gtest_discover_tests(telemetryproducertests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})

if(BUILD_TELEMETRY)
    add_executable(telemetryproducerprobe TelemetryProducerProbe.c)
    set_target_properties(telemetryproducerprobe PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../telemetry")
    target_link_libraries(telemetryproducerprobe PRIVATE telemetry)
    add_dependencies(telemetryproducerprobe telemetryworker)
    add_dependencies(commontests telemetryproducerprobe)
    target_compile_definitions(commontests PRIVATE
        TELEMETRY_PRODUCER_PROBE_PATH="$<TARGET_FILE:telemetryproducerprobe>")
endif()
