# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

project(telemetryjsontests)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

include(CTest)
find_package(GTest REQUIRED)

add_executable(telemetryjsontests TelemetryJsonTests.cpp)

target_link_libraries(telemetryjsontests gtest gtest_main pthread telemetryjsonlib)

gtest_discover_tests(telemetryjsontests XML_OUTPUT_DIR ${GTEST_OUTPUT_DIR})
