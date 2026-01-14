# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetrytests)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

include(GoogleTest)
find_package(GTest CONFIG REQUIRED)

add_executable(telemetrytests TelemetryTests.cpp)

target_link_libraries(telemetrytests GTest::gtest GTest::gtest_main pthread telemetrylib)
gtest_discover_tests(telemetrytests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
