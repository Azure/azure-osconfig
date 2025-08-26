# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrytests)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

include(CTest)
find_package(GTest REQUIRED)

add_executable(telemetrytests TelemetryTests.cpp)
target_link_libraries(telemetrytests gtest gtest_main pthread telemetrylib)

# Include directory for telemetry headers
target_include_directories(telemetrytests PRIVATE ../lib)

gtest_discover_tests(telemetrytests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
