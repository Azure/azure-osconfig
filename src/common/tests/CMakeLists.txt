# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(commontests)

cmake_minimum_required(VERSION 3.2.0)

include(GoogleTest)
find_package(GTest REQUIRED)

add_executable(commontests
    CommonUtilsUT.cpp
    TelemetryEventUT.cpp
    TelemetryIpcUT.cpp
    TelemetryUT.cpp
    ../telemetry/Telemetry.c
)

# The producer suite mocks the platform and external-worker boundary only.
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
        TelemetryWorkerSendEvent=TelemetryTestWorkerSendEvent
        TelemetryWorkerDestroy=TelemetryTestWorkerDestroy
        SetFileAccess=TelemetryTestSetFileAccess
        GetOsPrettyName=TelemetryTestGetOsPrettyName)

# Keep the real commonutils reader, redirecting only its fixture path.
set_property(SOURCE ../telemetry/Telemetry.c
    APPEND PROPERTY COMPILE_DEFINITIONS
        LoadStringFromFile=TelemetryTestLoadStringFromFile
        access=TelemetryTestAccess)

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

# An offline IPC peer, not the telemetry executable or its implementation.
add_executable(telemetryipcpeer TelemetryIpcPeer.c)
target_include_directories(telemetryipcpeer PRIVATE ../telemetry ../logging)
target_link_libraries(telemetryipcpeer PRIVATE parsonlib)
add_dependencies(commontests telemetryipcpeer)
target_compile_definitions(commontests PRIVATE
    TELEMETRY_IPC_PEER_PATH="$<TARGET_FILE:telemetryipcpeer>")

gtest_discover_tests(commontests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
