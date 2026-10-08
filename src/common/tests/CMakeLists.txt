# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
    SshUtilsUT.cpp
    TelemetryEncoderUT.cpp
    TelemetryEventUT.cpp
    TelemetryHttpUT.cpp
    TelemetryProxyUT.cpp
    TelemetryTlsProviderUT.cpp
    TelemetryTlsUT.cpp
    TelemetryResolverUT.cpp
    TelemetryUT.cpp
    TelemetryWorkerUT.cpp
    Helper.c
    ../telemetry/Telemetry.c
    ../telemetry/Tls.c
)

# Isolate mocked implementations by source file; other tests link the real telemetry library.
set_property(SOURCE TelemetryTlsProviderUT.cpp ../telemetry/Tls.c
    APPEND PROPERTY COMPILE_DEFINITIONS
        TelemetryTls=TelemetryTestTls
        TelemetryTlsCreate=TelemetryTestTlsCreate
        TelemetryTlsHandshake=TelemetryTestTlsHandshake
        TelemetryTlsWrite=TelemetryTestTlsWrite
        TelemetryTlsRead=TelemetryTestTlsRead
        TelemetryTlsDestroy=TelemetryTestTlsDestroy
        dlopen=TelemetryTestDlopen
        dlsym=TelemetryTestDlsym)

set_property(SOURCE TelemetryUT.cpp ../telemetry/Telemetry.c
    APPEND PROPERTY COMPILE_DEFINITIONS
        TelemetryInitialize=TelemetryTestInitialize
        TelemetryInitializeInternal=TelemetryTestInitializeInternal
        TelemetryCleanup=TelemetryTestCleanup
        GetModuleDirectory=TelemetryTestGetModuleDirectory
        GetCachedDistroName=TelemetryTestGetCachedDistroName
        OSConfigTimeStampSave=TelemetryTestTimeStampSave
        OSConfigGetElapsedTime=TelemetryTestGetElapsedTime
        OSConfigTelemetryStatusTraceInternal=TelemetryTestStatusTraceInternal
        OSConfigTelemetryBaselineRun=TelemetryTestBaselineRun
        OSConfigTelemetryRuleComplete=TelemetryTestRuleComplete
        OSConfigTelemetryCrashDetected=TelemetryTestCrashDetected
        TelemetryWorker=TelemetryTestWorker
        TelemetryWorkerCreate=TelemetryTestWorkerCreate
        TelemetryWorkerResolve=TelemetryTestWorkerResolve
        TelemetryWorkerSendEvent=TelemetryTestWorkerSendEvent
        TelemetryWorkerAccountPreparation=TelemetryTestWorkerAccountPreparation
        TelemetryWorkerDestroy=TelemetryTestWorkerDestroy
        SetFileAccess=TelemetryTestSetFileAccess
        GetOsPrettyName=TelemetryTestGetOsPrettyName)

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
    rt
)

add_executable(telemetryresolverlookupworker
    ../telemetry/ResolverWorker.c
    ../telemetry/ResolverLookup.c
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
    ../telemetry/WorkerMain.c
    ../telemetry/ResolverLookup.c
    TelemetryResolverLookupStub.c)
target_include_directories(telemetryworkerlookup PRIVATE ../telemetry "${CMAKE_CURRENT_BINARY_DIR}/../telemetry")
target_compile_definitions(telemetryworkerlookup PRIVATE
    getaddrinfo=TelemetryTestGetAddrInfo
    freeaddrinfo=TelemetryTestFreeAddrInfo)
target_link_libraries(telemetryworkerlookup PRIVATE telemetry commonutils rt)

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

add_executable(telemetrytlspeer TelemetryTlsPeer.cpp)
target_link_libraries(telemetrytlspeer PRIVATE ${CMAKE_DL_LIBS})
add_dependencies(commontests telemetrytlspeer)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_TLS_PEER_PATH="$<TARGET_FILE:telemetrytlspeer>")

add_dependencies(commontests telemetryariatest)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_ARIA_TEST_PATH="$<TARGET_FILE:telemetryariatest>")

add_executable(telemetryproducerprobe TelemetryProducerProbe.c)
set_target_properties(telemetryproducerprobe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../telemetry")
target_link_libraries(telemetryproducerprobe PRIVATE telemetry)
add_dependencies(telemetryproducerprobe telemetryworker)
add_dependencies(commontests telemetryproducerprobe)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_PRODUCER_PROBE_PATH="$<TARGET_FILE:telemetryproducerprobe>")
